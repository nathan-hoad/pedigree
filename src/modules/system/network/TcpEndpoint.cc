/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "NetManager.h"
#include "TcpManager.h"
#include <Log.h>
#include <syscallError.h>
#include <process/Scheduler.h>
#include <utilities/TimeoutGuard.h>

int TcpEndpoint::state()
{
    return static_cast<int>(TcpManager::instance().getState(m_ConnId));
}

Endpoint* TcpEndpoint::accept()
{
    // acquire() will return true when there is at least one connection waiting
    m_IncomingConnectionCount.acquire();
    Endpoint* e = m_IncomingConnections.popFront();
    return e;
}

void TcpEndpoint::listen()
{
    /// \todo Interface-specific connections
    m_IncomingConnections.clear();
    m_ConnId = TcpManager::instance().Listen(this, getLocalPort());
}

bool TcpEndpoint::connect(Endpoint::RemoteEndpoint remoteHost, bool bBlock)
{
    setRemoteHost(remoteHost);
    setRemotePort(remoteHost.remotePort);
    if (getLocalPort() == 0)
    {
        uint16_t port = TcpManager::instance().allocatePort();
        setLocalPort(port);
        if (getLocalPort() == 0)
            return false;
    }
    m_ConnId = TcpManager::instance().Connect(m_RemoteHost, getLocalPort(), this, bBlock);
    if (m_ConnId == 0)
        WARNING("TcpEndpoint::connect: got 0 for the connection id");
    return (m_ConnId != 0); /// \todo Error codes
}

void TcpEndpoint::close()
{
    TcpManager::instance().Disconnect(m_ConnId);
}

int TcpEndpoint::send(size_t nBytes, uintptr_t buffer)
{
    /// \todo Send needs to return an error code or something, and PUSH needs to be an option
    return TcpManager::instance().send(m_ConnId, buffer, true, nBytes);
};

int TcpEndpoint::recv(uintptr_t buffer, size_t maxSize, bool bBlock, bool bPeek)
{
    if ((!buffer || !maxSize) && !bPeek)
        return -1;

    bool queueReady = false;
    queueReady = dataReady(bBlock);

    /// \bug Possible race condition if another thread reads the same buffer after we have the size.
    /// \todo Needs either a Spinlock or access to the TcpBuffer lock to keep things safe.
    if (queueReady)
    {
        // Read off the front
        uintptr_t front = m_DataStream.getBuffer();

        // How many bytes to read?
        size_t nBytes = maxSize;
        size_t streamSize = m_DataStream.getSize();
        if (nBytes > streamSize)
            nBytes = streamSize;

        // Copy
        memcpy(reinterpret_cast<void*>(buffer), reinterpret_cast<void*>(front), nBytes);

        // Remove from the buffer, we've read
        if(!bPeek)
            m_DataStream.remove(0, nBytes);

        // We've read in this block
        return nBytes;
    }

    // no data is available - EOF?
    if (TcpManager::instance().getState(m_ConnId) > Tcp::FIN_WAIT_2)
    {
        return 0;
    }
    else
    {
        SYSCALL_ERROR(NoMoreProcesses);
        return -1;
    }
};

void TcpEndpoint::depositPayload(size_t nBytes, uintptr_t payload, uint32_t sequenceNumber, bool push)
{
    if (nBytes > 0xFFFF)
    {
        WARNING("Dud length passed to depositPayload!");
        return;
    }

    // If there's data to add to the shadow stream, add it now. Then, if the PUSH flag
    // is set, copy the shadow stream into the main stream. By allowing a zero-byte
    // deposit, data that did not have the PSH flag can be pushed to the application
    // when we receive a FIN.
    if (nBytes && payload)
        m_ShadowDataStream.insert(payload, nBytes, sequenceNumber - nBytesRemoved, false);
    if (push)
    {
        // Take all the data OUT of the shadow stream, shove it into the user stream
        size_t shadowSize = m_ShadowDataStream.getSize();
        m_DataStream.append(m_ShadowDataStream.getBuffer(), shadowSize);
        m_ShadowDataStream.remove(0, shadowSize);
        nBytesRemoved += shadowSize;

        // Data has arrived!
        for(List<Socket*>::Iterator it = m_Sockets.begin(); it != m_Sockets.end(); ++it)
        {
          (*it)->endpointStateChanged();
        }
    }
}

bool TcpEndpoint::dataReady(bool block, uint32_t tmout)
{
    if (block)
    {
        TimeoutGuard guard(tmout);
        if (!guard.timedOut())
        {
            bool ret = false;
            while (true)
            {
                if (m_DataStream.getSize() != 0)
                {
                    ret = true;
                    break;
                }

                // If there's no more data in the stream, and we need to close, do it
                // You'd think the above would handle this, but timing is an awful thing to assume
                // Much testing has led to the addition of the stream size check
                if (TcpManager::instance().getState(m_ConnId) > Tcp::FIN_WAIT_2 && (m_DataStream.getSize() == 0))
                {
                    break;
                }

                // Yield control otherwise we're using up all the CPU time here
                Scheduler::instance().yield();
            }
            return ret;
        }
        else
            return false;
    }
    else
    {
        return (m_DataStream.getSize() != 0);
    }
};

bool TcpEndpoint::shutdown(ShutdownType what)
{
    if(what == ShutSending)
    {
        // No longer able to write!
        NOTICE("No longer able to send!");
        TcpManager::instance().Shutdown(m_ConnId);
        return true;
    }
    else if(what == ShutReceiving)
    {
        WARNING("TCP: SHUT_RD not yet supported");
        return true; // Fake it
    }
    else if(what == ShutBoth)
    {
        // Standard shutdown, don't block
        TcpManager::instance().Shutdown(m_ConnId);
        return true;
    }
    return false;
}

void TcpEndpoint::stateChanged(Tcp::TcpState newState)
{
    // If we've moved into a data transfer state, notify the socket.
    if(newState == Tcp::ESTABLISHED)
    {
        for(List<Socket*>::Iterator it = m_Sockets.begin(); it != m_Sockets.end(); ++it)
        {
          (*it)->endpointStateChanged();
        }
    }
}
