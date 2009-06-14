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

#include <Debugger.h>

#include <DebuggerIO.h>
#include <LocalIO.h>
#include <SerialIO.h>

#include <utilities/StaticString.h>

#include <processor/Processor.h>
#include <machine/Machine.h>

#include <Log.h>
#include <utilities/utility.h>

void _assert(bool b, const char *file, int line, const char *func)
{
    if(b)
        return;

    ERROR("Assertion failed in file " << String(file) << " (line " << Dec << line << Hex << ")");
    ERROR("In function '" << func << "'.");
    Processor::breakpoint();

    ERROR("You may not resume after a failed assertion.");
    Processor::halt();
}
