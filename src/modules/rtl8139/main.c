/*  
 * Copyright (c) 2007 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
 * by Kevin Wolf.
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

#include "types.h"
#include "syscall.h"
#include "rpc.h"
#include "stdlib.h"
#include "stdio.h"
#include "pci.h"
#include "ports.h"

#define _USE_START_
#include "init.h"

#include "rtl8139.h"

//Hier koennen die Debug-Nachrichten aktiviert werden
#define DEBUG_MSG(s) printf("[RTL8139] debug: %s() '%s'\n", __FUNCTION__, s)
//#define DEBUG_MSG(s) //

struct netcard {
    byte    irq;
    void*   mem_base;
    dword   port_base;

    qword   mac : 48;
    dword   ip;

    dma_mem_ptr_t buffer;
    bool buffer_used;
    byte cur_buffer;

    dma_mem_ptr_t rx_buffer;
    dword rx_buffer_offset;
};

void get_netcard(struct netcard* netcard);
void init_netcard(struct netcard* netcard);
void irq_handler();
bool send(struct netcard* netcard, void* data, dword size);

void test_arp(struct netcard* netcard);

// TODO Unterstützung für mehrere Netzwerkkarten
struct netcard* the_one_and_only_netcard;


int main(int argc, char* argv[])
{
    struct netcard netcard;
    get_netcard(&netcard);

    printf("Netzwerkkarte: IRQ %d, Ports ab %x, Speicher ab %x\n", 
        netcard.irq, netcard.port_base, (dword) netcard.mem_base);

    init_netcard(&netcard);
    netcard.ip = 0xc0a8010b; // 192.168.1.11

    printf("MAC: %llx\n", netcard.mac);
    the_one_and_only_netcard = &netcard;

    test_arp(&netcard);

    while(1) {
        wait_for_rpc();
    }
}


static inline void write_register_byte(struct netcard* netcard, byte reg, byte value) 
{
    outb(netcard->port_base + reg, value);
}

static inline void write_register_word(struct netcard* netcard, byte reg, word value) 
{
    outw(netcard->port_base + reg, value);
}

static inline void write_register_dword(struct netcard* netcard, byte reg, dword value) 
{
    outl(netcard->port_base + reg, value);
}

static inline byte read_register_byte(struct netcard* netcard, byte reg) 
{
    return inb(netcard->port_base + reg);
}

static inline word read_register_word(struct netcard* netcard, byte reg) 
{
    return inw(netcard->port_base + reg);
}

static inline dword read_register_dword(struct netcard* netcard, byte reg) 
{
    return inl(netcard->port_base + reg);
}

void init_netcard(struct netcard* netcard)
{
    DEBUG_MSG("Interrupthandler und Ports registrieren");
    register_intr_handler(0x20 + netcard->irq, irq_handler);
    request_ports(netcard->port_base, 0x100);

    // Einen Reset der Karte durchführen
    DEBUG_MSG("Reset der Karte");
    write_register_byte(netcard, REG_COMMAND, CR_RESET);
    
    // MAC-Adresse auslesen
    DEBUG_MSG("MAC-Adresse auslesen");
    netcard->mac = 
        read_register_dword(netcard, REG_ID0)
        | (((qword) read_register_dword(netcard, REG_ID4) & 0xFFFF) << 32);

    // Receive/Transmit (Rx/Tx) aktivieren
    DEBUG_MSG("Aktiviere Rx/Tx");
    write_register_byte(netcard, REG_COMMAND, 
        CR_RECEIVER_ENABLE | CR_TRANSMITTER_ENABLE);

    // RCR/TCR initialisieren
    DEBUG_MSG("Inititalisiere RCR/TCR");
    write_register_dword(netcard, REG_RECEIVE_CONFIGURATION,
        RCR_MXDMA_UNLIMITED | RCR_ACCEPT_BROADCAST | RCR_ACCEPT_PHYS_MATCH);
    
    write_register_dword(netcard, REG_TRANSMIT_CONFIGURATION,
        TCR_IFG_STANDARD | TCR_MXDMA_2048);

    // Wir wollen Interrupts, wann immer es was zu vermelden gibt
    DEBUG_MSG("Setze Interruptmaske");
    write_register_word(netcard, REG_INTERRUPT_STATUS, 0);
    write_register_word(netcard, REG_INTERRUPT_MASK, 0xFFFF);
    
    // Eine Page für Puffer
    // FIXME Eigentlich sollte man hier keinen DMA-Speicher brauchen, aber
    // der Kernel gibt nur bei DMA-Speicher die physische Adresse raus
    DEBUG_MSG("Initialisiere Buffer");
    netcard->buffer = mem_dma_allocate(0x1000, 0);
    netcard->buffer_used = FALSE;
    netcard->cur_buffer = 0;

    netcard->rx_buffer.phys = netcard->buffer.phys + 0x800;
    netcard->rx_buffer.virt = netcard->buffer.virt + 0x800;

    write_register_dword(netcard, REG_RECEIVE_BUFFER, 
        (dword) netcard->rx_buffer.phys);

    netcard->rx_buffer_offset = 0;

    DEBUG_MSG("Fertig initialisiert");
}

void get_netcard(struct netcard* netcard)
{
    FILE* f;
    DEBUG_MSG("Hole Geraetedaten von PCI");
    
    netcard->irq = 0;
    netcard->mem_base = 0x0;
    netcard->port_base = 0x0;

    f = fopen("pci:/devices/10ec:8139", "r");
    if (f == NULL) {
        printf("Keine RTL 8139 gefunden.\n");
        exit(EXIT_FAILURE);
    }     
    
    struct pci_device device;
    if (!fread(&device, sizeof(device), 1, f)) {
        printf("Konnte nicht von pci:/devices/10ec:8139 lesen.\n");
        fclose(f);
        exit(EXIT_FAILURE);
    }

    DEBUG_MSG("Hole Ressourcendaten von PCI");
    struct pci_resource res;
    while (!feof(f)) {
        if (fread(&res, sizeof(res), 1, f)) {
            switch (res.type) {
                case PCI_RESOURCE_MEM:
                    netcard->mem_base = (void*) res.start;
                    break;
                
                case PCI_RESOURCE_PORT:
                    netcard->port_base = res.start;
                    break;
            }
        } else {
            printf("Konnte nicht von pci:/devices/10ec:8139 lesen.\n");
            fclose(f);
            exit(EXIT_FAILURE);
        }
    }

    fclose(f);

    netcard->irq = device.irq;
    DEBUG_MSG("PCI fertig abgefragt");
}

bool send(struct netcard* netcard, void* data, dword size)
{
    if (size > 0x700) {
        // Spezialfall - keine Lust
        DEBUG_MSG("size ist boese");
        return FALSE;
    }

    DEBUG_MSG("Vor dem p()...");
    // FIXME printf innerhalb von p()/v()
    p();
    if (netcard->buffer_used) {
//        DEBUG_MSG("Tx-Buffer ist schon besetzt");
        v();
        return FALSE;
    }
//    DEBUG_MSG("Tx-Buffer reserviert");
    // Kopiere das Packet in den Puffer. Wichtig: Der Speicherbereich
    // muß physisch zusammenhängend sein.
    netcard->buffer_used = TRUE;
    memcpy(netcard->buffer.virt, data, size);
    
//    printf("Phys Buffer = %08x\n", netcard->buffer.phys);

    // Adresse und Größe des Buffers setzen
    // In TASD (REG_TRANSMIT_STATUS) wird das OWN-Bit gelöscht, was die
    // tatsächliche Übertragung anstößt.
    write_register_dword(netcard, 
        REG_TRANSMIT_ADDR0 + (4 * netcard->cur_buffer), 
        (dword) netcard->buffer.phys);

    write_register_dword(netcard, 
        REG_TRANSMIT_STATUS0 + (4 * netcard->cur_buffer), 
        size);

    // Die vier Deskriptoren müssen der Reihe nach verwendet werden,
    // auch wenn wir momentan nur einen einzigen Puffer verwenden
    // und immer warten.
    netcard->cur_buffer++;
    netcard->cur_buffer %= 4;

    v();
    DEBUG_MSG("Gesendet");

    return TRUE;
}

void receive_ok_handler(struct netcard* netcard)
{
    DEBUG_MSG("ROK");

    while (1)
    {
        dword cr = read_register_byte(netcard, REG_COMMAND);
        if (cr & CR_BUFFER_IS_EMPTY) {
            printf("Rx-Puffer ist leer.\n");
            break;
        }
        
        void* buffer = netcard->rx_buffer.virt + netcard->rx_buffer_offset;

        word packet_header = *((word*) buffer);
        buffer += 2;

        if ((packet_header & 1) == 0) {
            printf("Paketheader: Kein ROK.\n");
            break;
        }

        // Die Länge enthält die CRC-Prüfsumme, nicht aber den Paketheader
        word length = *((word*) buffer); 
        buffer += 2;
        
        printf("Empfangen: %d Bytes\n", length);

        int i;
        for (i = 0; i < length; i++) {
            printf("%02x ", ((char*) buffer)[i]);
        }
        printf("\n");

        // Den aktuellen Offset im Lesepuffer anpassen. Jedes Paket ist 
        // dword-aligned, daher anschließend Aufrundung.
        // TODO Behandlung von Überläufen
        netcard->rx_buffer_offset += length + 4;    
        netcard->rx_buffer_offset = (netcard->rx_buffer_offset + 3) & ~0x3;

        write_register_dword(netcard, REG_CUR_READ_ADDR, 
            netcard->rx_buffer_offset);
    }
}

void transmit_ok_handler(struct netcard* netcard)
{
    DEBUG_MSG("TOK");
    netcard->buffer_used = FALSE;
}

void irq_handler()
{
    printf("<RTL8139 Interrupt>");

    struct netcard* netcard = the_one_and_only_netcard;
    dword isr = read_register_word(netcard, REG_INTERRUPT_STATUS);
    dword clear_isr = 0;

    if (isr & ISR_RECEIVE_OK) {
        receive_ok_handler(netcard);
        clear_isr |= ISR_RECEIVE_OK;
    } 

    if (isr & ISR_TRANSMIT_OK){
        transmit_ok_handler(netcard);
        clear_isr |= ISR_TRANSMIT_OK;
    }

    // Achtung Stolperfalle: Bits, die beim Schreiben auf den Port
    // gesetzt sind, werden im Register gelöscht. Es ist also nicht
    // der neue Wert des Registers zu übergeben, sondern die
    // verarbeiteten und somit zu löschenden Bits.
    write_register_word(netcard, REG_INTERRUPT_STATUS, clear_isr);
}


//////////////////////////////////////////////////

#include "ethernet.h"

#define ARP_HW_ETHERNET 0x0100
#define ARP_PROTO_IPV4 0x0008
#define ARP_HWADDRSIZE_ETHERNET 6
#define ARP_PROTOADDRSIZE_IPV4 4
#define ARP_CMD_REQUEST 0x0100
#define ARP_CMD_RESPONSE 0x0200

struct arp {
    word    hwtype;
    word    proto;
    byte    hwaddrsize;
    byte    protoaddrsize;
    word    command;

    qword   source_mac : 48;
    dword   source_ip;

    qword   dest_mac : 48;
    dword   dest_ip;
} __attribute__ ((packed));

void test_arp(struct netcard* netcard) 
{
    void* packet = malloc(sizeof(struct eth_packet_header) + sizeof(struct arp));

    struct eth_packet_header* eth = packet;
    struct arp* arp = packet + sizeof(struct eth_packet_header);
    
    eth->src = netcard->mac;
    eth->dest = 0xFFFFFFFFFFFFLL;
    eth->type = ETH_TYPE_ARP;

    arp->hwtype = ARP_HW_ETHERNET;
    arp->proto = ARP_PROTO_IPV4;
    arp->hwaddrsize = ARP_HWADDRSIZE_ETHERNET;
    arp->protoaddrsize = ARP_PROTOADDRSIZE_IPV4;
    arp->command = ARP_CMD_REQUEST;

    arp->source_mac = eth->src;
    arp->source_ip = netcard->ip;
    arp->dest_mac = eth->dest;
    //p->dest_ip = 0xc0a80103; // 192.168.1.3
    arp->dest_ip = 0x0202000a; // 10.0.2.2

    send(netcard, packet, sizeof(struct eth_packet_header) + sizeof(struct arp));
}
