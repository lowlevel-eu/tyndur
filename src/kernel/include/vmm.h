#ifndef VMM_H
#define VMM_H

#define PAGE_DIRECTORY_LENGTH 1024
#define PAGE_TABLE_LENGTH 1024

#define PAGE_SHIFT 12
#define PAGE_SIZE (1 << PAGE_SHIFT)
#define PAGE_MASK (~(PAGE_SIZE - 1))

// Die Anzahl der Pages, die von n Bytes belegt werden.
#define NUM_PAGES(n) ((((n) + ~PAGE_MASK) & PAGE_MASK) / PAGE_SIZE)

// Rundet eine Adresse auf das kleinste Vielfache von PAGE_SIZE > n auf
#define PAGE_ALIGN_ROUND_UP(n) (((n) + ~PAGE_MASK) & PAGE_MASK)

// Rundet eine Adresse auf das größte Vielfache von PAGE_SIZE < n ab
#define PAGE_ALIGN_ROUND_DOWN(n) ((n) & PAGE_MASK)

#define PGDIR_SHIFT 22

#define PTE_P       0x001 // present
#define PTE_W       0x002 // writable
#define PTE_U       0x004 // user
#define PTE_PWT     0x008 // write-through
#define PTE_PCT     0x010 // cache-disable
#define PTE_A       0x020 // accessed
#define PTE_D       0x040 // dirty
#define PTE_PS      0x080 // page size

#define PTE_AVAIL1  0x200 // available for software use
#define PTE_AVAIL2  0x400 // available for software use
#define PTE_AVAIL3  0x800 // available for software use

typedef unsigned long * page_directory_t;
typedef unsigned long * page_table_t;

typedef enum { page_4K, page_4M } page_size_t;

// Die Adresse, an der der Kernel-Adressraum beginnt
#define KERNEL_BASE 0x00000000

// Die Anzahl der Page Tables, die für den Kerneladressraum benötigt werden
#define NUM_KERNEL_PAGE_TABLES (PAGE_DIRECTORY_LENGTH - (KERNEL_BASE >> PGDIR_SHIFT))

// Alle Kernel Page Tables werden nach KERNEL_PAGE_TABLES_VADDR gemappt
#define KERNEL_PAGE_TABLES_VADDR 0x3fc00000


#define USER_MEM_START 0x40000000
#define USER_MEM_END   0xffffffff

#endif /* ndef VMM_H */
