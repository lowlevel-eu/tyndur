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

#ifndef _TMS_H_
#define _TMS_H_

struct tms_strings {
    void*   resstr;
    char*   translation;
} __attribute__((packed));

struct tms_lang {
    char*   lang;
    int     numbers;
    int     (*get_number)(int);

    const struct tms_strings* strings;
};

#define LANGUAGE(x) \
    static const void* __attribute__((section("tmslang"), used)) __lang = (x);

#define tms_glue(x, y, z) x ## y ## ## z
#define tms_xglue(x, y, z) tms_glue(x, y, z)
#define tms_stringify(x) #x
#define tms_xstringify(x) tms_stringify(x)

/**
 * Das Makro TMS_MODULE kann vor dem Einbinden dieses Headers optional gesetzt
 * werden und enthält dann den Namen des einbindenden Moduls. Dieser Name wird
 * in die Symbolnamen der Strings eingebaut.
 */
#ifndef TMS_MODULE
#define TMS_XMODULE
#else
#define TMS_XMODULE tms_xglue(TMS_MODULE, _,)
#endif

#define TMS_NAME(sym) tms_xglue(__tms_, TMS_XMODULE, sym)
#define TMS_SNAME(sym) tms_xstringify(TMS_NAME(sym))

/**
 * Definiert einen übersetzbaren String. Das Makro wird an Stelle eines
 * Stringliterals verwendet (z.B. printf(TMS(hello, "Hello World\n")))
 *
 * @param sym Eindeutiger Bezeichner für den String. Das Makro legt eine
 * Variable der Form __tms_module_sym, wobei module für TMS_MODULE steht.
 * @param string Der zu übersetzende String. Wenn keine Übersetzung
 * verfügbar ist, wird dieser String benutzt
 */
#define TMS(sym, string) \
    ({ \
        asm volatile( \
            ".ifndef "TMS_SNAME(sym)"\n\t" \
            ".pushsection tmsstring,\"aw\",@progbits\n\t" \
            ".set "TMS_SNAME(sym)"_string, %P0\n\t" \
            ".global " TMS_SNAME(sym) "\n\t" \
            TMS_SNAME(sym)":\n\t"\
            ".int %P0\n\t" \
            ".popsection\n\t" \
            ".else\n\t" \
            ".if "TMS_SNAME(sym)"_string != %P0\n\t" \
            ".error \"TMS: Mehrfachdefinition des Symbols '"TMS_SNAME(sym)"' mit verschiedenen Strings\\n\"\n\t" \
            ".endif\n\t" \
            ".endif\n" : : "i" (string)); \
        extern const char* TMS_NAME(sym); \
        TMS_NAME(sym); \
    })


/**
 * Initialisiert ein mehrsprachiges Programm durch Aktivieren der
 * Übersetzungen, die zur Sprache in der Umgebungsvariablen LANG passen.
 */
void tms_init(void);

/**
 * Lädt die Übersetzungen einer bestimmten Sprache. Diese Funktion muss in
 * der Regel nicht explizit aufgerufen werden, tms_init bereits die
 * Standardsprache aus LANG lädt.
 *
 * @param lang Sprachcode der zu ladenden Sprache
 */
void tms_init_lang(const char* lang);

#endif
