/*
 * Copyright (c) 2010 The tyndur Project. All rights reserved.
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
#include <string.h>
#include <env.h>

extern struct tms_lang* __start_tmslang;
extern struct tms_lang* __stop_tmslang;

/**
 * Lädt die Übersetzungen einer bestimmten Sprache. Diese Funktion muss in
 * der Regel nicht explizit aufgerufen werden, tms_init bereits die
 * Standardsprache aus LANG lädt.
 *
 * @param lang Sprachcode der zu ladenden Sprache
 */
void tms_init_lang(const char* lang)
{
    struct tms_lang** plang;
    const struct tms_strings* strings;

    plang= &__start_tmslang;
    while (plang < &__stop_tmslang) {

        if (!strcmp((*plang)->lang, lang)) {
            strings = (*plang)->strings;
            while (strings->resstr) {
                *(const char**)strings->resstr = strings->translation;
                strings++;
            }
            break;
        }

        plang++;
    }
}

/**
 * Initialisiert ein mehrsprachiges Programm durch Aktivieren der
 * Übersetzungen, die zur Sprache in der Umgebungsvariablen LANG passen.
 */
void tms_init(void)
{
    const char* lang = getenv("LANG");

    if (lang != NULL) {
        tms_init_lang(lang);
    }
}
