/*  
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Mathias Gottschlag.
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

#include <stdint.h>
#include <stdbool.h>

#include "vm86.h"
#include "string.h"
#include "stdlib.h"
#include "syscall_structs.h"
#include "paging.h"
#include "intr.h"
#include "kprintf.h"
#include "kmm.h"
#include "kernel.h"
#include <ports.h>

extern int num_tasks;

// Kopie der ersten 4k RAM
struct
{
    uint16_t ivt[256][2];
    uint8_t data[3072];
} bios_data __attribute__ ((aligned (4096)));

// TODO: Support für mehrere VM86-Tasks
// Gibt evtl Probleme mit der Platzierung des Kernels.
bool vm86_task_running = false;

/**
 * Speichert BIOS-Daten, um sie den VM86-Tasks später bereitstellen zu können
 */
void save_bios_data(void)
{
    memcpy(&bios_data, 0, 4096);
}

/**
 * Liefert ein Segment:Offset-Paar aus der IVT zurück
 *
 * @param interrupt Nummer des Interrupts
 *
 * @return Pointer auf Segment/Offset
 */
uint16_t *get_ivt_entry(uint16_t interrupt)
{
    return bios_data.ivt[interrupt];
}

/**
 * Erstellt einen VM86-Task, der einen bestimmten RM-Interrupt aufruft
 *
 * @param interrupt Nummer des Interrupts
 * @param regs Struktur mit Registerinhalt (wird auch zur Rückgabe der Ergebnisse
 * benutzt.
 * @param meminfo Daten über Speicherbereiche, die in den Task gemappt werden.
 * Die erste Zahl steht für die Anzahl der Einträge, danach folgen je drei Zahlen
 * für Adresse im neuen Task, Adresse in aufrufenden Task und Größe.
 *
 * @return Taskstruktur des neuen Tasks oder Null bei Fehlern
 */

struct task * create_vm86_task(uint16_t interrupt, vm86_regs_t *regs, uint32_t *meminfo, struct task *parent)
{
    // Nur einen VM86-Task starten
    if (vm86_task_running) {
        return 0;
    }
    vm86_task_running = true;
    
    struct task* new_task = malloc(sizeof(struct task));
    new_task->pid = generate_pid();
    
    // Aufrufender Thread wird geblockt, damit später Ergebnisse zurückgegeben werden können
    if (!block_task(parent, new_task->pid)) {
        //puts("VM86: Konnte aufrufenden Task nicht blockieren!\n");
        free(new_task);
        return 0;
    }

    
    new_task->rpcs = list_create();
    new_task->io_bitmap = NULL;
    new_task->status = TS_RUNNING;
    new_task->shmids = list_create();
    new_task->shmaddresses = list_create();
    new_task->memory_used = 0;
    // Neuer Task wird nicht geblockt, da er sofort ausgeführt werden kann und sollte
    new_task->blocked_by_pid = 0;
    new_task->blocked_count = 0;
    // Info über Register/Speicher für Rückgabe von Daten aufbewahren
    new_task->vm86 = true;
    new_task->vm86_info = malloc(sizeof(vm86_info_t));
    new_task->vm86_info->regs = regs;
    new_task->vm86_info->meminfo = meminfo;
    //printf("Neuer vm86-Task mit PID %d\n", new_task->pid);
    
    // Name des Tasks setzen
    new_task->cmdline = malloc(strlen("vm86") + 1);
    memcpy((void*) (new_task->cmdline), "vm86", strlen("vm86") + 1);
    
    // Scheduling-Eigenschaften fuer den Task setzen
    new_task->schedule_ticks_left = 0;
    new_task->schedule_ticks_max = 50;
    
    // Haenge den Task an die Liste an
    // TODO Trennen der Task-Liste von der Scheduling-Liste
    new_task->next_task = first_task;
    first_task = new_task;
    if (parent->pid == 0) {
        new_task->parent_task = current_task;
    } else {
        new_task->parent_task = get_task(parent->pid);
    }
    
