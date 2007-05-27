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

#ifndef _MM_H_
#define _MM_H_

#define PAGE_SHIFT 12
#define PAGE_SIZE (1 << PAGE_SHIFT)
#define PAGE_MASK (~(PAGE_SIZE - 1))

#define PAGE_DIRECTORY_LENGTH 1024
#define PAGE_TABLE_LENGTH 1024


// Die Anzahl der Pages, die von n Bytes belegt werden.
#define NUM_PAGES(n) ((((n) + ~PAGE_MASK) & PAGE_MASK) / PAGE_SIZE)

// Rundet eine Adresse auf das kleinste Vielfache von PAGE_SIZE > n auf
#define PAGE_ALIGN_ROUND_UP(n) (((n) + ~PAGE_MASK) & PAGE_MASK)

// Rundet eine Adresse auf das größte Vielfache von PAGE_SIZE < n ab
#define PAGE_ALIGN_ROUND_DOWN(n) ((n) & PAGE_MASK)

#define PGDIR_SHIFT 22

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

typedef unsigned long * page_directory_t;
typedef unsigned long * page_table_t;

typedef enum { page_4K, page_4M } page_size_t;

// Die Adresse, an der der Kernel-Adressraum beginnt
#define KERNEL_MEM_START    0x00000000
#define KERNEL_MEM_END      0x40000000

#define PAGETABLES_MEM_START 0x3fc00000
#define PAGETABLES_MEM_END   0x40000000

#define USER_MEM_START 0x40000000
#define USER_MEM_END   0xffffffff

// Die Anzahl der Page Tables, die für den Kerneladressraum benötigt werden
#define NUM_KERNEL_PAGE_TABLES \
    ((KERNEL_MEM_END >> PGDIR_SHIFT) - (KERNEL_MEM_START >> PGDIR_SHIFT))



void pmm_init(struct multiboot_info* multiboot_info);

size_t pmm_count_free();
size_t pmm_count_pages();

paddr_t pmm_alloc(paddr_t lower_limit, paddr_t upper_limit, size_t num);
void pmm_free(paddr_t start, size_t count);

#endif
