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
#define MODULE_INIT

#include "types.h"
#include "syscall.h"
#include "rpc.h"
#include "collections.h"
#include "stdlib.h"
#include "stdio.h"
#include "elf32.h"
#include "elf_common.h"
#include "io.h"


#define ELF_MAGIC 0x464C457F
#define PAGE_OFFSET(a) (a % 0x1000)
#define PAGE_SHIFT 12
#define PAGE_SIZE (1 << PAGE_SHIFT)
#define PAGE_MASK (~(PAGE_SIZE - 1))
#define PAGE_ALIGN_ROUND_UP(n) (((n) + ~PAGE_MASK) & PAGE_MASK)
#define PAGE_ALIGN_ROUND_DOWN(n) ((n) & PAGE_MASK)


struct service_s
{
    pid_t   pid;
    char*   name;
};

struct module_list_s
{
    dword   count;
    dword  modules[];
};



void rpc_service_register(pid_t pid, dword correlation_id, size_t data_size, void* data);
void rpc_service_get(pid_t pid, dword correlation_id, size_t data_size, void* data);
void rpc_service_shutdown(pid_t pid, dword correlation_id, size_t data_size, void* data);
void rpc_service_exit(pid_t pid, dword correlation_id, size_t data_size, void* data);

void load_modules(struct module_list_s* modules);
void load_single_module(Elf32_Ehdr* elf_header);
extern void init_sync_messages();

struct service_s* get_service_by_name(const char* name);
struct service_s* get_service_by_pid(pid_t pid);


list_t* service_list;


        
void _start(dword modules)
{
    init_messaging();
    init_memory_manager();
    init_sync_messages(); 
    //Liste fuer die Prozesse erstellen
    service_list = list_create();
    
    
    puts("\n\033[1;33mLOST-Init V0.0.1 pre-alpha\033[0;37m\n");
    register_message_handler("SERV_REG", &rpc_service_register);
    register_message_handler("SERV_GET", &rpc_service_get);
    register_message_handler("SERV_SHD", &rpc_service_shutdown);
    register_message_handler("SERV_EXI", &rpc_service_exit);

    io_init();

    load_modules((struct module_list_s*) modules);
      
    while(1) yield();
}


struct service_s* get_service_by_name(const char* name)
{
    int i = 0;
    struct service_s* service;

    while((service = list_get_element_at(service_list, i++)))
    {
        if(strcmp(service->name, name) == 0)
        {
            break;
        }
    }

    return service;
}

struct service_s* get_service_by_pid(pid_t pid)
{
    int i = 0;
    struct service_s* service;

    while((service = list_get_element_at(service_list, i++)))
    {
        if(service->pid == pid)
        {
            break;
        }
    }

    return service;
}


/**
 * Einen Dienst registrieren
 */
void rpc_service_register(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    struct service_s* new_service = malloc(sizeof(struct service_s));
    service_list = list_push(service_list, (void*)new_service);
    
    new_service->pid = pid;
    
    //name kopieren
    new_service->name = malloc(strlen((char*)data) + 10);
    strncpy(new_service->name, (char*)data, strlen((char*)data) + 1);
    
    
    printf("[  INIT  ] service_register '%s' = %d\n", new_service->name, new_service->pid);
}


/**
 * Einen Dienst anhand seines Namens finden
 */
void rpc_service_get(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    int i;
    struct service_s* service;
    
    //5 Versuche einen Task zu finden
    for(i = 0; i < 5; i++)
    {
        service = get_service_by_name((const char*) data);
        
        //Wenn der service gefunden wurde => return pid
        if(service != NULL)
        {
            //printf("got pid of '%s'   corr_id:%d\n", (char*)data, correlation_id);
            rpc_send_dword_response(pid, correlation_id, service->pid);
            return;
        }
        
        yield();
    }

    rpc_send_dword_response(pid, correlation_id, 0);
    
}


/**
 * Wird vom kernel gesendet, wenn ein Prozess beendet wird.
 */
void rpc_service_shutdown(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    pid_t* _pid = (pid_t*)data;
    
    //printf("[  INIT  ] service_shutdown %d\n", *_pid);
}



/**
 * Wird vom kernel gesendet, wenn ein Prozess beendet wird.
 */
