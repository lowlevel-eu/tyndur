/*
 * Copyright (c) 2008 The tyndur Project. All rights reserved.
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

#ifndef _LOCALE_H_
#define _LOCALE_H_

#include <lost/config.h>

struct lconv {
    /* Allgemeine Definitionen */
    char *decimal_point;
    char *thousands_sep;
    char *grouping;

    /* Waehrungen */
    char *int_curr_symbol;
    char *currency_symbol;
    char *mon_decimal_point;
    char *mon_thousands_sep;
    char *mon_grouping;
    char *positive_sign;
    char *negative_sign;
    char  int_frac_digits;
    char  frac_digits;
    char  p_cs_precedes;
    char  p_sep_by_space;
    char  n_cs_precedes;
    char  n_sep_by_space;

    /**
     * Position des Vorzeichens bei positiven Betraegen
     *  1: + 100 EUR   und  + $100
     *  2: 100 EUR +   und  $100 +
     *  3: 100 + EUR   und  + $100
     *  4: 100 EUR +   und  $ +100
     */
    char  p_sign_posn;

    /// Position des Vorzeichens bei negativen Betraegen
    char  n_sign_posn;
};


#ifndef CONFIG_LIBC_NO_STUBS
/**
 * Gibt Informationen zur aktiven Locale zurueck
 */
static inline struct lconv* localeconv(void)
{
    static struct lconv lconv = {
        .decimal_point = ",",
        .thousands_sep = " ",
        .mon_decimal_point = ",",
        .mon_thousands_sep = " ",
        .int_curr_symbol = "EUR",
        .currency_symbol = "EUR",
        .positive_sign = "+",
        .negative_sign = "-",
        .p_sign_posn = 1,
        .n_sign_posn = 1,
    };

    return &lconv;
};
#endif

#endif
