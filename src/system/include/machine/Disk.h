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
#ifndef MACHINE_DISK_H
#define MACHINE_DISK_H

#include <machine/Device.h>

/**
 * A disk is a random access fixed size block device.
 */
class Disk : public Device
{
public:

    enum SubType
    {
        ATA = 0,
        ATAPI
    };

    Disk()
    {
        m_SpecificType = "Generic Disk";
    }
    Disk(Device *p) :
        Device(p)
    {
    }
    virtual ~Disk()
    {
    }

    virtual Type getType()
    {
        return Device::Disk;
    }

    virtual SubType getSubType()
    {
        return ATA;
    }

    virtual void getName(String &str)
    {
        str = "Generic disk";
    }

    virtual void dump(String &str)
    {
        str = "Generic disk";
    }

    /** Read from \p location on disk and return a pointer to it. \p location must be 512 byte aligned. The pointer returned is
     *  within a page of memory mapping 4096 bytes of disk area.
     * \param location The offset from the start of the device, in bytes, to start the read, must be multiple of 512.
     * \return Pointer to writable area of memory containing the data. If the data
     *         is written, the page is marked as dirty and may be written back
     *         to disk at any time (or forced with write().). */
    virtual uintptr_t read(uint64_t location)
    {
        return ~0;
    }

    /** This function schedules a cache writeback of the given location. The data to be written back is
     * fetched from the cache (pointer returned by \c read() ).
     * \param location The offset from the start of the device, in bytes, to start the write. Must be 512byte aligned. */
    virtual void write(uint64_t location)
    {
        return;
    }

    /** \brief Sets the page boundary alignment after a specific location on the disk.
     *
     * For example, if one has a partition starting on byte 512, one will
     * probably want 4096-byte reads to be aligned with this (so reading 4096
     * bytes from byte 0 on the partition will create one page of cache and not
     * span two).
     * \todo Better documentation.
     */
    virtual void align(uint64_t location)
    {
        return;
    }
};

#endif