void rpc_service_exit(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    int* result = (int*)data;
    struct service_s* service;
    int i;
    
    for(i = 0; (service = list_get_element_at(service_list, i)); i++)
    {
        if(service->pid == pid)
        {
            break;
        }
    }
    
    //Wenn der service gefunden wurde
    if(service != NULL)
    {
        //printf("[  INIT  ] service_exit '%s'  result:%d\n", service->name, *result);
        list_remove(service_list, i);
        return;
    }
}


void load_modules(struct module_list_s* modules) 
{ 
    int i = 0; 
    for(i = 0; i < modules->count; i++) 
    { 
        load_single_module((Elf32_Ehdr*) modules->modules[i]); 
    } 
}

/**
 * Der ELF-Loader (noch Total Kaputt)
 */
void load_single_module(Elf32_Ehdr* elf_header) 
{
    Elf32_Phdr* program_header;
    dword i, j;
    pid_t pid;
    
    //Liste mit den Programm-Headern. Wird benutzt um auch programme laden zu koennen, die nicht
    list_t* pheader_list = list_create();
    
    pid = 0;
    if (elf_header->e_magic == ELF_MAGIC)
    {
        program_header = (Elf32_Phdr *) ((dword) elf_header + elf_header->e_phoff);
        
        //Zuerst den Bereich mit dem entry-point suchen
        for (j = 0; j < 2; j++)
        {
            for (i = 0; i < elf_header->e_phnum; i++)
            {
                // Im ersten durchgang alles ausser dem Bereich mit dem entry-Point pushe, damit er zu oberst ist
                if((j == 1) && ((elf_header->e_entry >= program_header[i].p_vaddr) && (elf_header->e_entry <= program_header[i].p_vaddr + program_header[i].p_memsz)))
                {
                    pheader_list = list_push(pheader_list, (void*)&(program_header[i]));
                }
                else if((j == 0) && !((elf_header->e_entry >= program_header[i].p_vaddr) && (elf_header->e_entry <= program_header[i].p_vaddr + program_header[i].p_memsz)))
                {
                    pheader_list = list_push(pheader_list, (void*)&(program_header[i]));
                }
            }
        }

        while(list_is_empty(pheader_list) == FALSE)
        {
            //Den naechsten Bereich aus der Liste holen
            program_header = (Elf32_Phdr*) list_pop(pheader_list);
            
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
                    //printf("[  INIT  ] Erstelle Prozess PID=%d\n", pid);
                }
                else if (pid == 0)
                {
                    // FIXME
                    printf("Kein Task: phdr->vaddr = %x, ehdr->entry = %x\nDas Modul ist entweder beschaedigt oder wird von diesem Loader nicht unterstuetzt (Kein Entry Point wurde gefunden :-S)\n", program_header->p_vaddr, elf_header->e_entry);
                    continue;
                }
                
                //Falls p_memsz > p_filesz muss der Bereich dazwischen allokiert und mit 0 gefuellt werden
                
                if(program_header->p_filesz < program_header->p_memsz)
                {
                    //Den Rest der Pages die fuer das Dateiimage Benutzt wurden mit 0 fuellen
                    if (program_header->p_filesz) {
                        memset((void*) ((dword) elf_header + program_header->p_offset + program_header->p_filesz), 0, 0x1000 - (program_header->p_filesz % 0x1000));
                    }
                    
                    //speicher fuer den Bereich, der zwar im Speicher vorahnden sein soll, in der Datei aber nicht enthalten ist, allokieren und mit 0 fuellen
                    size_t null_pages_size = PAGE_ALIGN_ROUND_UP(program_header->p_memsz - program_header->p_filesz) ;

                    if(null_pages_size != 0)
                    {
                        void* null_pages = mem_allocate(null_pages_size, 0);
                        
                        memset(null_pages, 0, null_pages_size);
                    
                        //Die leeren Pages in den neuen Prozess mapen
                        init_child_page (pid, (void*) PAGE_ALIGN_ROUND_UP(PAGE_ALIGN_ROUND_DOWN(program_header->p_vaddr) + program_header->p_filesz), null_pages, null_pages_size);
                    }
                }
                
                
                //Den Bereich in den Adressraum des Prozesses mapen
                init_child_page (pid, (void*) (program_header->p_vaddr), (void*)((dword) elf_header + program_header->p_offset), program_header->p_filesz);
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

