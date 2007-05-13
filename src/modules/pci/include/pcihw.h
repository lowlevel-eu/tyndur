#ifndef _PCI_HW_
#define _PCI_HW_

#define PCI_CONFIG_ADDR 0xCF8
#define PCI_CONFIG_DATA 0xCFC

#define PCI_VENDOR_ID   0x00
#define PCI_DEVICE_ID   0x02
#define PCI_COMMAND     0x04
#define PCI_STATUS      0x06
#define PCI_REVISION    0x08
#define PCI_CLASS       0x09
#define PCI_HEADERTYPE  0x0E
#define PCI_BAR0        0x10
#define PCI_INTERRUPT   0x3C


struct bar {
    void* addr;
    size_t size;
    enum { bar_mem = 0, bar_io = 1} type;
};

void pci_request_io_ports();

void scanpci();
char* pciinfo();

void pci_request_io_ports();
struct bar* pci_get_bar(dword bus, dword device, dword func, dword bar_number);
struct pci_device pci_get_device(dword bus, dword device, dword func);

#endif
