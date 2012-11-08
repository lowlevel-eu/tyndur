/*
 * Copyright (c) 2006-2011 The tyndur Project. All rights reserved.
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

#ifndef _PAGE_H_
#define _PAGE_H_

#define PAGE_SHIFT 12
#define PAGE_SIZE 0x1000

// AMD64 beschränkt den physischen Adressraum auf 52 Bit
#define PAGE_MASK 0x000ffffffffff000L

#define PAGE_MAP_LENGTH 512
#define PAGE_MAP_SHIFT 39
#define PAGE_MAP_INDEX(addr) ((uintptr_t)(addr) >> PAGE_MAP_SHIFT)

#define PAGE_DIR_PTR_TABLE_LENGTH 512
#define PAGE_DIR_PTR_TABLE_SHIFT 30
#define PAGE_DIR_PTR_TABLE_INDEX(addr) \
  (((uintptr_t)(addr) >> PAGE_DIR_PTR_TABLE_SHIFT) & (PAGE_DIR_PTR_TABLE_LENGTH - 1))

#define PAGE_DIRECTORY_LENGTH 512
#define PAGE_DIRECTORY_SHIFT 21
#define PAGE_DIRECTORY_INDEX(addr) (              \
    ((uintptr_t)(addr) >> PAGE_DIRECTORY_SHIFT) & \
    (PAGE_DIRECTORY_LENGTH - 1))

#define PAGE_TABLE_LENGTH 512
#define PAGE_TABLE_SHIFT 12
#define PAGE_TABLE_INDEX(addr) (((uintptr_t)(addr) >> PAGE_TABLE_SHIFT) & \
    (PAGE_TABLE_LENGTH - 1))

// Die Adresse, an der der Kernel-Adressraum beginnt
#define KERNEL_MEM_START    0x00000000
#define KERNEL_MEM_END      0x40000000

// Die Adresse, an der der Userspace-Adressraum beginnt
#define USER_MEM_START (0x1L << PAGE_MAP_SHIFT)
#define USER_MEM_END   (0x100L << PAGE_MAP_SHIFT)


// Rundet eine Adresse auf das kleinste Vielfache von PAGE_SIZE > n auf
#define PAGE_ALIGN_ROUND_UP(n) (((n) + ~PAGE_MASK) & PAGE_MASK)

// Rundet eine Adresse auf das größte Vielfache von PAGE_SIZE < n ab
#define PAGE_ALIGN_ROUND_DOWN(n) ((n) & PAGE_MASK)

// Die Anzahl der Pages, die von n Bytes belegt werden.
#define NUM_PAGES(n) (PAGE_ALIGN_ROUND_UP(n) >> PAGE_SHIFT)

// Die Anzahl der Page Maps die für n Bytes benötigt werden.
#define NUM_PAGE_MAPS(n) (((n) + (1L << PAGE_MAP_SHIFT) - 1) >> PAGE_MAP_SHIFT)

#endif
