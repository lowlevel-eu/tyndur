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
#include <lock.h>
#include <page.h>
#include <types.h>

#include "multiboot.h"

// TODO: Redundanz oder semantischer Unterschied
#define MM_USER_START USER_MEM_START
#define MM_USER_END   USER_MEM_END

#define USER_STACK_START USER_MEM_END
#define USER_STACK_SIZE 0x1000

#define ADDRESS_SIGN_EXTEND (0xFFFFL << 48)

// Der gesamte physische Speicher wird 1:1 an diese Adresse gemapt
#define MAPPED_PHYS_MEM_START ((0x1FFL << PAGE_MAP_SHIFT) | ADDRESS_SIGN_EXTEND)
#define MAPPED_PHYS_MEM_GET(addr) ((vaddr_t) ((uintptr_t)MAPPED_PHYS_MEM_START \
    | (uintptr_t)(addr)))

typedef uint64_t mmc_pml4_entry_t;
typedef uint64_t mmc_pdpt_entry_t;
typedef uint64_t mmc_pd_entry_t;
typedef uint64_t mmc_pt_entry_t;
typedef struct {
    uint_least16_t version;
    lock_t lock;
    paddr_t pml4;
} mmc_context_t;

#define PAGE_FLAGS_PRESENT  (1 << 0)
#define PAGE_FLAGS_WRITABLE (1 << 1)
#define PAGE_FLAGS_USER     (1 << 2)
#define PAGE_FLAGS_NO_CACHE (1 << 4)
#define PAGE_FLAGS_BIG      (1 << 7)
#define PAGE_FLAGS_GLOBAL   (1 << 8)
#define PAGE_FLAGS_NO_EXEC  (1 << 9)

#define PAGE_FLAGS_INTERNAL_NX (1L << 63)

#define MM_FLAGS_KERNEL_CODE (PAGE_FLAGS_PRESENT | PAGE_FLAGS_GLOBAL)
#define MM_FLAGS_KERNEL_DATA (PAGE_FLAGS_PRESENT | PAGE_FLAGS_WRITABLE | \
    PAGE_FLAGS_GLOBAL | PAGE_FLAGS_NO_EXEC)
#define MM_FLAGS_USER_CODE (PAGE_FLAGS_PRESENT | PAGE_FLAGS_USER)
#define MM_FLAGS_USER_DATA (PAGE_FLAGS_PRESENT | PAGE_FLAGS_USER | \
    PAGE_FLAGS_WRITABLE | PAGE_FLAGS_NO_EXEC)

#endif
