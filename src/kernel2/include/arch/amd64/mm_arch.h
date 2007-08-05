/*
 * Copyright (c) 2006-2007 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
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
 *     This product includes software developed by the LOST Project
 *     and its contributors.
 * 4. Neither the name of the LOST Project nor the names of its
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

#include <types.h>
#include <stdint.h>

#include "multiboot.h"

#define PAGE_SHIFT 12
#define PAGE_SIZE (1 << PAGE_SHIFT)
#define PAGE_MASK (~(PAGE_SIZE - 1))

#define PAGE_MAP_LENGTH 512
#define PAGE_MAP_SHIFT 39
#define PAGE_MAP_INDEX(addr) ((uintptr_t)addr >> PAGE_MAP_SHIFT)

#define PAGE_DIR_PTR_TABLE_LENGTH 512
#define PAGE_DIR_PTR_TABLE_SHIFT 30
#define PAGE_DIR_PTR_TABLE_INDEX(addr) (((uintptr_t)addr >> \
    PAGE_DIR_PTR_TABLE_SHIFT) & (PAGE_DIR_PTR_TABLE_LENGTH - 1))

#define PAGE_DIRECTORY_LENGTH 512
#define PAGE_DIRECTORY_SHIFT 21
#define PAGE_DIRECTORY_INDEX(addr) (((uintptr_t)addr >> PAGE_DIRECTORY_SHIFT) & \
    (PAGE_DIRECTORY_LENGTH - 1))

#define PAGE_TABLE_LENGTH 512
#define PAGE_TABLE_SHIFT 12
#define PAGE_TABLE_INDEX(addr) (((uintptr_t)addr >> PAGE_TABLE_SHIFT) & \
    (PAGE_TABLE_LENGTH - 1))


// Die Adresse, an der der Kernel-Adressraum beginnt
#define KERNEL_MEM_START    0x00000000
#define KERNEL_MEM_END      0x40000000

// Die Adresse, an der der Userspace-Adressraum beginnt
#define USER_MEM_START (0x1L << PAGE_MAP_SHIFT)
#define USER_MEM_END   ((0x100L << PAGE_MAP_SHIFT) - 1)
 
#define USER_STACK_START USER_MEM_END
#define USER_STACK_SIZE 0x1000

// Der gesamte Physikalische Speicher wird 1:1 an diese Adresse gemapt
#define ADDRESS_SIGN_EXTEND (0xFFFFL << 48)
#define MAPPED_PHYS_MEM_START ((0x1FFL << PAGE_MAP_SHIFT) | ADDRESS_SIGN_EXTEND)
#define MAPPED_PHYS_MEM_GET(addr) ((vaddr_t) ((uintptr_t)MAPPED_PHYS_MEM_START \
    | (uintptr_t)addr))

typedef qword mmc_pm_entry_t;
typedef qword mmc_pdpt_entry_t;
typedef qword mmc_pd_entry_t;
typedef qword mmc_pt_entry_t;
typedef mmc_pm_entry_t* mmc_context_t;

#define PAGE_FLAGS_PRESENT  (1 << 0)
#define PAGE_FLAGS_WRITABLE (1 << 1)
#define PAGE_FLAGS_USER     (1 << 2)
#define PAGE_FLAGS_NO_CACHE (1 << 4)
#define PAGE_FLAGS_BIG      (1 << 7)
#define PAGE_FLAGS_GLOBAL   (1 << 8)
#define PAGE_FLAGS_NO_EXEC  (1 << 9)

#define PAGE_FLAGS_INTERNAL_NX (1L << 63)

// FIXME: Der Code sollte nicht beschreibbar sein.
#define MM_FLAGS_KERNEL_CODE (PAGE_FLAGS_PRESENT | PAGE_FLAGS_GLOBAL | \
    PAGE_FLAGS_WRITABLE)
#define MM_FLAGS_KERNEL_DATA (PAGE_FLAGS_PRESENT | PAGE_FLAGS_WRITABLE | \
    PAGE_FLAGS_GLOBAL | PAGE_FLAGS_NO_EXEC)
#define MM_FLAGS_USER_CODE (PAGE_FLAGS_PRESENT | PAGE_FLAGS_USER)
#define MM_FLAGS_USER_DATA (PAGE_FLAGS_PRESENT | PAGE_FLAGS_USER | \
    PAGE_FLAGS_WRITABLE | PAGE_FLAGS_NO_EXEC)

// Die Anzahl der Pages, die von n Bytes belegt werden.
#define NUM_PAGES(n) ((((n) + ~PAGE_MASK) & PAGE_MASK) / PAGE_SIZE)

// Rundet eine Adresse auf das kleinste Vielfache von PAGE_SIZE > n auf
#define PAGE_ALIGN_ROUND_UP(n) (((n) + ~PAGE_MASK) & PAGE_MASK)

// Rundet eine Adresse auf das groesste Vielfache von PAGE_SIZE < n ab
#define PAGE_ALIGN_ROUND_DOWN(n) ((n) & PAGE_MASK)



#endif
