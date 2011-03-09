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

#define pi 3.1415926535897932384
#define pio2 1.570796326794896619


double tan(double x)
{
    double res;
    asm("fptan;"
        "fstp %%st(0)": "=t" (res) : "0" (x));
    return res;
}

float tanf(float x)
{
    float res;
    asm("fptan;"
        "fstp %%st(0)": "=t" (res) : "0" (x));
    return res;
}

long double tanl(long double x)
{
    long double res;
    asm("fptan;"
        "fstp %%st(0)": "=t" (res) : "0" (x));
    return res;
}

double atan(double x)
{
    double res;
    asm("fld1; fpatan" : "=t" (res) : "0" (x));
    return res;
}

float atanf(float x)
{
    float res;
    asm("fld1; fpatan" : "=t" (res) : "0" (x));
    return res;
}


long double atanl(long double x)
{
    long double res;
    asm("fld1; fpatan" : "=t" (res) : "0" (x));
    return res;
}

double atan2(double x, double y) {
    double res;
    asm("fpatan" : "=t" (res) : "0" (y), "u" (x));
    return res;
}


float atan2f(float x, float y) {
    float res;
    asm("fpatan" : "=t" (res) : "0" (y), "u" (x));
    return res;
}

long double atan2l(long double x, long double y) {
    long double res;
    asm("fpatan" : "=t" (res) : "0" (y), "u" (x));
    return res;
}

double tanh(double x)
{
    if (x == NAN) {
        return NAN;
    } else if (x == INFINITY) {
        return 1.0;
    } else if (x == -INFINITY) {
       return -1.0;
    }
    return sinh(x) / cosh(x);
}

float tanhf(float x)
{
    if (x == NAN) {
        return NAN;
    } else if (x == INFINITY) {
        return 1.0f;
    } else if (x == -INFINITY) {
       return -1.0f;
    }
    return sinhf(x) / coshf(x);
}

long double tanhl(long double x)
{
    if (x == NAN) {
        return NAN;
    } else if (x == INFINITY) {
        return 1.0L;
    } else if (x == -INFINITY) {
       return -1.0L;
    }
    return sinhl(x) / coshl(x);
}
