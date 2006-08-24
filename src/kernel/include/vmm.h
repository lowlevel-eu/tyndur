#ifndef VMM_H
#define VMM_H

#define PAGE_SHIFT 12
#define PAGE_SIZE (1 << PAGE_SHIFT)
#define PAGE_MASK (PAGE_SIZE - 1)

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

#endif /* ndef VMM_H */
