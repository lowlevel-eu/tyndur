/*  
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Antoine Kaufmann.
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

#ifndef _TIME_H_
#define _TIME_H_
#include <sys/types.h>
#include <lost/config.h>

struct tm {
    int tm_sec;     /// Sekunden 0-60
    int tm_min;     /// Minuten 0-59
    int tm_hour;    /// Stunde 0-23
    int tm_mday;    /// Tag 1-31
    int tm_mon;     /// Monat 1-12
    int tm_year;    /// Jahre seit 1900
    int tm_wday;    /// Wochentag 0-6 (Wobei 0 = Sonntag)
    int tm_yday;    /// Tag des Jahres 0-365
    int tm_isdst;   /// Todo
};


#ifdef __cplusplus
extern "C" {
#endif

time_t time(time_t* t);

/// Timestamp in tm-Struct einfuellen
struct tm* gmtime(const time_t* time_ptr);
struct tm* gmtime_r(const time_t* time_ptr, struct tm* result);

struct tm* localtime(const time_t* time_ptr);
struct tm* localtime_r(const time_t* time_ptr, struct tm* result);

size_t strftime(char *str, size_t maxs, const char *datestr, const struct tm *tm);

#ifndef CONFIG_LIBC_NO_STUBS
/// Datum und Uhrzeit in einen String umwandeln
char* ctime(const time_t* time_ptr);

/// Zeit und Datum als String
char* asctime(const struct tm* time_ptr);

time_t mktime(struct tm* time_ptr);
#endif

#ifdef __cplusplus
}; // extern "C"
#endif

#endif

