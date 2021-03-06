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

#include "DynamicLinker.h"
#include <Log.h>
#include <vfs/VFS.h>
#include <utilities/StaticString.h>
#include <Module.h>
#include <processor/Processor.h>
#include <processor/VirtualAddressSpace.h>
#include <processor/PhysicalMemoryManager.h>
#include <processor/KernelCoreSyscallManager.h>
#include <process/Scheduler.h>
#include <panic.h>

extern "C" void resolveSymbol(void);

void DynamicLinker::initPlt(Elf *pElf, uintptr_t value)
{
  // Value == loadBase. If this changes, add an extra parameter to get loadBase here!
    NOTICE("Value: " << value);
  uint64_t *got = reinterpret_cast<uint64_t*> (pElf->getGlobalOffsetTable()+value);
  if (reinterpret_cast<uintptr_t>(got) == value)
  {
    WARNING("DynamicLinker: Global offset table not found!");
    return;
  }
  NOTICE("got: " << reinterpret_cast<uintptr_t>(got));
  got++;                     // Go to GOT+8
  *got = value&0xFFFFFFFF;   // Library ID
  got++;                     // Got to GOT+16

  // Check if the resolve function has been set already...
  if (*got == 0)
  {
    uintptr_t resolveLocation = 0;

    // Grab a page to copy the PLT resolve function to.
    // Start at 0x20000000, looking for the next free page.
    /// \todo Change this to use the size of the elf!
    for (uintptr_t i = 0x40000000; i < 0x50000000; i += 0x1000) /// \todo Page size here.
    {
      bool failed = false;
      if (Processor::information().getVirtualAddressSpace().isMapped(reinterpret_cast<void*>(i)))
      {
        failed = true;
        continue;
      }

      resolveLocation = i;
      break;
    }

    if (resolveLocation == 0)
    {
      ERROR("DynamicLinker: nowhere to put resolve function.");
      return;
    }

    physical_uintptr_t physPage = PhysicalMemoryManager::instance().allocatePage();
    bool b = Processor::information().getVirtualAddressSpace().map(physPage,
                                                                   reinterpret_cast<void*> (resolveLocation),
                                                                   VirtualAddressSpace::Write|VirtualAddressSpace::Execute);

    if (!b)
    {
      ERROR("DynamicLinker: Could not map resolve function.");
    }

    // Memcpy over the resolve function into the user address space.
    // resolveSymbol is an ASM function, defined in ./asm-amd64.s
    memcpy(reinterpret_cast<uint8_t*> (resolveLocation), reinterpret_cast<uint8_t*> (&::resolveSymbol), 0x1000); /// \todo Page size here.

    *got = resolveLocation;
  }

}

uintptr_t DynamicLinker::resolvePltSymbol(uintptr_t libraryId, uintptr_t symIdx)
{
    // Find the correct ELF to patch.
    Elf *pElf = 0;
    uintptr_t loadBase = libraryId;

    if (libraryId == 0)
        pElf = m_pProgramElf;
    else
    {
        SharedObject *pSo = m_Objects.lookup(libraryId);
        if (pSo) pElf = pSo->elf;
    }

    if (!pElf)
    {
        ERROR("DynamicLinker::resolvePltSymbol: No library found for id `" << Hex << libraryId << "'");
        return 0;
    }

    uintptr_t result = pElf->applySpecificRelocation(symIdx, m_pProgramElf->getSymbolTable(), loadBase);
    if(result == 0)
        result = pElf->applySpecificRelocation(symIdx, m_pProgramElf->getSymbolTable(), loadBase, SymbolTable::NotOriginatingElf);
NOTICE("Resolve " << symIdx << ", lib: " << libraryId << "; " << result);
    return result;
}
