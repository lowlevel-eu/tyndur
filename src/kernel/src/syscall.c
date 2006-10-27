#include "syscall.h"
#include "kprintf.h"
#include "vmm.h"
#include "paging.h"
#include "kernel.h"

void syscall(struct int_stack_frame ** esp) {
    struct int_stack_frame * isf = *esp;

    // FIXME: Adressen, die vom aufrufenden Programm kommen, 
    // vor der Verwendung prüfen
    switch (isf->eax) {
        case SYSCALL_KPRINTF:
            kprintf("%s", *((char**) isf->esp));
            break;

        case SYSCALL_MEM_ALLOCATE:
        {
            dword num;
            dword flags;

            paddr_t paddr;
            vaddr_t vaddr;

            num = PAGE_ALIGN_ROUND_UP(*((size_t*) isf->esp)) >> 12;
            flags = *((dword*) (isf->esp + 4));

           // kprintf("alloc %d Pages, flags %x\n", num, flags);
            
            
            // TODO: Flags berücksichtigen
            vaddr = find_contiguous_pages((page_directory_t) current_task->cr3, num, USER_MEM_START, USER_MEM_END);
            isf->eax = (dword) vaddr;

            while(num--) {
                paddr = phys_alloc_page();
                if (!map_page((page_directory_t) current_task->cr3, vaddr, paddr, PTE_P | PTE_U | PTE_W)) {
                    panic("Fehler beim Zuweisen der reservierten Speicherseite %x", vaddr);
                }
                vaddr += PAGE_SIZE;
            }
            
            break;
        }

        case SYSCALL_MEM_FREE:
            break;

        default:
            // TODO: Task beenden statt hängen lassen
            kprintf("\n"
                "\033[1;37m\033[41m" // weiss auf rot
                "Task gestoppt: Ungueltige Systemfunktion %d\n"
                "\033[0;37m\033[40m"
                , isf->eax);
            while(1) { asm("hlt"); }
            break;
    }
}
