/*
 * Copyright (c) 2006 The tyndur Project. All rights reserved.
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

#ifndef _ERRNO_H_
#define _ERRNO_H_

#define ERANGE 1
#define EINVAL 2
#define ENOMEM 3
#define EINTR 4
#define ENOENT 5
#define EEXIST 6
#define EBADF 7
#define EPERM 8
#define EIO 9
#define EXDEV 10
#define EFAULT 11
#define E2BIG 12
#define ENOTDIR 13
#define EACCES 14
#define EMFILE 15
#define ENOEXEC 16
#define ECHILD 17
#define EAGAIN 18
#define ENFILE 19
#define EISDIR 20
#define ENODEV 21
#define ENOTTY 22
#define EDOM 23
#define ENXIO 24
#define ESRCH 25
#define EPIPE 26
#define EILSEQ 27


#define EAFNOSUPPORT 28
#define ETIMEDOUT 29
#define EPROTOTYPE 30
#define ECONNREFUSED 31
#define ENOTCONN 32
#define ECONNRESET 33
#define EINPROGRESS 34
#define EALREADY 35
#define EISCONN 36

#define EOPNOTSUPP 37
#define EROFS 38
#define ENOSPC 39
#define EBUSY 40
#define EOVERFLOW 41
#define EFBIG 42
#define EDEADLK 43
#define EADDRNOTAVAIL 44
#define ENOSYS 45
#define ENAMETOOLONG 46
#define ESPIPE 47
#define EMLINK 48
#define ENOTEMPTY 49

#define EHOSTDOWN 50
#define EMSGSIZE 51
#define EPROTO 52

#define EADDRINUSE 53
#define ECONNABORTED 54
#define EDESTADDRREQ 55
#define EHOSTUNREACH 56
#define ENETDOWN 57
#define ENETRESET 58
#define ENETUNREACH 59
#define ENOBUFS 60
#define ENOLCK 61
#define ENOMSG 62
#define ENOPROTOOPT 63
#define ENOTSOCK 64
#define EWOULDBLOCK 65
#define EPROTONOSUPPORT 66
#define ELOOP 67

extern int errno;

#endif
