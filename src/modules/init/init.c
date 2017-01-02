/*  
 * Copyright (c) 2006 The tyndur Project. All rights reserved.
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
#define MODULE_INIT

#include <stdint.h>
#include <stdbool.h>
#include "types.h"
#include "syscall.h"
#include "rpc.h"
#include "collections.h"
#include "stdlib.h"
#include "stdio.h"
#include <string.h>
#include "elf32.h"
#include "elf_common.h"
#include "io.h"
#include "dir.h"
#include "sleep.h"
#include "zlib.h"
#include <init.h>

#define ELF_MAGIC 0x464C457F
#define PAGE_OFFSET(a) (a % 0x1000)
#define PAGE_SHIFT 12
#define PAGE_SIZE (1 << PAGE_SHIFT)
#define PAGE_MASK (~(PAGE_SIZE - 1))
#define PAGE_ALIGN_ROUND_UP(n) (((n) + ~PAGE_MASK) & PAGE_MASK)
#define PAGE_ALIGN_ROUND_DOWN(n) ((n) & PAGE_MASK)

int main(void) { return 0; }

void rpc_service_register(pid_t pid, uint32_t correlation_id, size_t data_size, void* data);
void rpc_service_get(pid_t pid, uint32_t correlation_id, size_t data_size, void* data);
void rpc_service_get_name(pid_t pid, uint32_t correlation_id, size_t data_size, void* data);
void rpc_service_shutdown(pid_t pid, uint32_t correlation_id, size_t data_size, void* data);
void rpc_service_exit(pid_t pid, uint32_t correlation_id, size_t data_size, void* data);

void rpc_loadelf(pid_t pid, uint32_t correlation_id, size_t data_size, void* data);

void rpc_reopen_stdio(pid_t pid, uint32_t cid, size_t size, void* data);

void rpc_dev_register(pid_t pid, uint32_t cid, size_t size, void* data);
void rpc_dev_list(pid_t pid, uint32_t cid, size_t size, void* data);

void load_modules(init_module_list_t* modules);
pid_t load_single_module(void* image, size_t image_size, char* args,
    pid_t parent_pid, int ppb_shm_id);
static pid_t start_program(const char* path, char* args, pid_t parent_pid,
    int shm);
extern void init_sync_messages(void);

struct service_s* get_service_by_name(const char* name);
struct service_s* get_service_by_pid(pid_t pid);

extern void init_envvars(void* ppb, size_t ppb_size);

list_t* service_list;

/**
 * Verarbeitet die Kommandozeilenoptionen fuer init
 */
static void parse_cmdline(void)
{
    char* cmdline = get_cmdline();
    char* s;

    // Defaultwerte setzen
    setenv("CWD", "floppy:/devices/fd0|fat:/", 1);
    setenv("PATH", "floppy:/devices/fd0|fat:/apps", 1);

    // Den Programmnamen ('init') wegschneiden
    s = strtok(cmdline, " ");

    // Der Rest sind durch Leezeichen getrennte Optionen
    while ((s = strtok(NULL, " "))) {
        char* key = s;
        char* value = strchr(s, '=');

        if (value) {
            *value = '\0';
            value++;
        }

        if (!strcmp(key, "boot")) {

            if (!value) {
                printf("init: boot= benoetigt einen Wert\n");
                continue;
            }

            setenv("CWD", value, 1);

            // Fuer $PATH wird noch apps angehaengt
            size_t boot_size = strlen(value);
            char path[boot_size + 5];
            memcpy(path, value, boot_size);
            memcpy(path + boot_size, "apps", 5);
            setenv("PATH", path, 1);

        } else if (!strncmp(key, "env.", 4)) {

            if (key[4] == '\0') {
                printf("init: Umgebungsvariable benoetigt einen Namen\n");
                continue;
            }
            if (!value || !*value) {
                printf("init: %s= benoetigt einen Wert\n", key);
                continue;
            }

            setenv(key + 4, value, 1);

        }
    }
}

