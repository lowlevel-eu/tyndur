#include "math.h"
#include <errno.h>

double log2(double x) {
    double one = 1;
    double res;
    if (x <= 0.0) {
        errno = EDOM;
        return NAN;
    }
    asm("fyl2x" : "=t" (res) : "0" (x), "u" (one));
    return res;
}

float log2f(float x)
{
    float one = 1;
    float res;
    if (x <= 0.0) {
        errno = EDOM;
        return NAN;
    }
    asm("fyl2x" : "=t" (res) : "0" (x), "u" (one));
    return res;
}


long double log2l(long double x)
{
    long double one = 1;
    long double res;
    if (x <= 0.0) {
        errno = EDOM;
        return NAN;
    }
    asm("fyl2x" : "=t" (res) : "0" (x), "u" (one));
    return res;
}

double log(double x)
{
    double one = 1;
    double res;
    if (x <= 0.0) {
        errno = EDOM;
        return NAN;
    }
    asm("fyl2x;"
        "fldl2e;"
        "fdivrp" : "=t" (res) : "0" (x), "u" (one));
    return res;
}

float logf(float x)
{
    float one = 1;
    float res;
    if (x <= 0.0) {
        errno = EDOM;
        return NAN;
    }
    asm("fyl2x;"
        "fldl2e;"
        "fdivrp" : "=t" (res) : "0" (x), "u" (one));
    return res;
}

long double logl(long double x)
{
    long double one = 1;
    long double res;
    if (x <= 0.0) {
        errno = EDOM;
        return NAN;
    }
    asm("fyl2x;"
        "fldl2e;"
        "fdivrp" : "=t" (res) : "0" (x), "u" (one));
    return res;

}


double log10(double x)
{
    double one = 1;
    double res;
    if (x <= 0.0) {
        errno = EDOM;
        return NAN;
    }
    asm("fyl2x;"
        "fldl2t;"
        "fdivrp" : "=t" (res) : "0" (x), "u" (one));
    return res;

}

float log10f(float x)
{
    float one = 1;
    float res;
    if (x <= 0.0) {
        errno = EDOM;
        return NAN;
    }
    asm("fyl2x;"
        "fldl2t;"
        "fdivrp" : "=t" (res) : "0" (x), "u" (one));
    return res;

}

long double log10l(long double x)
{
    long double one = 1;
    long double res;
    if (x <= 0.0) {
        errno = EDOM;
        return NAN;
    }
    asm("fyl2x;"
        "fldl2t;"
        "fdivrp" : "=t" (res) : "0" (x), "u" (one));
    return res;

}

