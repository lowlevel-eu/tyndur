/*
 * Copyright (c) 2009 The tyndur Project. All rights reserved.
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
#ifndef _PWD_H_
#define _PWD_H_

#include <sys/types.h>
#include <lost/config.h>

/** Repraesentiert einen Eintrag in der Benutzerdatenbank */
struct passwd {
    /** Benutzername */
    char*   pw_name;
    /** Benutzer-ID */
    uid_t   pw_uid;
    /** Gruppen-ID */
    gid_t   pw_gid;
    /** Homeverzeichnis */
    char*   pw_dir;
    /** Login-Shell */
    char*   pw_shell;
    /** Echter Name des Benutzers */
    char*   pw_gecos;
};

#ifdef __cplusplus
extern "C" {
#endif


#ifndef CONFIG_LIBC_NO_STUBS

/**
 * Benutzerdatenbank-Eintrag anhand des Namens holen
 *
 * @param name Benutzername
 *
 * @return Pointer auf internen Speicher, der bei weiteren Aufrufen
 *         ueberschrieben wird.
 */
struct passwd* getpwnam(const char* name);

/**
 * Benutzerdatenbank-Eintrag ahnand der UID holen
 *
 * @return Pointer auf internen Speicher, der bei weiteren Aufrufen
 *         ueberschrieben wird.
 */
struct passwd* getpwuid(uid_t uid);

#endif /* ndef CONFIG_LIBC_NO_STUBS */

#ifdef __cplusplus
}; // extern "C"
#endif

#endif /* ndef _PWD_H */

