#include "types.h"
#include "syscall.h"
#include "rpc.h"
#include "collections.h"


typedef struct service_s
{
    pid_t               pid;
    char*               name;
    
    struct service_s*   next;
    struct service_s*   prev;
};

void service_register(dword pid, dword data_size, void* data);


list_t* service_list;


void _start()
{
    init_messaging();
    init_memory_manager();
    
    puts("\nSetze RPC-Handler für INIT...");
        
    if (!register_message_handler("SERV_REG", &service_register))
    {
        puts("[FALSE]\n");
    }
    else
    {
        puts("[-OK-]\n");
    }
    
    while(1) yield();
}

void service_register(pid_t pid, dword data_size, void* data)
{
    //char* name = data;
    puts("\n\nservice_register");
    //puts(name);
    //puts("\n");
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
