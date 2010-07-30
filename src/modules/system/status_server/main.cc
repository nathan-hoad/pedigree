/*
 * Copyright (c) 2010 Matthew Iselin
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
#include <Log.h>
#include <Module.h>
#include <Version.h>
#include <machine/Network.h>
#include <processor/Processor.h>
#include <network-stack/Endpoint.h>
#include <network-stack/TcpManager.h>
#include <network-stack/RoutingTable.h>
#include <network-stack/NetworkStack.h>
#include <network-stack/ConnectionBasedEndpoint.h>

class StringCallback : public Log::LogCallback
{
    public:
        StringCallback() : m_Str()
        {
        }

        virtual void callback(const char *s)
        {
            m_Str += s;
        }

        virtual inline ~StringCallback()
        {
        }

        String &getStr()
        {
            return m_Str;
        }
    private:
        String m_Str;
};

#define LISTEN_PORT     1234

StringCallback *pCallback = 0;

int clientThread(void *p)
{
    if(!p)
        return 0;

    ConnectionBasedEndpoint *pClient = reinterpret_cast<ConnectionBasedEndpoint*>(p);
    
    NOTICE("Got a client - state is " << pClient->state() << ".");

    // Wait for data from the client - block.
    if(!pClient->dataReady(true))
    {
        WARNING("Client from IP: " << pClient->getRemoteIp().toString() << " wasn't a proper HTTP client.");
        pClient->close();
        return 0;
    }

    // Read all incoming data from the client. Assume no nulls.
    /// \todo Wait until last 4 bytes are \r\n\r\n - that's the end of the HTTP header.
    String inputData("");
    while(pClient->dataReady(false))
    {
        char buff[512];
        pClient->recv(reinterpret_cast<uintptr_t>(buff), 512, true, false);
        inputData += buff;
    }

    bool bHeadRequest = false;

    List<String*> firstLine = inputData.tokenise(' ');
    String operation = *firstLine.popFront();
    if(!(operation == String("GET")))
    {
        if(operation == String("HEAD"))
            bHeadRequest = true;
        else
        {
            NOTICE("Unsupported HTTP method: '" << operation << "'");
            pClient->close();
            return 0;
        }
    }

    bool bNotFound = false;

    String path = *firstLine.popFront();
    if(!(path == String("/")))
        bNotFound = true;

    // Got a heap of information now - prepare to return
    size_t code = bNotFound ? 404 : 200;
    NormalStaticString statusLine;
    statusLine = "HTTP/1.0 ";
    statusLine += code;
    statusLine += " ";
    statusLine += bNotFound ? "Not Found" : "OK";

    // Build the response
    String response;
    response = statusLine;
    response += "\r\nContent-type: text/html";
    response += "\r\n\r\n";

    // Do we need data there?
    if(!bHeadRequest)
    {
        // Formulate data itself
        if(bNotFound)
        {
            response += "Error 404: Page not found.";
        }
        else
        {
            response += "<html><head><title>Pedigree - Live System Status Report</title></head><body>";
            response += "<h1>Pedigree Live Status Report</h1>";
            response += "<p>This is a live status report from a running Pedigree system.</p>";
            response += "<h3>Current Build</h3><pre>";

            {
                HugeStaticString str;
                str += "Pedigree - revision ";
                str += g_pBuildRevision;
                str += "<br />===========================<br />Built at ";
                str += g_pBuildTime;
                str += " by ";
                str += g_pBuildUser;
                str += " on ";
                str += g_pBuildMachine;
                str += "<br />Build flags: ";
                str += g_pBuildFlags;
                str += "<br />";
                response += str;
            }

            response += "</pre>";

            response += "<h3>Network Interfaces</h3>";
            response += "<table border='1'><tr><th>Interface #</th><th>IP Address</th><th>Subnet Mask</th><th>Gateway</th><th>DNS Servers</th></tr>";
            size_t i;
            for (i = 0; i < NetworkStack::instance().getNumDevices(); i++)
            {
                Network* card = NetworkStack::instance().getDevice(i);
                StationInfo info = card->getStationInfo();

                // Interface number
                response += "<tr><td>";
                TinyStaticString s;
                s.append(i);
                response += s;
                if(card == RoutingTable::instance().DefaultRoute())
                    response += " <b>(default route)</b>";
                response += "</td>";

                // IP address
                response += "<td>";
                response += info.ipv4.toString();
                response += "</td>";

                // Subnet mask
                response += "<td>";
                response += info.subnetMask.toString();
                response += "</td>";

                // Gateway
                response += "<td>";
                response += info.gateway.toString();
                response += "</td>";

                // DNS servers
                response += "<td>";
                if(!info.nDnsServers)
                    response += "(none)";
                else
                {
                    for(size_t j = 0; j < info.nDnsServers; j++)
                    {
                        response += info.dnsServers[j].toString();
                        if((j + 1) < info.nDnsServers)
                            response += "<br />";
                    }
                }
                response += "</td></tr>";
            }
            response += "</table>";

            response += "<h3>Kernel Log</h3>";
            response += "<pre>";
            response += pCallback->getStr();
            response += "</pre>";
            
            response += "</body></html>";
        }
    }

    // Send to the client
    pClient->send(response.length(), reinterpret_cast<uintptr_t>(static_cast<const char*>(response)));

    // All done - close the connection
    pClient->close();

    return 0;
}

int mainThread(void *p)
{
    ConnectionBasedEndpoint *pEndpoint = static_cast<ConnectionBasedEndpoint*>(TcpManager::instance().getEndpoint(LISTEN_PORT, RoutingTable::instance().DefaultRoute()));

    pEndpoint->listen();

    while(1)
    {
        ConnectionBasedEndpoint *pClient = static_cast<ConnectionBasedEndpoint*>(pEndpoint->accept());
        new Thread(Processor::information().getCurrentThread()->getParent(), clientThread, pClient);
    }

    return 0;
}

static void init()
{
    pCallback = new StringCallback();
    Log::instance().installCallback(pCallback);

    new Thread(Processor::information().getCurrentThread()->getParent(), mainThread, 0);
}

static void destroy()
{
}

MODULE_INFO("Status Server", &init, &destroy, "network-stack", "init");