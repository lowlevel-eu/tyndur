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

#ifndef _LOCK_H_
#define _LOCK_H_

#include <stdint.h>
#include <stdbool.h>

/* Spinlocks ***********************************/

#define LOCK_LOCKED 1
#define LOCK_UNLOCKED 0

/// Typ fuer einen Lock
typedef volatile uint8_t lock_t;


/// Einen Lock sperren
void lock(lock_t* l);

/// Sperre aufheben
void unlock(lock_t* l);

/// Testen ob ein Lock gesperrt ist
bool locked(lock_t* l);

/// Wartet, bis ein Lock nicht mehr gesperrt ist.
void lock_wait(lock_t* l);

/**
 * Eine Variable Inkrementieren, waerend der Bus gesperrt ist.
 *
 * @param var Pointer auf die zu inkrementierende Variable
 */
static inline uint32_t locked_increment(volatile uint32_t* var)
{
    return __sync_fetch_and_add(var, 1);
}



/* Mutexe **************************************/

typedef int mutex_t;

void mutex_lock(mutex_t* mutex);
void mutex_unlock(mutex_t* mutex);

#endif //ifndef _LOCK_H_
