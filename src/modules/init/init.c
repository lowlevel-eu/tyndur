/*  
 * Copyright (c) 2006 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
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
#include "types.h"
#include "syscall.h"
#include "rpc.h"
#include "collections.h"
#include "stdlib.h"
#include "stdio.h"

struct service_s
{
    pid_t   pid;
    char*   name;
};

void rpc_service_register(dword pid, size_t data_size, void* data);
void rpc_service_get(dword pid, size_t data_size, void* data);
void rpc_service_shutdown(dword pid, size_t data_size, void* data);
void rpc_service_startup(dword pid, size_t data_size, void* data);

struct service_s* get_service_by_name(const char* name);
struct service_s* get_service_by_pid(pid_t pid);
list_t* service_list;


void _start(dword modulelist, dword a)
{
    init_messaging();
    init_memory_manager();
    
    printf("0x%x 0x%x", modulelist, a);
    //Den Prozess nun fuer externe Anfragen freigeben
    
    
    //Liste fuer die Prozesse erstellen
    service_list = list_create();
    
    
    puts("\n\033[1;33mLOST-Init V0.0.1\033[0;37m");
    register_message_handler("SERV_REG", &rpc_service_register);
    register_message_handler("SERV_GET", &rpc_service_get);
    register_message_handler("SERV_SHD", &rpc_service_shutdown);
    register_message_handler("SERV_STA", &rpc_service_startup);
    
    unblock_process(0);
    
    while(1) yield();
}


/**
 * Einen Dienst registrieren
 */
void rpc_service_register(pid_t pid, size_t data_size, void* data)
{
    struct service_s* new_service = malloc(sizeof(struct service_s));
    service_list = list_push(service_list, (void*)new_service);
    
    new_service->pid = pid;
    
    //name kopieren
    new_service->name = malloc(strlen((char*)data) + 10);
    strncpy(new_service->name, (char*)data, strlen((char*)data) + 1);
    
    
    //printf("service_register '%s' = %d\n", new_service->name, new_service->pid);
}


/**
 * Einen Dienst anhand seines Namens finden
 */
void rpc_service_get(pid_t pid, size_t data_size, void* data)
{
    int i  = 0;
    struct service_s* service = list_get_element_at(service_list, i++);
    
    while(service != NULL)
    {
        if(strcmp(service->name, (const char*) data) == 0)
            printf("service_get '%s' = %d\n", service->name, service->pid);
        service = list_get_element_at(service_list, i++);
    }
    
}


/**
 * Wird vom kernel gesendet, wenn ein Prozess beendet wird.
 */
void rpc_service_shutdown(pid_t pid, size_t data_size, void* data)
{
    pid_t* _pid = (pid_t*)data;
    
    printf("service_shutdown %d\n", *_pid);
}

void rpc_service_startup(dword pid, size_t data_size, void* data)
{
    pid_t* _pid = (pid_t*)data;
    
    printf("service_shutdown %d\n", *_pid);
}


/*

void service_get(dword pid, dword data_size, void* data)
{
    puts("[RPCRECV] Hello world (aus TEST2)!");
    puts((char*)data);
}

bool service_next()
{
    if (curr == NULL && first == NULL)
    {
        return FALSE;    
    }
    if (curr == NULL && first != NULL)
    {
        curr = first;
        return TRUE;
    } 
    else 
    {
        if ((*curr).next != NULL)
        {
            curr = (*curr).next;
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
}

void service_none()
{
    curr = NULL;
}



/*
#include "elf32.h"
#include "elf_common.h"
#define ELF_MAGIC 0x464C457F


                 extern void* hello_start;
         void load_single_module(Elf32_Ehdr* elf_header);
         void create_process_test()
{
    puts("Erstelle Prozess....");
    load_single_module((Elf32_Ehdr*)&hello_start);
    puts("[ OK ]\n");

}


void load_single_module(Elf32_Ehdr* elf_header) 
{
    Elf32_Phdr * program_header;
    dword i;

    pid_t pid = 0;
    if (elf_header->e_magic == ELF_MAGIC)
    {
        program_header = (Elf32_Phdr *) ((dword) elf_header + elf_header->e_phoff);
        
        for (i = 0; i < elf_header->e_phnum; i++, program_header++)
        {
            if (program_header->p_type == 1)
            {
                if ((elf_header->e_entry >= program_header->p_vaddr) && (elf_header->e_entry <= program_header->p_vaddr + program_header->p_memsz))
                {
                    if(pid != 0)
                    {
                        unblock_process(pid);
                        pid = 0;
                    }
                    pid = create_process(elf_header->e_entry, 0);
                }
                else if (pid == 0)
                {
                    // FIXME
                    printf("Kein Task: phdr->vaddr = %x, ehdr->entry = %x\nDas Modul ist entweder beschaedigt oder wird von diesem Loader nicht unterstuetzt (Entry Point muss im ersten Program Header liegen)\n", program_header->p_vaddr, elf_header->e_entry);
                    continue;
                }

                init_child_page (pid, (void*) &(program_header->p_vaddr), (void*)((dword) elf_header + program_header->p_offset), program_header->p_memsz);
            }
        }
      
    }
    else
    {
        printf("ELF-Magic ungueltig (0x%x)\n",elf_header->e_magic);
    }

    
    if(pid != 0)
    {
        unblock_process(pid);
    }
}
*/
