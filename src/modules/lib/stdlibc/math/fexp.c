/*
 * Copyright (c) 2009 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Patrick Kaufmann.
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

#include "math.h"
#include <errno.h>

double exp(double x)
{
    return expl(x);
}

float expf(float x)
{
    return expl(x);
}

long double expl(long double x)
{
    long double res;
    asm(// st0 = x
        "fldl2e;"       // Laedt log2e
        // st0 log2(e), st1 = x
        "fmulp;"        // Multipliziert mit x
        // st0 = x*log2(e)
        "fld %%st(0);"  //
        // st0 = st1 = x*log2(e)
        "frndint;"
        // st0 = int(x*log2(e)), st1 = x*log2(e)
        "fld1;"
        // st0 = 1, st1 = int(x*log2(e)), st2 = x*log2(e)
        "fscale;"
        // st0 = 2**(int(x*log2(e))), st1 = int(x*log2(e)), st2 = x*log2(e)
        "fxch %%st(2);"
        // st0 = x*log2(e), st1 = int(x*log2(e)), st2 = 2**(int(x*log2(e)))
        "fsubp %%st(1);"
        // st0 = x*log2(e) - int(x*log2(e)), st1 = 2**(int(x*log2(e)))
        "f2xm1;"
        // st0 = 2**(x*log2(e) - int(x*log2(e))) - 1, st1 = 2**(int(x*log2(e)))
        "fld1;"
        // st0 = 1, st1 = 2**(x*log2(e) - int(x*log2(e))) - 1, st2 = 2**(int(x*log2(e)))
        "faddp;"
        // st0 = 2**(x*log2(e) - int(x*log2(e))), st1 = 2**(int(x*log2(e)))
        "fmulp" : "=t" (res) : "0" (x));

    return res;
}

