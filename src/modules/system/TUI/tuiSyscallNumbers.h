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

#ifndef SYSCALL_NUMBERS_H
#define SYSCALL_NUMBERS_H

struct vid_req_t
{
    struct rgb_t *buffer;
    size_t x, y, x2, y2, w, h;
    struct rgb_t *c;
};

#define TUI_NEXT_REQUEST       1
#define TUI_LOG                2
#define TUI_GETFB              3
#define TUI_REQUEST_PENDING    4
#define TUI_RESPOND_TO_PENDING 5
#define TUI_CREATE_CONSOLE     6
#define TUI_SET_CTTY           7
#define TUI_SET_CURRENT_CONSOLE 8

#define TUI_VID_NEW_BUFFER     9
#define TUI_VID_SET_BUFFER     10
#define TUI_VID_UPDATE_BUFFER  11
#define TUI_VID_KILL_BUFFER    12
#define TUI_VID_BIT_BLIT       13
#define TUI_VID_FILL_RECT      14

#define TUI_EVENT_RETURNED             15
#define TUI_INPUT_REGISTER_CALLBACK    16
#define TUI_STOP_REQUEST_QUEUE 17

#endif