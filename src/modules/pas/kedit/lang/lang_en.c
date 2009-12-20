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
    // Die Datei ist noch nicht gespeichert. Speichern?
    &RESSTR_KEDIT_INPUT_RSFILENOTSAVED,
    "The file has not been saved yet. Save it now?",

    // Dateiname
    &RESSTR_KEDIT_MAIN_RSFILENAME,
    "File name",

    // Die Datei %s existiert und ist schreibgeschützt
    &RESSTR_KEDIT_MAIN_RSFILEISREADONLY,
    "The file %s exists already and is write protected",

    // Die Datei %s gibt's schon. Überschreiben?
    &RESSTR_KEDIT_MAIN_RSOVERWRITEEXISTINGFILE,
    "The file %s exists already. Overwrite it?",

    // Datei %s nicht gefunden.
    &RESSTR_KEDIT_MAIN_RSFILENOTFOUND,
    "File %s could not be found.",

    // F2: Speichern, F3: Laden, F8: Syntax, F10: Beenden,
    // EINFG: Einfügen/Ersetzen
    &RESSTR_KEDIT_TUI_RSMENUBAR,
    "F2: Save, F3: Load, F8: Syntax, F10: Exit; INS: Insert/Replace",

    // F2: R/W laden, F3: Laden, F8: Syntax, F10: Beenden,
    // EINFG: Einf'#195#188'gen/Ersetzen
    &RESSTR_KEDIT_TUI_RSMENUBARREADONLY,
    "F2: Open R/W, F3: Load, F8: Syntax, F10: Exit; INS: Insert/Replace",

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
