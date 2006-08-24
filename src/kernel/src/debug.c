#include "elf32.h"
#include "kprintf.h"
#include "multiboot.h"
#include "string.h"

extern struct multiboot_info multiboot_info;

Elf32_Sym * find_sym(Elf32_Shdr * symtab, dword addr)
{
    Elf32_Sym * sym = (Elf32_Sym *)symtab->sh_addr;
    int i;

    for(i = 0; i < symtab->sh_size / sizeof(Elf32_Sym); i++)
    {
        if(addr >= (dword)(sym[i].st_value) && addr < (dword)(sym[i].st_value) + sym[i].st_size)
        {
            return sym + i;
        }
    }

    return 0;
}

char * find_str(Elf32_Shdr * strtab, Elf32_Word index)
{
    return (char*)strtab->sh_addr + index;
}

void stack_backtrace()
{
    dword * ebp;
    int i;
    Elf32_Shdr * symtab = 0, * strtab = 0;
    Elf32_Sym * sym;

    if(multiboot_info.mi_flags & MULTIBOOT_INFO_HAS_ELF_SYMS)
    {
        /* Symbol Table und String Table suchen */

        for (i = 0; i < multiboot_info.mi_elfshdr_num; i++)
        {
            Elf32_Shdr * shdr = &((Elf32_Shdr *)multiboot_info.mi_elfshdr_addr)[i];

            if(shdr->sh_type == SHT_SYMTAB)
            {
                symtab = shdr;
                strtab = &((Elf32_Shdr *)multiboot_info.mi_elfshdr_addr)[symtab->sh_link];
            }
        }
    }

    kprintf("stack backtrace:\n");
    __asm__ __volatile__ ("mov %%ebp, %0" : "=r"(ebp));
    for( ; ebp != 0; ebp = (dword*)(ebp[0]))
    {
        Elf32_Sym * sym;
        kprintf("ebp %08x eip %08x", ebp[0], ebp[1]);

        if(symtab != 0 && strtab != 0)
        {
            sym = find_sym(symtab, ebp[1]);
            if(sym != 0)
            {
                kprintf(" <%s + 0x%x>", find_str(strtab, sym->st_name), ebp[1] - sym->st_value);
            }
        }

        kprintf("\n");
//        kprintf("\n\targs %08x %08x %08x %08x %08x\n", ebp[2], ebp[3], ebp[4], ebp[5], ebp[6]);
    }
}
