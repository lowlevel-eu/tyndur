/*
 * Copyright (c) 2008 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Antoine Kaufmann.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _KBD_H_
#define _KBD_H_

#define KBD_RPC_REGISTER_CALLBACK "RPCREGIS"
#define KBD_RPC_CALLBACK "KBDEVENT"

/**
 * Das sind einige der Keycodes die KBC ueber die Callback-Funktion verschickt.
 * Es handelt sich dabei aber nur um diese Keycodes, die auf allen Layouts am
 * selben Ort sind.
 */

#define KEYCODE_SHIFT_LEFT      42
#define KEYCODE_SHIFT_RIGHT     54
#define KEYCODE_CONTROL_LEFT    29
#define KEYCODE_CONTROL_RIGHT   97
#define KEYCODE_ALT             56
#define KEYCODE_ALTGR           100

#define KEYCODE_INSERT          110
#define KEYCODE_DELETE          111
#define KEYCODE_PAGE_UP         104
#define KEYCODE_PAGE_DOWN       109
#define KEYCODE_HOME            102
#define KEYCODE_END             107

#define KEYCODE_F1              59
#define KEYCODE_F2              60
#define KEYCODE_F3              61
#define KEYCODE_F4              62
#define KEYCODE_F5              63
#define KEYCODE_F6              64
#define KEYCODE_F7              65
#define KEYCODE_F8              66
#define KEYCODE_F9              67
#define KEYCODE_F10             68
#define KEYCODE_F11             87
#define KEYCODE_F12             88

#define KEYCODE_ARROW_UP        103
#define KEYCODE_ARROW_DOWN      108
#define KEYCODE_ARROW_LEFT      105
#define KEYCODE_ARROW_RIGHT     106

#define KEYCODE_KP_0             82
#define KEYCODE_KP_1             79
#define KEYCODE_KP_2             80
#define KEYCODE_KP_3             81
#define KEYCODE_KP_4             75
#define KEYCODE_KP_5             76
#define KEYCODE_KP_6             77
#define KEYCODE_KP_7             71
#define KEYCODE_KP_8             72
#define KEYCODE_KP_9             73
#define KEYCODE_KP_PLUS          78
#define KEYCODE_KP_MINUS         74
#define KEYCODE_KP_MUL           37
#define KEYCODE_KP_DIV           99
#define KEYCODE_KP_DECIMAL       83
#define KEYCODE_KP_ENTER         96

#define KEYCODE_SCROLL_LOCK     70

#endif // ifndef _KBD_H_