    // Neues Pagedirectory anlegen
    // TODO: Die erste Pagetable sollte kopiert werden oder der Kernel etwas nach
    // hinten verschoben werden.
    paddr_t phys_pagedir = (paddr_t)phys_alloc_page();
    page_directory_t pagedir = (page_directory_t) map_phys_addr(phys_pagedir, PAGE_SIZE);

    memset((void*) pagedir, 0, PAGE_SIZE);
    memcpy((void*) pagedir, kernel_page_directory, 1024);
    new_task->cr3 = pagedir;
    pagedir[0] |= 0x4;
    
    map_page_range(kernel_page_directory, (vaddr_t)0xC0000, (paddr_t)0xC0000, PTE_P | PTE_U, 0x40);
    map_page_range(kernel_page_directory, (vaddr_t)0xA0000, (paddr_t)0xA0000, PTE_W | PTE_P | PTE_U, 0x10);
    
    
    // Speicher fuer die Stacks allokieren
    // Stack im PD des neuen Tasks mappen
    paddr_t phys_kernel_stack = (paddr_t)phys_alloc_page();
    paddr_t phys_user_stack = (paddr_t)phys_alloc_page();

    // TODO: Dynamisch freien Speicher suchen, bin ich grad zu faul zu -.-
    map_page(pagedir, (vaddr_t)0x90000, phys_user_stack, PTE_W | PTE_P | PTE_U);
    new_task->user_stack_bottom = (vaddr_t) 0x90000;
    
    // Erste 4k mappen
    uint32_t *page_table = (uint32_t*)find_contiguous_kernel_pages(1);
    map_page(kernel_page_directory, page_table, (paddr_t)(pagedir[0] & ~0xFFF), PTE_P | PTE_W);
    page_table[0] = (uint32_t)&bios_data | 0x7;
    unmap_page(kernel_page_directory, page_table);
    
    // Gewünschte Speicherbereiche reservieren
    if (meminfo) {
        uint32_t infosize = meminfo[0];
        uint32_t i;
        for (i = 0; i < infosize; i++) {
            uint32_t addr = meminfo[1 + i * 3];
            uint32_t src = meminfo[1 + i * 3 + 1];
            uint32_t size = meminfo[1 + i * 3 + 2];
            paddr_t phys_mem = (paddr_t)phys_alloc_page();
            map_page(pagedir, (vaddr_t)(addr & ~0xFFF), phys_mem, PTE_W | PTE_P | PTE_U);
            memcpy((void*)addr, (void*)src, size);
        }
    }
    
    // Userstack mit Nullen füllen, um bei einem iret den Task zu beenden
    uint32_t *user_stack = (uint32_t*)find_contiguous_kernel_pages(1);
    map_page(kernel_page_directory, user_stack, phys_user_stack, PTE_P | PTE_W);
    user_stack[1023] = 0x0;
    user_stack[1022] = 0x0;
    unmap_page(kernel_page_directory, user_stack);
    
    // Den Kernelstack mappen und initialisieren
    // Der Stack wird von oben her beschrieben, daher 4K addieren 
    // (1024 * sizeof(uint32_t))
    uint32_t * kernel_stack = (uint32_t*)map_phys_addr(phys_kernel_stack, PAGE_SIZE);
    kernel_stack += 1024;

    *(--kernel_stack) = 0x00; // gs
    *(--kernel_stack) = 0x00; // fs
    *(--kernel_stack) = regs->es; // es
    *(--kernel_stack) = regs->ds; // ds
    *(--kernel_stack) = 0x9000; // ss
    *(--kernel_stack) = 0 + 4096 - 6;
    *(--kernel_stack) = 0x20202; // eflags = VM-Bit gesetzt, interrupts aktiviert und iopl = 0
    *(--kernel_stack) = get_ivt_entry(interrupt)[1]; // cs
    *(--kernel_stack) = get_ivt_entry(interrupt)[0]; // eip
    //kprintf("Interrupt: %x/%x\n", get_ivt_entry(interrupt)[1], get_ivt_entry(interrupt)[0]);

    *(--kernel_stack) = 0; // interrupt nummer
    *(--kernel_stack) = 0; // error code

