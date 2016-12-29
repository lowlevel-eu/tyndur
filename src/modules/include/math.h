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

#ifndef _MATH_H_
#define _MATH_H_

#define NAN         (__builtin_nanf (""))
#define INFINITY    (__builtin_inff ())
#define HUGE_VAL    (__builtin_huge_val())
#define HUGE_VALF   (__builtin_huge_valf())
#define HUGE_VALL   (__builtin_huge_vall())
#define MAXFLOAT    3.4028234663852886e+38


#define M_E         2.71828182845904523536
#define M_LOG2E     1.44269504088896340735
#define M_LOG10E    0.43429448190325182765
#define M_LN2       0.69314718055994530942
#define M_LN10      2.30258509299404568402
#define M_PI        3.14159265358979323846
#define M_PI_2      1.57079632679489661923
#define M_PI_4      0.78539816339744830962
#define M_1_PI      0.31830988618379067154
#define M_2_PI      0.63661977236758134308
#define M_2_SQRTPI  1.12837916709551257390
#define M_SQRT2     1.41421356237309504880
#define M_SQRT1_2   0.70710678118654752440


#define FP_INFINITE     1
#define FP_NAN          2
#define FP_NORMAL       3
#define FP_SUBNORMAL    4
#define FP_ZERO         5

#define fpclassify(x) \
    __builtin_fpclassify(FP_NAN, FP_INFINITE, FP_NORMAL, FP_SUBNORMAL, \
                         FP_ZERO, x)

#define isfinite(x)     __builtin_isfinite(x)
#define isinf(x)        __builtin_isinf(x)
#define isnan(x)        __builtin_isnan(x)
#define isnormal(x)     __builtin_isnormal(x)

#ifdef __cplusplus
extern "C" {
#endif

double      tan(double);
float       tanf(float);
long double tanl(long double);
double      sqrt(double);
float       sqrtf(float);
long double sqrtl(long double);
double      sin(double);
float       sinf(float);
long double sinl(long double);
double      log2(double);
float       log2f(float);
long double log2l(long double);
double      log10(double);
float       log10f(float);
long double log10l(long double);
double      log(double);
float       logf(float);
long double logl(long double);
double      ldexp(double, int);
float       ldexpf(float, int);
long double ldexpl(long double, int);
double      fabs(double);
float       fabsf(float);
long double fabsl(long double);
double      exp(double);
float       expf(float);
long double expl(long double);
double      cos(double);
float       cosf(float);
long double cosl(long double);
double      atan(double);
float       atanf(float);
long double atanl(long double);
double      atan2(double, double);
float       atan2f(float, float);
long double atan2l(long double, long double);
double      asin(double);
float       asinf(float);
long double asinl(long double);
double      acos(double);
float       acosf(float);
long double acosl(long double);
double      sinh(double);
float       sinhf(float);
long double sinhl(long double);
double      cosh(double);
float       coshf(float);
long double coshl(long double);
double      tanh(double);
float       tanhf(float);
long double tanhl(long double);

double      floor(double x);
float       floorf(float x);
long double floorl(long double x);
double      ceil(double x);
float       ceilf(float x);
long double ceill(long double x);
double      trunc(double x);
float       truncf(float x);
long double truncl(long double x);
double      round(double x);
float       roundf(float x);
long double roundl(long double x);
double      nearbyint(double x);
float       nearbyintf(float x);
long double nearbyintl(long double x);

double      rint(double x);
double      pow(double x, double y);

long        lround(double x);
long        lroundf(float x);
long        lroundl(long double x);
long long   llround(double x);
long long   llroundf(float x);
long long   llroundl(long double x);

long        lrint(double x);
long        lrintf(float x);
long        lrintl(long double x);
long long   llrint(double x);
long long   llrintf(float x);
long long   llrintl(long double x);

double      fmod(double x, double y);
float       fmodf(float x, float y);
long double fmodl(long double x, long double y);

double      frexp(double x, int* exp);
float       frexpf(float x, int* exp);
long double frexpl(long double x, int* exp);

float       modff(float x, float* i);
double      modf(double x, double* i);
long double modfl(long double x, long double* i);

#ifdef __cplusplus
}; // extern "C"
#endif

#endif

