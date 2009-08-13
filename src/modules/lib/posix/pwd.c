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

#include <string.h>
#include <pwd.h>
#include <lost/config.h>


#ifndef CONFIG_LIBC_NO_STUBS
static struct passwd pwdent;

/**
 * Benutzerdatenbank-Eintrag anhand des Namens holen
 *
 * @param name Benutzername
 *
 * @return Pointer auf internen Speicher, der bei weiteren Aufrufen
 *         ueberschrieben wird.
 */
struct passwd* getpwnam(const char* name)
{
    // FIXME: Wir haben nur den Benutzer root
    if (strcmp(name, "root")) {
        return NULL;
    }

    return getpwuid(0);
}

/**
 * Benutzerdatenbank-Eintrag ahnand der UID holen
 *
 * @return Pointer auf internen Speicher, der bei weiteren Aufrufen
 *         ueberschrieben wird.
 */
struct passwd* getpwuid(uid_t uid)
{
    // FIXME: Wir haben nur den Benutzer root
    if (uid != 0) {
        return NULL;
    }

    pwdent.pw_name  = "root";
    pwdent.pw_uid   = 0;
    pwdent.pw_gid   = 0;
    pwdent.pw_dir   = "file:/";
    pwdent.pw_shell = "file:/apps/sh";
    pwdent.pw_gecos = "tyndur-Benutzer";
    return &pwdent;
}

#endif /* ndef CONFIG_LIBC_NO_STUBS */

