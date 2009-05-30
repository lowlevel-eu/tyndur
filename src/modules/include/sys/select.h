/*
 * Copyright (c) 2009 The tyndur Project. All rights reserved.
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
#ifndef _SYS_SELECT_H_
#define _SYS_SELECT_H_

#include <sys/time.h>
#include <stdint.h>

#ifndef CONFIG_LIBC_NO_STUBS

/**
 * Menge von Dateideskriptoren
 *
 * FIXME Irgendwann will man da sicher mehr als 32 benutzen koennen
 */
typedef struct {
    uint32_t bits;
} fd_set;

#define FD_ZERO(fdset) do { (fdset)->bits = 0; } while (0)
#define FD_SET(fd, fdset) do { (fdset)->bits |= (1 << (fd)); } while (0)
#define FD_CLR(fd, fdset) do { (fdset)->bits &= ~(1 << (fd)); } while (0)
#define FD_ISSET(fd, fdset) ((fdset)->bits & (1 << (fd)))

/**
 * Prueft, welche der gegebenen Dateideskriptoren bereit zum Lesen oder
 * Schreiben sind oder ein Fehler fuer sie aufgetreten ist. Dateideskriptoren
 * die nicht bereit bzw. in einem Fehlerzustand sind, werden aus der Menge
 * entfernt.
 *
 * @param number_fds Nummer des hoechsten Dateideskriptors in einem der
 * uebergebenen Mengen.
 * @param readfds Dateideskriptoren, bei denen ueberprueft werden soll, ob sie
 * zum Lesen bereit sind.
 * @param writefds Dateideskriptoren, bei denen ueberprueft werden soll, ob sie
 * zum Schreiben bereit sind.
 * @param errfds Dateideskriptoren, bei denen ueberprueft werden soll, ob sie
 * in einem Fehlerzustand sind.
 * @param timeout Maximales Timeout, das gewartet werden soll, falls kein
 * Deskriptor bereit ist. NULL fuer dauerhaftes Blockieren.
 *
 * @return Anzahl der Dateideskriptoren, die bereit bzw. in einem Fehlerzustand
 * sind
 */
int select(int number_fds, fd_set* readfds, fd_set* writefds,
    fd_set* errfds, struct timeval* timeout);
#endif

#endif
