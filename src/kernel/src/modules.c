#include "modules.h"
#include "multiboot.h"
#include "kprintf.h"
#include "elf32.h"
#include "vmm.h"
#include "kernel.h"
#include "string.h"
#include "paging.h"

#define ELF_MAGIC 0x464C457F
#define MODULE_ENTRY ((vaddr_t) 0x40000000)

void load_single_module(Elf32_Ehdr * elf_header);

void load_multiboot_modules(struct multiboot_info * multiboot_info)
{
  dword i;
  struct multiboot_module * multiboot_module;

  if (multiboot_info->mi_mods_count == 0) {
    kprintf("Keine Multiboot-Module zu laden.\n");
  } else {
    kprintf("Lade Multiboot-Module...\n");
    
    multiboot_module = multiboot_info->mi_mods_addr;

    for (i = 0; i < multiboot_info->mi_mods_count; i++) {
      kernel_identity_map(multiboot_module, sizeof(multiboot_module));
      kprintf("[%d @ %x] ", i,  multiboot_module);
      load_single_module(multiboot_module->start);
      multiboot_module++;
    }
  }
}  

void load_single_module(Elf32_Ehdr * elf_header) 
{
  Elf32_Phdr * program_header;
  dword i, j;
  dword pages;

  struct task * task;
  vaddr_t address;

  kprintf("[ehdr: %x]", elf_header);
  kernel_identity_map(elf_header, sizeof(elf_header));
  if (elf_header->e_magic == ELF_MAGIC) {
    
    program_header = (Elf32_Phdr *) ((dword) elf_header + elf_header->e_phoff);

    kprintf("    EntryCount: %d", elf_header->e_phnum);
    kprintf("    Offset: 0x%x\n", elf_header->e_phoff);

    for (i = 0; i < elf_header->e_phnum; i++) {
      kernel_identity_map(program_header, sizeof(program_header));

      kprintf(" ph @ %x, type=%d\n", program_header, program_header->p_type);
      if (program_header->p_type == 1) {

        kprintf("CREATE TASK\n");
        task = create_task(MODULE_ENTRY);
        
        // Pages erstellen
        pages = program_header->p_memsz / 0x1000;
        if (program_header->p_memsz % 0x1000) {
          pages++;
        }
        
        kprintf("Mapping Pages\n");
        for (j = 0; j < pages; j++) {
          paddr_t curPage = phys_alloc_page();
          map_page((page_directory_t) task->cr3, (vaddr_t) (MODULE_ENTRY + 0x1000 * j), curPage, PTE_P | PTE_U | PTE_W);
        }

        // Programm kopieren
        kprintf("Programm kopieren\n");
        pages = program_header->p_filesz / 0x1000;
        if (program_header->p_filesz % 0x1000) {
          pages++;
        }

        address = (vaddr_t) ((dword) elf_header + program_header->p_offset);
        for (j = 0; j < pages; j++) {
          kernel_identity_map(address, 0x1000);
          kernel_identity_map(resolve_vaddr((page_directory_t) task->cr3, (vaddr_t) (MODULE_ENTRY + 0x1000 * j)), 0x1000);
          memcpy(resolve_vaddr((page_directory_t) task->cr3, (vaddr_t) (MODULE_ENTRY + 0x1000 * j)), address, 0x1000);
          address += 0x1000;
        }

        kprintf("    Modul geladen.\n");
      }
      
      program_header++;
    }
    
  } else {
    kprintf("    ELF-Magic ungueltig (0x%x)\n",elf_header->e_magic);
  }
}
