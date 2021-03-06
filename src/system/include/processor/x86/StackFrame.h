/*
 * 
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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

#ifndef KERNEL_PROCESSOR_X86_STACKFRAME_H
#define KERNEL_PROCESSOR_X86_STACKFRAME_H

#include <compiler.h>
#include <processor/types.h>
#include <processor/StackFrameBase.h>

/** @addtogroup kernelprocessorx86
 * @{ */

/** x86 StackFrame */
class X86StackFrame : public StackFrameBase
{
  public:
#if defined(DEBUGGER)
    /** Creates a stack frame based on the given processor state and also the given
     *  symbol name (mangled). */
    inline X86StackFrame(const ProcessorState &State, uintptr_t basePointer,
                         LargeStaticString mangledSymbol)
      : StackFrameBase(State, basePointer, mangledSymbol){}
    /** The destructor does nothing */
    inline ~X86StackFrame(){}
#endif
    static void construct(ProcessorState &state,
                          uintptr_t returnAddress,
                          unsigned int nParams,
                          ...);
    
  private:
#if defined(DEBUGGER)
    /** Returns the n'th 32-bit parameter in the stack frame. */
    virtual uintptr_t getParameter(size_t n);
#endif
};

/** @} */

#endif
