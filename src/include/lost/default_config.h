/*
 * Copyright (c) 2006 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Andreas Klebinger.
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

#ifndef _CONFIG_H_
#define _CONFIG_H_

#define MALLOC_LOST 1
#define MALLOC_BSD 2
#define MALLOC_LIBALLOC 3

#define ARCH_I386 1
#define ARCH_AMD64 2

#define SYSCALL_V1 1
#define SYSCALL_V2 2

//%desc "Architektur"
//%type "radio"
//%values "ARCH_I386,ARCH_AMD64"
#define CONFIG_ARCH ARCH_I386

//%desc "Kooperatives Multitasking"
//%type "yesno"
#undef CONFIG_COOPERATIVE_MULTITASKING

//%desc "Timerfrequenz in Hertz"
//%type "text"
#define CONFIG_TIMER_HZ 50

//%desc "Letzten Syscall fuer Debugausgaben merken"
//%type "yesno"
#define CONFIG_DEBUG_LAST_SYSCALL

//%desc "malloc"
//%type "radio"
//%values "MALLOC_LOST,MALLOC_BSD,MALLOC_LIBALLOC"
#define CONFIG_MALLOC MALLOC_LIBALLOC

//%desc "shell - Eingebaute Befehle"
//%type "yesno"
#define CONFIG_SHELL_BUILTIN_ONLY

//%desc "Releaseversion (Einige Pruefungen weglassen)"
//%type "yesno"
#undef CONFIG_RELEASE_VERSION

//%desc "Stubs in der LibC nicht benutzen"
//%type "yesno"
#define CONFIG_LIBC_NO_STUBS

#include <lost/version.h>

#endif // _CONFIG_H_

