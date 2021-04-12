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

#include <types.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

// string.h koennte dieses Macro definiert haben, aber hier koennen wir
// das nicht brauchen.
#ifdef strerror_r
#undef strerror_r
#endif

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

static char* error_messages[] = {
    [ERANGE]    = "Ergebnis außerhalb des Wertebereichs",
    [EINVAL]    = "Ungüliges Argument",
    [ENOMEM]    = "Kein freier Speicher",
    [EINTR]     = "Unterbrochen",
    [ENOENT]    = "Datei nicht gefunden",
    [EEXIST]    = "Datei existiert schon",
    [EBADF]     = "Ungültiger Dateideskriptor",
    [EPERM]     = "Operation nicht erlaubt",
    [EIO]       = "Ein-/Ausgabefehler",
    [EXDEV]     = "Geräteübergreifender Link",
    [EFAULT]    = "Ungültige Adresse",
    [E2BIG]     = "Zu viele Argumente",
    [ENOTDIR]   = "Kein Verzeichnis",
    [EACCES]    = "Keine Berechtigung",
    [ENOEXEC]   = "Nicht ausführbar",
    [ECHILD]    = "Keine Kindprozesse",
    [EAGAIN]    = "Erneuter Versuch nötig",
    [EISDIR]    = "Ist ein Verzeichnis",
    [ENODEV]    = "Gerät existiert nicht",
    [ENOTTY]    = "Kein Terminal",
};

/**
 * Netter Fehlertext zu der angegebenen Fehlernummer zurueckgeben
 * TODO: ATM nur stub
 *
 * @param error_code Fehlernummer
 * @param buf Puffer, in dem die Fehlermeldung gespeichert wird
 * @param buflen Laenge des Puffers
 * 
 * @return 0 bei Erfolg, sonst errno (errno wird ebenfalls gesetzt)
 */
int _xsi_strerror_r(int error_code, char* buf, size_t buflen)
{
    int old_errno = errno;
    int printf_len;
    int ret = 0;

    if (error_code >= 0 && error_code < ARRAY_SIZE(error_messages) &&
        error_messages[error_code] != NULL)
    {
        printf_len = snprintf(buf, buflen,
            "%s (%d)", error_messages[error_code], error_code);
        errno = old_errno; // errno nicht veraendern
    } else {
        printf_len = snprintf(buf, buflen,
            "Unbekannter Fehler %d", error_code);
        ret = errno = EINVAL;
    }

    if (printf_len >= buflen) {
        ret = errno = ERANGE;
    }

    return ret;
}

char* strerror(int error_code)
{
    static char error_message[64];
    _xsi_strerror_r(error_code, error_message, sizeof(error_message));
    return error_message;
}

char* strerror_r(int error_code, char* buf, size_t buflen)
{
    _xsi_strerror_r(error_code, buf, buflen);
    return buf;
}