void _start(uint32_t modules)
{
    puts("\n\033[1;33mtyndur-Init " TYNDUR_VERSION " \033[0;37m\n");

    p();
    init_messaging();
    init_memory_manager();
    init_sync_messages(); 
    //Liste fuer die Prozesse erstellen
    service_list = list_create();
    init_envvars(NULL, 0);

    parse_cmdline();

    register_message_handler("SERV_REG", &rpc_service_register);
    register_message_handler("SERV_GET", &rpc_service_get);
    register_message_handler("SERV_NAM", &rpc_service_get_name);
    register_message_handler("SERV_SHD", &rpc_service_shutdown);
    register_message_handler("SERV_EXI", &rpc_service_exit);
    
    register_message_handler("LOADELF ", &rpc_loadelf);

    register_message_handler("UP_STDIO", &rpc_reopen_stdio);

    register_message_handler("DEV_REG ", &rpc_dev_register);
    register_message_handler("DEV_LIST", &rpc_dev_list);

    io_init();
    

    load_modules((init_module_list_t*) modules);
    v();

    while(true) {
        wait_for_rpc();
    }
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
void rpc_service_register(pid_t pid, uint32_t correlation_id, size_t data_size, void* data)
{
    struct service_s* new_service = malloc(sizeof(struct service_s));
    
    new_service->pid = pid;
    
    // Name kopieren
    data_size = strnlen((char*) data, data_size);
    new_service->name = malloc(data_size + 1);
    strncpy(new_service->name, (char*)data, data_size);
    new_service->name[data_size] = '\0';
   
    // Jetzt erst eintragen, damit die Zeit im kritischen Abschnitt
    // m�glichst kurz bleibt.
    p();
    service_list = list_push(service_list, (void*)new_service);
    
    printf("[  INIT  ] service_register '%s' = %d\n", new_service->name, new_service->pid);
    v();
}


/**
 * Einen Dienst anhand seines Namens finden
 */
void rpc_service_get(pid_t pid, uint32_t correlation_id, size_t data_size, void* data)
{
    int i;
    struct service_s* service;

    // Der Name muss nullterminiert sein
    if (strnlen(data, data_size) == data_size) {
        goto out_error;
    }
    
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

out_error:
    rpc_send_dword_response(pid, correlation_id, 0);
    
}

/**
 * Den Namen eines Dienstes anhand seiner pid finden
*/
void rpc_service_get_name(pid_t pid, uint32_t correlation_id, size_t data_size, void* data)
{
    if (data_size != sizeof(pid_t))
        return;

    pid_t service_pid = * (pid_t *) data;

    p();
    struct service_s* service = get_service_by_pid(service_pid);
    v();

    char *name = "";
    if (service != NULL)
        name = service->name;

    rpc_send_string_response(pid, correlation_id, name);
}

/**
 * Wird vom kernel gesendet, wenn ein Prozess beendet wird.
 */
void rpc_service_shutdown(pid_t pid, uint32_t correlation_id, size_t data_size, void* data)
{
    //pid_t* _pid = (pid_t*)data;
    
    //printf("[  INIT  ] service_shutdown %d\n", *_pid);
}



/**
 * Wird vom kernel gesendet, wenn ein Prozess beendet wird.
 */
void rpc_service_exit(pid_t pid, uint32_t correlation_id, size_t data_size, void* data)
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

void rpc_reopen_stdio(pid_t pid, uint32_t cid, size_t size, void* data)
{
    if (stdout != NULL) {
        fclose(stdout);
        stdout = NULL;
    }

    stdout = fopen("servmgr:/term", "w");
    setlinebuf(stdout);
}


void load_modules(init_module_list_t* modules) 
{ 
    int i;

    for(i = 0; i < modules->count; i++) 
    { 
        char* s = strdup(modules->modules[i].cmdline);
        int argc = cmdline_get_argc(s);
        const char* argv[argc + 1];
        int shm;

        cmdline_copy_argv(s, argv, argc);

        shm = ppb_from_argv(argv);
        if (shm == -1) {
            printf("init: Konnte PPB nicht erzeugen: '%s'\n",
                   modules->modules[i].cmdline);
            free(s);
            continue;
        }

        //printf("Modul %d: %s\n", i, modules->modules[i].cmdline);
        load_single_module((Elf32_Ehdr*) modules->modules[i].start,
            modules->modules[i].size, modules->modules[i].cmdline, 0, shm);
        mem_free(modules->modules[i].start, modules->modules[i].size);
        //msleep(50); // Damit die Meldungen nicht durcheinandergeraten

        close_shared_memory(shm);
        free(s);
    } 

    mem_free(modules->modules[0].cmdline, PAGE_SIZE);
    mem_free(modules, PAGE_SIZE);
}

size_t ainflate(void* src, size_t len, void** dest_ptr)
{
    int ret = 0;
    z_stream stream;

    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;

    //Quellpuffer Grösse
    stream.avail_in = len;
    //Pointer auf Quellpuffer
    stream.next_in = src;

    //Aufbau des GZip-Headers auf http://www.gzip.org/zlib/rfc-gzip.html

    //Testen ob die Datei wirklich GZip komprimiert ist
    uint8_t* src_buf = (uint8_t*) src;
    if((src_buf[0] != 0x1f) || (src_buf[1] != 0x8b))
    {
        puts("[  INIT  ] Datei enthaelt keinen korrekten GZip-Header");
        return 0;
    }
    
    //Beinhaltet Offset zu den Daten
    int i = 10;

    //Extra-Length dazuzaehlen (falls vorhanden)
    if((src_buf[3] & (1 << 2)) != 0) i += *((uint16_t*) (src_buf + 10));
    //Laenge des Dateinamens bestimmen (falls vorhanden)
    if((src_buf[3] & (1 << 3)) != 0) { for( ; src_buf[i] != 0; i++); i++; }
    //Laenge des Kommentars dazuzaehlen (falls vorhanden)
    if((src_buf[3] & (1 << 4)) != 0) { for( ; src_buf[i] != 0; i++); i++; }
    //2 Byte fuer CRC dazuzaehlen (falls vorhanden)
    if((src_buf[3] & (1 << 1)) != 0) i += 2;

    stream.next_in = (uint8_t*) ((uint32_t) src + i);
    stream.avail_in = len - i - 8;

    stream.avail_out = *((uint32_t*) (src_buf + len - 4));
    stream.next_out = malloc(stream.avail_out);
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

static pid_t start_program(const char* path, char* args, pid_t parent_pid,
    int shm)
{
    pid_t pid;
    FILE* f = fopen(path, "r");
    
    if(f == NULL)
    {
        return false;
    }

    void* buffer = NULL;
    size_t buffer_size = 0;
    long size = 0;
    
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    size_t pages = size / 4096;

    if(size % 4096 != 0)
    {
        pages++;
    }
    
    uint16_t gzip_id;
    fread(&gzip_id, 1, sizeof(uint16_t), f);
    fseek(f, 0, SEEK_SET);

    if(gzip_id == 0x8b1f)
    {
        uint8_t* in_buffer = malloc(pages * 4096);
        fread(in_buffer, 1, size, f);
        
        buffer_size = ainflate(in_buffer, size, &buffer);
        free(in_buffer);
        if (buffer_size == 0) {
            return false;
        }
    }
    else if (gzip_id == 0x2123) 
    {
        /* Skript: Interpreter aufrufen */
        char interpreter[256];
        char* interp_args;
        int ppb_argc, interp_argc, argc;
        int ret;
        size_t i;
        struct proc_param_block* ppb;
        size_t ppb_size;
        int new_shm;

        fgets(interpreter, 256, f);
        fclose(f);

        i = strlen(interpreter);
        if ((i > 0) && (interpreter[i - 1] == '\n')) {
            interpreter[i - 1] = '\0';
        }

        if (strlen(interpreter) < 2) {
            return false;
        }

        interp_args = interpreter + 2;

        ppb = open_shared_memory(shm);
        if (ppb == NULL) {
            return false;
        }
        ppb_size = PAGE_SIZE; // FIXME

        ppb_argc = ppb_get_argc(ppb, ppb_size);
        interp_argc = cmdline_get_argc(interp_args);
        argc = ppb_argc + interp_argc;

        const char* argv[argc + 1];

        cmdline_copy_argv(interp_args, argv, interp_argc);
        ppb_copy_argv(ppb, ppb_size, (char**) argv + interp_argc, ppb_argc);
        argv[argc] = NULL;

        new_shm = ppb_from_argv(argv);
        ret = start_program(argv[0], args, parent_pid, new_shm);
        close_shared_memory(new_shm);
        close_shared_memory(shm);

        return ret;
    }
    else
    {
        buffer_size = pages * 4096;
        buffer = malloc(buffer_size);
        fread(buffer, 1, size, f);
    }
    fclose(f);

    pid = load_single_module(buffer, buffer_size, args, parent_pid, shm);
    free(buffer);
    return pid;
}


void rpc_loadelf(pid_t pid, uint32_t correlation_id, size_t data_size, void* data)
{
    pid_t new_pid;
    size_t fn_len;
    int shm;

    // Wir erwarten einen Null-terminierten String und einen Integer
    fn_len = strnlen(data, data_size);

    if (data_size < fn_len + sizeof(int) + 1) {
        rpc_send_dword_response(pid, correlation_id, 0);
        return;
    }

    shm = *(int*)((uintptr_t) data + fn_len + 1);

    new_pid = start_program(data, data, pid, shm);
    rpc_send_dword_response(pid, correlation_id, new_pid);
}

