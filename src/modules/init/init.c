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
#include "dir.h"
#include "sleep.h"
#include "zlib.h"

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

void rpc_loadelf(pid_t pid, dword correlation_id, size_t data_size, void* data);

void load_modules(struct module_list_s* modules);
bool load_single_module(Elf32_Ehdr* elf_header, char* args, pid_t parent_pid);
bool start_program(char* path, char* args, pid_t parent_pid);
extern void init_sync_messages();

struct service_s* get_service_by_name(const char* name);
struct service_s* get_service_by_pid(pid_t pid);

extern void init_envvars();

list_t* service_list;

        
void _start(dword modules)
{
    init_messaging();
    init_memory_manager();
    init_sync_messages(); 
    //Liste fuer die Prozesse erstellen
    service_list = list_create();
    init_envvars();
    //setenv("CWD", "ide:/hd1a|fat:/", 1);
    setenv("CWD", "floppy:/devices/fd0|fat:/", 1);
       
    
    puts("\n\033[1;33mLOST-Init $Rev$ \033[0;37m\n");
    register_message_handler("SERV_REG", &rpc_service_register);
    register_message_handler("SERV_GET", &rpc_service_get);
    register_message_handler("SERV_SHD", &rpc_service_shutdown);
    register_message_handler("SERV_EXI", &rpc_service_exit);
    
    register_message_handler("LOADELF ", &rpc_loadelf);

    io_init();
    


    load_modules((struct module_list_s*) modules);
    
    msleep(4000);

    char* cmdline = get_cmdline();
    char* boot = strstr(cmdline, "boot=");
    if(boot != NULL)
    {
        boot += 5;
        char* init_config_name = "etc/init.cfg";
        char* init_config_path = malloc(strlen(init_config_name) + strlen(boot) + 1);
        memcpy(init_config_path, boot, strlen(boot));
        memcpy((void*)((dword) init_config_path + strlen(boot)), init_config_name, strlen(init_config_name) + 1);
        
        //printf("cmdline: \"%s\"\n", init_config_path);
        directory_close(directory_open(boot));


        FILE* config_file = fopen(init_config_path, "r");
        fclose(config_file);

        config_file = fopen(init_config_path, "r");
        if(config_file == NULL)
        {
            puts("Fehler beim Oeffnen der Konfigurationsdatei!");
        }
        else
        {
            //Groesse der Datei holen
            fseek(config_file, 0, SEEK_END);
            size_t size = ftell(config_file);

            //Buffer allokieren
            char* config_file_content = malloc(size+1);
            
            //Datei auslesen
            fseek(config_file, 0, SEEK_SET);
            fread(config_file_content, 1, 4096, config_file);
            fclose(config_file);
            config_file_content[size] = 0;

            //Datei zeilenweise auswerten
            char* config_line = config_file_content;            
            int i;
            for(i = 0; i <= size; i++)
            {
                if((config_file_content[i] == '\n') || (config_file_content[i] == '\0'))
                {
                    config_file_content[i] = '\0';
                    

                    if(config_line == strstr(config_line, "start "))
                    {
                        char* module_path = config_line + 6;
                        char* module_args = strstr(module_path, " ");
                        
                        //Eventuelle Argumente zuschneiden
                        if(module_args == NULL)
                        {
                            module_args = "";
                        }
                        else
                        {
                            module_args[0] = '\0';
                            module_args++;
                        }
                        
                        char* module_path_comp = malloc(strlen(boot) + strlen(module_path) + 1);
                        memcpy(module_path_comp, boot, strlen(boot));
                        memcpy((void*)((dword)module_path_comp + strlen(boot)), module_path, strlen(module_path) + 1);
                        
                        //printf("name:'%s',  args:'%s'\n", module_path_comp, module_args);
                        if(start_program(module_path_comp, module_args, 0) != TRUE)
                        {
                            printf("Fehler beim laden von '%s'!\n", module_path_comp);
                        }
                        
                        free(module_path_comp);
                    }
                    
                    config_line = &(config_file_content[i + 1]);
                    config_file_content[i] = '\n';
                }
            }

            free(config_file_content);
        }

        free(init_config_path);
    }
    //printf("cmdline: %x \"%s\"\n", (dword) cmdline, cmdline);
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
    
    new_service->pid = pid;
    
    // Name kopieren
    data_size = strnlen((char*) data, data_size);
    new_service->name = malloc(data_size + 1);
    strncpy(new_service->name, (char*)data, data_size);
    new_service->name[data_size] = '\0';
   
    // Jetzt erst eintragen, damit die Zeit im kritischen Abschnitt
    // mˆglichst kurz bleibt.
    p();
    service_list = list_push(service_list, (void*)new_service);
    v();
    
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
        p();
        service = get_service_by_name((const char*) data);
        v();
        
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
    //pid_t* _pid = (pid_t*)data;
    
    //printf("[  INIT  ] service_shutdown %d\n", *_pid);
}



/**
 * Wird vom kernel gesendet, wenn ein Prozess beendet wird.
 */
void rpc_service_exit(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    //int* result = (int*)data;
    struct service_s* service;
    int i;
   
    p();
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
    }
    v();
}


void load_modules(struct module_list_s* modules) 
{ 
    int i = 0; 
    for(i = 0; i < modules->count; i++) 
    { 
        load_single_module((Elf32_Ehdr*) modules->modules[i], "", 0); 
        msleep(50); // Damit die Meldungen nicht durcheinandergeraten
    } 
}

