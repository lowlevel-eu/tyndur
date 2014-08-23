/*
 * Copyright (c) 2006-2007 tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Kevin Wolf.
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

#ifndef _LIMITS_H_
#define _LIMITS_H_

#define NAME_MAX 255

#define SCHAR_MIN (-128)
#define SCHAR_MAX 127

#define UCHAR_MAX 255

#define CHAR_MAX SCHAR_MAX
#define CHAR_MIN SCHAR_MIN

#define CHAR_BIT 8

#define SHRT_MIN (-32768)
#define SHRT_MAX 32767

#define USHRT_MAX 65535

#define LONG_MAX 0x7FFFFFFF
#define LONG_MIN ((signed long) -0x80000000)

#define INT_MAX 0x7FFFFFFF
#define INT_MIN ((signed int) -0x80000000)

#define ULONG_MAX 0xFFFFFFFF
#define UINT_MAX 0xFFFFFFFF

#define LLONG_MAX 0x7FFFFFFFFFFFFFFFLL
#define LLONG_MIN ((signed long long) -0x8000000000000000LL)
#define ULLONG_MAX 0xFFFFFFFFFFFFFFFFULL

#define _POSIX_PATH_MAX 4096
#define PATH_MAX _POSIX_PATH_MAX

#define _POSIX_SYMLOOP_MAX 8
#define SYMLOOP_MAX _POSIX_SYMLOOP_MAX

/**  Maximale laenge eines Multibyte-Zeichens in Bytes */
#define MB_LEN_MAX 4

#endif
