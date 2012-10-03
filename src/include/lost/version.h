#ifndef TYNDUR_VERSION_H
#define TYNDUR_VERSION_H

#include <lost/config.h>

#define TYNDUR_VERSION "0.3.0"
#define TYNDUR_RELEASE "Smaug"

#if CONFIG_ARCH == ARCH_I386
    #define TYNDUR_ARCH "i386"
#elif CONFIG_ARCH == ARCH_AMD64
    #define TYNDUR_ARCH "amd64"
#else
    #error Unbekannte Architektur
#endif

#endif