/**
 * Der ELF-Loader (noch Total Kaputt)
 */
bool load_single_module(Elf32_Ehdr* elf_header, char* args, pid_t parent_pid) 
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
                    pid = create_process(elf_header->e_entry, 0, args, parent_pid);
                    //printf("[  INIT  ] Erstelle Prozess PID=%d  entry=0x%08x\n", pid, elf_header->e_entry);
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
                
                
                //Den Bereich in den Adressraum des Prozesses mappen.
                init_child_page (pid, (void*) (program_header->p_vaddr), (void*)(PAGE_ALIGN_ROUND_DOWN((dword) elf_header + program_header->p_offset)), PAGE_ALIGN_ROUND_UP(program_header->p_filesz));
            }
        }
      
    }
    else
    {
        printf("ELF-Magic ungueltig (0x%x)\n",elf_header->e_magic);
        return FALSE;
    }

    
    if(pid != 0)
    {
        unblock_process(pid);
    }

    return TRUE;
}

size_t ainflate(void* src, size_t len, void** dest_ptr)
{
    int ret = 0;
    z_stream stream;

    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;

    //Quellpuffer Gr√∂sse
    stream.avail_in = len;
    //Pointer auf Quellpuffer
    stream.next_in = src;

    //Aufbau des GZip-Headers auf http://www.gzip.org/zlib/rfc-gzip.html

    //Testen ob die Datei wirklich GZip komprimiert ist
    byte* src_buf = (byte*) src;
    if((src_buf[0] != 0x1f) || (src_buf[1] != 0x8b))
    {
        puts("[  INIT  ] Datei enthaelt keinen korrekten GZip-Header");
        return 0;
    }
    
    //Beinhaltet Offset zu den Daten
    int i = 10;

    //Extra-Length dazuzaehlen (falls vorhanden)
    if((src_buf[3] & (1 << 2)) != 0) i += *((word*) (src_buf + 10));
    //Laenge des Dateinamens bestimmen (falls vorhanden)
    if((src_buf[3] & (1 << 3)) != 0) { for( ; src_buf[i] != 0; i++); i++; }
    //Laenge des Kommentars dazuzaehlen (falls vorhanden)
    if((src_buf[3] & (1 << 4)) != 0) { for( ; src_buf[i] != 0; i++); i++; }
    //2 Byte fuer CRC dazuzaehlen (falls vorhanden)
    if((src_buf[3] & (1 << 1)) != 0) i += 2;

    stream.next_in = (byte*) ((dword) src + i);
    stream.avail_in = len - i - 8;

    stream.avail_out = *((dword*) (src_buf + len - 4));
    stream.next_out = mem_allocate(stream.avail_out, 0);
    *dest_ptr = stream.next_out;


    //Auspacken vorbereiter
    ret = inflateInit2(&stream, -15);
    if(ret == Z_OK)
    {
        //Endlich kommen wir zum eigentlichen Dekomprimieren
        ret = inflate(&stream, Z_SYNC_FLUSH);
    }

    if(ret != Z_STREAM_END)
    {
        if(stream.msg == NULL)
        {
            printf("[  INIT  ] zlib Fehler: %d\n", ret);
        }
        else
        {
            printf("[  INIT  ] zlib Fehler: %d  '%s'\n", ret, stream.msg);
        }
        ret = 0;
    }
    else
    {
        ret = stream.total_out;
    }
    inflateEnd(&stream);
    return ret;
}

bool start_program(char* path, char* args, pid_t parent_pid)
{
    FILE* f = fopen(path, "r");
    
    if(f == NULL)
    {
        return FALSE;
    }

    void* buffer = NULL;
    long size = 0;
    int i;
    
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    size_t pages = size / 4096;

    if(size % 4096 != 0)
    {
        pages++;
    }
    
    byte* buffer_ptr;
    
    word gzip_id;
    fread(&gzip_id, 1, sizeof(word), f);
    fseek(f, 0, SEEK_SET);
    if(gzip_id == 0x8b1f)
    {
        byte* in_buffer = malloc(pages * 4096);
        buffer_ptr = in_buffer;
        for(i = 0; i < pages; i++)
        {
            buffer_ptr += fread(buffer_ptr, 1, 4096, f);
        }
        
        size_t length = ainflate(in_buffer, size, &buffer);
        free(in_buffer);
        if(length == 0)
        {
            return FALSE;
        }
    }
    else
    {
        buffer = mem_allocate(pages * 4096, 0);
    
        byte* buffer_ptr = buffer;
        for(i = 0; i < pages; i++)
        {
            buffer_ptr += fread(buffer_ptr, 1, 4096, f);
        }
    }
    fclose(f);
    return load_single_module((Elf32_Ehdr*) buffer, args, parent_pid);
}


void rpc_loadelf(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    char* first_space = strchr(data, ' ');

    //Wenn kein leerschlag vorhanden ist, muessen keine Kommandozeilenargumente
    //abgetrennt werden.
    if (first_space == NULL) {
        start_program(data, data, pid);
    } else {
        //Sonst wird erst der Name rauskopiert
        size_t filename_length = (dword) first_space - (dword) data;
        char* filename = malloc(filename_length + 1);
        memcpy(filename, data, filename_length);
        filename[filename_length] = '\0';

        start_program(filename, data, pid);
        
        free(filename);
    }

}

