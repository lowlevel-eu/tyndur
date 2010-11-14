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

#include <tms.h>
#include "resstr.h"

static int get_number(int n)
{
    return (n == 1 ? 0 : 1);
}

static const struct tms_strings dict[] = {
    // %d Bytes empfangen
    &RESSTR_HELPERS_RSBYTESRECEIVED,
    "%d %[0:byte:bytes] received",

    // %d/%d Bytes empfangen
    &RESSTR_HELPERS_RSBYTESRECEIVEDPARTIAL,
    "%d/%d %[1:byte:bytes] received",

    // Herunterladen von %s
    &RESSTR_HELPERS_RSDOWNLOADING,
    "Downloading %s",

    // %d %[0:Byte:Bytes] in %ds heruntergeladen
    &RESSTR_HELPERS_RSDOWNLOADSUMMARY,
    "Downloaded %d %[0:byte:bytes] in %ds",

    // Verbinde...
    &RESSTR_HELPERS_RSCONNECTING,
    "Connecting...",

    // Fehler: HTTP-Statuscode
    &RESSTR_HELPERS_RSHTTPERROR,
    "Error: HTTP status code",

    // Aufruf: lpt [scan|list|get <Paket>]'#10'  scan: L'#195#164'd
    &RESSTR_P$LPT_RSUSAGE,
    "Usage: lpt [scan|list|get <package>]\n"
    "  scan:    Download package lists from servers\n"
    "  list:    List all available packages\n"
    "  get:     Install a package\n"
    "  install: Install a package from a local file\n"
    "           (speficy file name as last parameter)",

    // Herunterladen und Entpacken von %s
    &RESSTR_P$LPT_RSDOWNLOADANDEXTRACT,
    "Downloading and extracting %s",

    // Suche %s
    &RESSTR_P$LPT_RSSEARCHING,
    "Searching %s",

    // Richte %s ein
    &RESSTR_P$LPT_RSCONFIGURING,
    "Configuring %s",

    // Installation hängt ab von:
    &RESSTR_P$LPT_RSLISTINSTALLDEPENDENCY,
    "Installation depends on: ",

    // Hängt ab von:
    &RESSTR_P$LPT_RSLISTRUNDEPENDENCY,
    "Depends on: ",

    // Fehler:
    &RESSTR_P$LPT_RSERROR,
    "Error:",

    // Kann Repository-Liste nicht laden:
    &RESSTR_P$LPT_RSCANTLOADREPOLIST,
    "Can't load repository list:",

    // Download der Paketliste f'#195#188'r %s fehlg
    &RESSTR_P$LPT_RSFAILDOWNLOADREPOLIST,
    "Download of the package list for %s failed",

    // Paket ist bereits installiert:
    &RESSTR_P$LPT_RSALREADYINSTALLED,
    "Package is already installed:",

    // Kann die Paketlisten nicht einlesen:
    &RESSTR_P$LPT_RSCANNOTREADPKGLISTS,
    "Cannot read package lists:",

    // Konnte keine Version von %s finden
    &RESSTR_P$LPT_RSCANNOTFINDPACKAGE,
    "Could not find a version of %s",

    // Konnte die Abh'#195#164'ngigkeiten des Pakets nic
    &RESSTR_P$LPT_RSCANNOTRESOLVEDEP,
    "Could not resolve dependencies:",

    // Falsche Parameterzahl
    &RESSTR_P$LPT_RSWRONGPARAMCOUNT,
    "Wrong parameter count",

    // Ung'#195#188'tige Aktion:
    &RESSTR_P$LPT_RSINVALIDACTION,
    "Invalid action:",

    // Paket %s fehlt
    &RESSTR_PACKAGES_RSPACKAGEMISSING,
    "Package %s is not available",

    // Repository unbekannten Typs!
    &RESSTR_REPOSITORIES_RSUNKNOWNREPOTYPE,
    "Repository of unknown type!",

    // Ung'#195#188'ltige Paketquelle:
    &RESSTR_REPOSITORIES_RSINVALIDPKGSRC,
    "Invalid package source!",

    // Keine Paketliste f'#195#188'r %s vorhand
    &RESSTR_REPOSITORY_BASE_RSNOPACKAGELIST,
    "No package list available for %s",

    // Lade Paketliste %s von %s
    &RESSTR_REPOSITORY_HTTP_RSDOWNLOADINGPKGLIST,
    "Downloading package list %s from %s",

    // Keine (lokale Datei)
    &RESSTR_REPOSITORY_SINGLE_RSLOCALPKGDESC,
    "None (local file)",

    0,
    0,
};

static const struct tms_lang lang = {
    .lang = "en",
    .numbers = 2,
    .get_number = get_number,
    .strings = dict,
};

LANGUAGE(&lang)