    // general purpose registers
    *(--kernel_stack) = regs->ax;
    *(--kernel_stack) = regs->cx;
    *(--kernel_stack) = regs->dx;
    *(--kernel_stack) = regs->bx;
    *(--kernel_stack) = 0;
    *(--kernel_stack) = 0;
    *(--kernel_stack) = regs->si;
    *(--kernel_stack) = regs->di;

    // segment registers
    *(--kernel_stack) = 0x23;
    *(--kernel_stack) = 0x23;
    *(--kernel_stack) = 0x23;
    *(--kernel_stack) = 0x23;
    
    new_task->esp = (uint32_t) kernel_stack;
    new_task->kernel_stack = (uint32_t) kernel_stack;

    num_tasks++;
    
    //puts("vm86-Task erstellt.\n");
        
    return new_task;

}

/**
 * Handler für Exceptions im VM86-Mode
 *
 * @param esp Pointer auf Stackpointer
 *
 * @return true, wenn die Exception verarbeitet wurde, false, wenn sie nicht
 * verarbeitet werden konnte und der Task beendet werden muss.
 */
bool vm86_exception(uint32_t *esp)
{
    // TODO: Das hier ist noch lange nicht vollständig
    struct int_stack_frame *isf = *((struct int_stack_frame **)esp);
    //puts("vm86 Exception.\n");
    if (isf->interrupt_number == 13) {
        // GPF, wurde vermutlich durch einen nicht erlaubten Befehl verursacht
        uint8_t *ops = (uint8_t*)(isf->eip + (isf->cs << 4));
        if (ops[0] == 0xCD) { // int
            //kprintf("VM86: int 0x%x\n", ops[1]);
            // Derzeitige Adresse auf den Stack pushen und neue Codeadresse setzen
            uint16_t intno = ops[1];
            isf->esp -= 6;
            ((uint16_t*)(isf->esp + (isf->ss << 4)))[0] = (uint16_t)isf->eflags;
            ((uint16_t*)(isf->esp + (isf->ss << 4)))[1] = (uint16_t)isf->cs;
            ((uint16_t*)(isf->esp + (isf->ss << 4)))[2] = (uint16_t)isf->eip + 2;
            isf->eip = bios_data.ivt[intno][0];
            isf->cs = bios_data.ivt[intno][1];
            return true;
        } else if (ops[0] == 0xCF) { // iret
            //puts("VM86: iret.\n");
            // Alte Adresse von Stack holen
            isf->eip = ((uint16_t*)(isf->esp + (isf->ss << 4)))[2];
            isf->cs = ((uint16_t*)(isf->esp + (isf->ss << 4)))[1];
            isf->esp += 6;
            // Abbrechen, wenn wir das letzte iret erreicht haben
            if ((isf->eip == 0) && (isf->cs == 0)) {
                //puts("Breche VM86-Task ab.\n");
                struct task *task = current_task;
                vm86_info_t *info = task->vm86_info;
                // Speicher kopieren
                if (info->meminfo) {
                    paddr_t phys_meminfo = resolve_vaddr(task->parent_task->cr3, (uint32_t*)((uint32_t)info->meminfo & ~0xFFF));
                    uint32_t *meminfo = (uint32_t*)find_contiguous_kernel_pages(1);
                    map_page(kernel_page_directory, meminfo, phys_meminfo, PTE_P | PTE_W);
                    meminfo = (uint32_t*)(((uint32_t)meminfo) + ((uint32_t)info->meminfo & 0xFFF));
                    uint32_t i;
                    for (i = 0; i < meminfo[0]; i++) {
                        paddr_t phys_mem = resolve_vaddr(task->parent_task->cr3, (uint32_t*)((uint32_t)meminfo[1 + i * 3 + 1] & ~0xFFF));
                        uint32_t *mem = (uint32_t*)find_contiguous_kernel_pages(1);
                        map_page(kernel_page_directory, mem, phys_mem, PTE_P | PTE_W);
                        memcpy((void*)((uint32_t)mem + (meminfo[1 + i * 3 + 1] & 0xFFF)), (void*)meminfo[1 + i * 3], meminfo[1 + i * 3 + 2]);
                        unmap_page(kernel_page_directory, mem);
                        unmap_page(kernel_page_directory, (uint32_t*)((uint32_t)meminfo[1 + i * 3] & ~0xFFF));
                    }
                    unmap_page(kernel_page_directory, (uint32_t*)((uint32_t)meminfo & ~0xFFF));
                }
                // Register speichern
                paddr_t phys_regs = resolve_vaddr(task->parent_task->cr3, (uint32_t*)((uint32_t)info->regs & ~0xFFF));
                uint32_t regs = (uint32_t) map_phys_addr(phys_regs, PAGE_SIZE);
                vm86_regs_t * vm86_regs = (vm86_regs_t *)(regs + ((uint32_t)info->regs & 0xFFF));
                vm86_regs->ax = isf->eax;
                vm86_regs->bx = isf->ebx;
                vm86_regs->cx = isf->ecx;
                vm86_regs->dx = isf->edx;
                vm86_regs->si = isf->esi;
                vm86_regs->di = isf->edi;
                vm86_regs->ds = ((uint32_t*)(isf + 1))[0];
                vm86_regs->es = ((uint32_t*)(isf + 1))[1];
                free_phys_addr((vaddr_t) regs, PAGE_SIZE);
                // Speicher freigeben
                // - Stack
                unmap_page(task->cr3, (vaddr_t)0x90000);
                // - BIOS-Daten
                uint32_t *page_table = (uint32_t*)find_contiguous_kernel_pages(1);
                map_page(kernel_page_directory, page_table, (((uint32_t*)task->cr3)[0] & ~0xFFF), PTE_P | PTE_W);
                page_table[0] = 0;
                unmap_page(kernel_page_directory, page_table);
                // - BIOS
                uint32_t i;
                for (i = 0; i < 0x40; i++) {
                    unmap_page(task->cr3, (uint32_t*)(0xC0000 + i * 0x1000));
                }
                // - VGA
                for (i = 0; i < 0x10; i++) {
                    unmap_page(task->cr3, (uint32_t*)(0xA0000 + i * 0x1000));
                }
                // Task beenden
                if (!unblock_task(task->parent_task, task->pid)) {
                    panic("VM86: Konnte aufrufenden Task nicht wecken!");
                }
                schedule(esp);
                destroy_task(task);
                vm86_task_running = false;
                return true;
            }
            return true;
        } else if (ops[0] == 0x9C) { // pushf
            // EFLAGS speichern
            isf->esp -= 2;
            ((uint16_t*)(isf->esp + (isf->ss << 4)))[0] = (uint16_t)isf->eflags;
            isf->eip++;
            return true;
        } else if (ops[0] == 0x9D) { // popf
            // So tun, als würden wir die EFLAGS wiederherstellen.
            // Das hier ist wohl alles andere als korrekt, aber funzt erstmal.
            isf->esp += 2;
            isf->eip++;
            return true;
        } else if (ops[0] == 0xEF) { // outw
            outw(isf->edx, isf->eax);
            isf->eip++;
            return true;
        } else if (ops[0] == 0xEE) { // outb
            outb(isf->edx, isf->eax);
            isf->eip++;
            return true;
        } else if (ops[0] == 0xED) { // inw
            isf->eax = inb(isf->edx);
            isf->eip++;
            return true;
        } else if (ops[0] == 0xEC) { // inb
            isf->eax = (isf->eax & 0xFF00) + inb(isf->edx);
            isf->eip++;
            return true;
        } else if (ops[0] == 0xFA) { // sti
            isf->eip++;
            return true;
        } else if (ops[0] == 0xFB) { // cli
            isf->eip++;
            return true;
        } else if (ops[0] == 0x66) { // o32
            // TODO
            isf->eip++;
            return true;
        } else if (ops[0] == 0x67) { // a32
            // TODO
            isf->eip++;
            return true;
        } else {
            abort_task("VM86: Unbekannter Opcode: %x\n", ops[0]);
            return false;
        }
    } else {
        return false;
    }
}

