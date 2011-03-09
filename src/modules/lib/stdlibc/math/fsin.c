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

double sin(double x)
{
    double res;
    asm("fsin" : "=t" (res) : "0" (x));
    return res;
}

float sinf(float x)
{
    float res;
    asm("fsin" : "=t" (res) : "0" (x));
    return res;
}

long double sinl(long double x)
{
    long double res;
    asm("fsin" : "=t" (res) : "0" (x));
    return res;
}

double asin(double x) {
    double res;
    if (x < -1 || x > 1) {
        errno = EDOM;
        return NAN;
    }
    res = 2 * atan(x / (1 + sqrt(1 - (x * x))));
    return res;
}

float asinf(float x) {
    float res;
    if (x < -1 || x > 1) {
        errno = EDOM;
        return NAN;
    }
    res = 2 * atanf(x / (1 + sqrtf(1 - (x * x))));
    return res;
}

long double asinl(long double x) {
    long double res;
    if (x < -1 || x > 1) {
        errno = EDOM;
        return NAN;
    }
    res = 2 * atanl(x / (1 + sqrtl(1 - (x * x))));
    return res;
}

double sinh(double x)
{
    if (x == NAN) {
        return NAN;
    }
    double res = (exp(x) - exp(-x)) / 2.0;
    if (((res == INFINITY) || (res == -INFINITY))
        && (x != INFINITY) && (x != - INFINITY))
    {
        errno = ERANGE;
        if (x > 0.0) {
            return HUGE_VAL;
        } else {
            return -HUGE_VAL;
        }
    }
    return res;
}

float sinhf(float x)
{
    if (x == NAN) {
        return NAN;
    }
    float res = (expf(x) - expf(-x)) / 2.0f;
    if (((res == INFINITY) || (res == -INFINITY))
        && (x != INFINITY) && (x != - INFINITY))
    {
        errno = ERANGE;
        if (x > 0.0f) {
            return HUGE_VALF;
        } else {
            return -HUGE_VALF;
        }
    }
    return res;
}

long double sinhl(long double x)
{
    if (x == NAN) {
        return NAN;
    }
    long double res = (expl(x) - expl(-x)) / 2.0L;
    if (((res == INFINITY) || (res == -INFINITY))
        && (x != INFINITY) && (x != - INFINITY))
    {
        errno = ERANGE;
        if (x > 0.0L) {
            return HUGE_VALL;
        } else {
            return -HUGE_VALL;
        }
    }
    return res;
}
