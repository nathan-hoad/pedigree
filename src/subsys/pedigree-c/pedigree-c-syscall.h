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

#ifndef _PEDIGREE_SYSCALL_H
#define _PEDIGREE_SYSCALL_H

// If you change this, ensure you change src/system/include/processor/Syscalls.h !
#define PEDIGREE_C_SYSCALL_SERVICE 4

#ifdef X86
#include "pedigree-c-syscall-i686.h"
#define SYSCALL_TARGET_FOUND
#endif

#ifdef X64
#include "pedigree-c-syscall-amd64.h"
#define SYSCALL_TARGET_FOUND
#endif

#ifdef PPC_COMMON
#include "pedigree-c-syscall-ppc.h"
#define SYSCALL_TARGET_FOUND
#endif

#ifdef ARM_COMMON
#include "pedigree-c-syscall-arm.h"
#define SYSCALL_TARGET_FOUND
#endif

#ifndef SYSCALL_TARGET_FOUND
#error Syscall target not found!
#endif


#endif
