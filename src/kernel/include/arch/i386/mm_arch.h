/*
 * Copyright (c) 2006-2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Burkhard Weseloh.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *     This product includes software developed by the tyndur Project
 *     and its contributors.
 * 4. Neither the name of the tyndur Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _MM_ARCH_H_
#define _MM_ARCH_H_

#include <stdint.h>
#include <types.h>
#include <lock.h>
#include <page.h>
#include "multiboot.h"

#define PTE_P       0x001 // present
#define PTE_W       0x002 // writable
#define PTE_U       0x004 // user
#define PTE_PWT     0x008 // write-through
#define PTE_PCT     0x010 // cache-disable
#define PTE_A       0x020 // accessed
#define PTE_D       0x040 // dirty
#define PTE_PS      0x080 // page size

#define PTE_AVAIL1  0x200 // available for software use
#define PTE_AVAIL2  0x400 // available for software use
#define PTE_AVAIL3  0x800 // available for software use

// Kernelinterne Flagwerte, die rausmaskiert werden
#define PTE_ALLOW_NULL 0x80000000 // Null-Mappings erlauben

typedef unsigned long* page_table_t;
typedef unsigned long* page_directory_t;

typedef struct {
    uint32_t version;
    paddr_t page_directory;
    page_directory_t page_directory_virt;
    lock_t lock;
} mmc_context_t;

typedef enum { page_4K, page_4M } page_size_t;

// Die Anzahl der Page Tables, die für den Kerneladressraum benötigt werden
#define NUM_KERNEL_PAGE_TABLES \
    ((KERNEL_MEM_END >> PGDIR_SHIFT) - (KERNEL_MEM_START >> PGDIR_SHIFT))

#define MM_FLAGS_KERNEL_CODE (PTE_P | PTE_W)
#define MM_FLAGS_KERNEL_DATA (PTE_P | PTE_W)
#define MM_FLAGS_USER_CODE (PTE_P | PTE_U | PTE_W)
#define MM_FLAGS_USER_DATA (PTE_P | PTE_U | PTE_W)
#define MM_FLAGS_NO_CACHE (PTE_PCT)


#define MM_USER_START 0x40000000
#define MM_USER_END   0xFFFFFFFF

#endif
