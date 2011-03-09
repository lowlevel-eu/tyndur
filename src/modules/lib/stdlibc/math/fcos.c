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

#define pio2 1.570796326794896619

double cos(double x)
{
    double res;
    asm("fcos;" : "=t" (res) : "0" (x));
    return res;
}

float cosf(float x)
{
    float res;
    asm("fcos" : "=t" (res) : "0" (x));
    return res;
}

long double cosl(long double x)
{
    long double res;
    asm("fcos" : "=t" (res) : "0" (x));
    return res;
}

double acos(double x) {
    double res;
    if (x < -1 || x > 1) {
        errno = EDOM;
        return NAN;
    }
    res = pio2 - asin(x);
    return res;
}

float acosf(float x) {
    float res;
    if (x < -1 || x > 1) {
        errno = EDOM;
        return NAN;
    }
    res = pio2 - asin(x);
    return res;
}

long double acosl(long double x) {
    long double res;
    if (x < -1 || x > 1) {
        errno = EDOM;
        return NAN;
    }
    res = pio2 - asin(x);
    return res;
}

double cosh(double x)
{
    if (x == NAN) {
        return NAN;
    }
    double res = (exp(x) + exp(-x)) / 2.0;
    if (((res == INFINITY) || (res == -INFINITY))
        && (x != INFINITY) && (x != - INFINITY))
    {
        errno = ERANGE;
        return HUGE_VAL;
    }
    return res;
}

float coshf(float x)
{
    if (x == NAN) {
        return NAN;
    }
    float res = (expf(x) + expf(-x)) / 2.0f;
    if (((res == INFINITY) || (res == -INFINITY))
        && (x != INFINITY) && (x != - INFINITY))
    {
        errno = ERANGE;
        return HUGE_VALF;
    }
    return res;
}

long double coshl(long double x)
{
    if (x == NAN) {
        return NAN;
    }
    long double res = (expl(x) + expl(-x)) / 2.0L;
    if (((res == INFINITY) || (res == -INFINITY))
        && (x != INFINITY) && (x != - INFINITY))
    {
        errno = ERANGE;
        return HUGE_VALL;
    }
    return res;
}
