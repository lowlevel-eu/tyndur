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

#include <stdint.h>
#include <stdbool.h>
#include <lock.h>

/**
 * Einen Lock sperren. Falls der Lock schon gesperrt ist, wîrd gewartet, bis
 * er gesperrt werden kann.
 *
 * @param l Pointer zum Lock
 */
void lock(lock_t* l)
{
    asm("movb $1, %%cl;"
        "lock_loop: xorb %%al, %%al;"
        "lock cmpxchgb %%cl, (%0);"
        "jnz lock_loop;" : : "D" (l) : "eax", "ecx");
}

/**
 * Den Status des Locks feststellen.
 *
 * @return true, wenn gesperrt, false sonst.
 */
bool locked(lock_t* l)
{
    return *l;
}

/**
 * Einen Lock entsperren.
 *
 * @param l Pointer zum Lock
 */
void unlock(lock_t* l)
{
    *l = 0;
}

/**
 * Warten bis ein Lock nicht mehr gesperrt ist.
 *
 * @param l Pointer zum Lock
 */
void lock_wait(lock_t* l)
{
    while (locked(l)) asm("nop");
}
