/*  
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
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
#ifndef _FCNTL_H_
#define _FCNTL_H_
#include <sys/types.h>

#define O_RDONLY 1
#define O_WRONLY 2
#define O_RDWR 4
#define O_APPEND 8
#define O_CREAT 16
#define O_EXCL 32
#define O_TRUNC 64

#define O_ACCMODE (O_RDONLY | O_WRONLY | O_RDWR)

enum {
    /// Liest die Flags des Filedeskriptors
    F_GETFD,

    /// Setzt die Flags ds Filedeskriptors
    F_SETFD,

    /// Liest die Flags einer Datei
    F_GETFL,

    /// Setzt die Flags einer Datei
    F_SETFL,
};

/* Manche Programme/configures erwarten, dass es Makros sind... */
#define F_GETFD F_GETFD
#define F_SETFD F_SETFD
#define F_GETFL F_GETFL
#define F_SETFL F_SETFL

/// Emulierter Unix-Syscall zum oeffnen von Dateien
int open(const char* filename, int flags, ...);

/// Emulierter Unix-Syscall zum erstellen von Dateien
int creat(const char *pathname, mode_t mode);

/// Fuehrt unterschiedliche Aktionen (F_*) auf Dateien aus
int fcntl(int fd, int cmd, ...);

#endif //ifndef _FCNTL_H_

