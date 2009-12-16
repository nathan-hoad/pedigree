/*
 * Copyright (c) 2008 James Molloy, J�rg Pf�hler, Matthew Iselin
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

#include "syscallNumbers.h"

#include "syscall.h"

#include <processor/types.h>
#include <pedigree_config.h>

#include <unistd.h>

// These are provided by libc
void *malloc(size_t);
size_t strlen(const char *);

int pedigree_load_keymap(char *buf, size_t sz)
{
    return syscall2(PEDIGREE_LOAD_KEYMAP, (long)buf, (long)sz);
}

void pedigree_reboot()
{
    syscall0(PEDIGREE_REBOOT);
}

int pedigree_get_mount(char* mount_buf, char* info_buf, size_t n)
{
    return syscall3(PEDIGREE_GET_MOUNT, (long) mount_buf, (long) info_buf, n);
}

void pedigree_module_load(char *file)
{
    syscall1(PEDIGREE_MODULE_LOAD, (long)file);
}

void pedigree_module_unload(char *name)
{
    syscall1(PEDIGREE_MODULE_UNLOAD, (long)name);
}

int pedigree_module_is_loaded(char *name)
{
    return syscall1(PEDIGREE_MODULE_IS_LOADED, (long)name);
}

int pedigree_module_get_depending(char *name, char *buf, size_t bufsz)
{
    return syscall3(PEDIGREE_MODULE_GET_DEPENDING, (long)name, (long)buf, bufsz);
}

void pedigree_config_getcolname(size_t resultIdx, size_t n, char *buf, size_t bufsz)
{
    syscall4(PEDIGREE_CONFIG_GETCOLNAME, resultIdx, n, (long)buf, bufsz);
}

void pedigree_config_getstr_n(size_t resultIdx, size_t n, char *buf, size_t bufsz)
{
    syscall4(PEDIGREE_CONFIG_GETSTR_N, resultIdx, n, (long)buf, bufsz);
}
void pedigree_config_getstr_s(size_t resultIdx, const char *col, char *buf, size_t bufsz)
{
    syscall4(PEDIGREE_CONFIG_GETSTR_S, resultIdx, (long)col, (long)buf, bufsz);
}

int pedigree_config_getnum_n(size_t resultIdx, size_t n)
{
    return syscall2(PEDIGREE_CONFIG_GETNUM_N, resultIdx, n);
}
int pedigree_config_getnum_s(size_t resultIdx, const char *col)
{
    return syscall2(PEDIGREE_CONFIG_GETNUM_S, resultIdx, (long)col);
}

int pedigree_config_getbool_n(size_t resultIdx, size_t n)
{
    return syscall2(PEDIGREE_CONFIG_GETBOOL_N, resultIdx, n);
}
int pedigree_config_getbool_s(size_t resultIdx, const char *col)
{
    return syscall2(PEDIGREE_CONFIG_GETBOOL_S, resultIdx, (long)col);
}

int pedigree_config_query(const char *query)
{
    return syscall1(PEDIGREE_CONFIG_QUERY, (long)query);
}

void pedigree_config_freeresult(size_t resultIdx)
{
    syscall1(PEDIGREE_CONFIG_FREERESULT, resultIdx);
}

int pedigree_config_numcols(size_t resultIdx)
{
    return syscall1(PEDIGREE_CONFIG_NUMCOLS, resultIdx);
}

int pedigree_config_numrows(size_t resultIdx)
{
    return syscall1(PEDIGREE_CONFIG_NUMROWS, resultIdx);
}

int pedigree_config_nextrow(size_t resultIdx)
{
    return syscall1(PEDIGREE_CONFIG_NEXTROW, resultIdx);
}

int pedigree_config_was_successful(size_t resultIdx)
{
    return syscall1(PEDIGREE_CONFIG_WAS_SUCCESSFUL, resultIdx);
}

void pedigree_config_get_error_message(size_t resultIdx, char *buf, int bufsz)
{
    syscall3(PEDIGREE_CONFIG_GET_ERROR_MESSAGE, resultIdx, (long)buf, bufsz);
}

char *pedigree_config_escape_string(const char *str)
{
    char *buf = (char*)malloc(strlen(str)*2+1);
    const char *it = str;
    int i = 0;
    while(*it)
    {
        if(*it == '\'')
        {
            buf[i] = '\'';
            buf[i+1] = '\'';
            i+=2;
        }
        else
        {
            buf[i] = *it;
            i++;
        }
        it++;
    }
    buf[i] = '\0';

    return buf;
}

// Pedigree-specific function: login with given uid and password.
int login(uid_t uid, char *password)
{
    return (long)syscall2(PEDIGREE_LOGIN, uid, (long)password);
}