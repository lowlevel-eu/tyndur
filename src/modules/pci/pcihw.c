/*  
 * Copyright (c) 2007-2010 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Kevin Wolf and Max Reitz.
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

#include <stdint.h>
#include <stdbool.h>
#include "ports.h"
#include "syscall.h"
#include "stdlib.h"

#include "pci.h"
#include "pcihw.h"

/**
 * Fordert die PCI-Ports beim Kernel zur Benutzung an.
 */
void pci_request_io_ports()
{
    request_ports(0xCF8, 0x8);
}

/**
 * Liest ein dword aus der PCI-Konfiguration eines Gerätes aus.
 * Ein PCI-Gerät wird über die drei Angabe bus:device:func identifiziert.
 *
 * @param bus Bus-Nummer des Geräts
 * @param device Gerätenummer des Geräts
 * @param func Funktionsnummer des Geräts
 * @param reg Offset des auszulesenden Registers
 *
 * @return Inhalt des angeforderten Registers. Ein komplettes dword wird nur
 * zurückgegeben, wenn reg dword-aligned ist. Ansonsten werden die
 * höchstwertigen Bits mit Nullen aufgefüllt.
 */
uint32_t pci_config_read(uint32_t bus, uint32_t device, uint32_t func,
    uint32_t reg)
{
    uint32_t offset = reg % 0x04;

    outl(PCI_CONFIG_ADDR, 
        0x1 << 31
        | ((bus     & 0xFF) << 16) 
        | ((device  & 0x1F) << 11) 
        | ((func    & 0x07) <<  8) 
        | ((reg     & 0xFC)));

    return inl(PCI_CONFIG_DATA) >> (8 * offset);
}

/**
 * @return Inhalt eines dword-Registers (32 Bit)
 * @see pci_config_read
 */
static inline uint32_t pci_config_read_dword
    (uint32_t bus, uint32_t device, uint32_t func, uint32_t reg)
{
    return pci_config_read(bus, device, func, reg);
}

/**
 * @return Inhalt eines word-Registers (16 Bit)
 * @see pci_config_read
 */
static inline uint16_t pci_config_read_word
    (uint32_t bus, uint32_t device, uint32_t func, uint32_t reg)
{
    return pci_config_read(bus, device, func, reg) & 0xffff;
}

/**
 * @return Inhalt eines byte-Registers (8 Bit)
 * @see pci_config_read
 */
static inline uint8_t pci_config_read_byte
    (uint32_t bus, uint32_t device, uint32_t func, uint32_t reg)
{
    return pci_config_read(bus, device, func, reg) & 0xff;
}

/**
 * Schreibt ein dword in ein Konfigurationsregister eines PCI-Geräts
 *
 * @param bus Bus-Nummer des Geräts
 * @param device Gerätenummer des Geräts
 * @param func Funktionsnummer des Geräts
 * @param reg Offset des zu schreibenden Registers
 *
 */
void pci_config_write_dword
    (uint32_t bus, uint32_t device, uint32_t func, uint32_t reg, uint32_t value)
{
    outl(PCI_CONFIG_ADDR, 
        0x1 << 31
        | ((bus     & 0xFF) << 16) 
        | ((device  & 0x1F) << 11) 
        | ((func    & 0x07) <<  8) 
        | ((reg     & 0xFC)));

    outl(PCI_CONFIG_DATA, value);
}

/**
 * Schreibt ein word in ein Konfigurationsregister eines PCI-Geräts
 * @see pci_config_write_dword
 */
void pci_config_write_word
    (uint32_t bus, uint32_t device, uint32_t func, uint32_t reg, uint16_t value)
{
    outl(PCI_CONFIG_ADDR,
        0x1 << 31
        | ((bus    & 0xFF) << 16)
        | ((device & 0x1F) << 11)
        | ((func   & 0x07) <<  8)
        | ((reg    & 0xFC)));

    outw(PCI_CONFIG_DATA + (reg & 2), value);
}

/**
 * Schreibt ein byte in ein Konfigurationsregister eines PCI-Geräts
 * @see pci_config_write_dword
 */
void pci_config_write_byte
    (uint32_t bus, uint32_t device, uint32_t func, uint32_t reg, uint8_t value)
{
    outl(PCI_CONFIG_ADDR,
        0x1 << 31
        | ((bus    & 0xFF) << 16)
        | ((device & 0x1F) << 11)
        | ((func   & 0x07) <<  8)
        | ((reg    & 0xFC)));

    outb(PCI_CONFIG_DATA + (reg & 3), value);
}

/**
 * Prueft ob ein PCI-Geraet Funktionen unterstuetzt.
 *
 * @param bus Bus-Nummer des Geräts
 * @param device Gerätenummer des Geräts
 */
bool pci_has_functions(uint32_t bus, uint32_t device)
{
    return pci_config_read_dword(bus, device, 0, PCI_HEADERTYPE) & (1 << 7);
}

/**
 * Liest ein BAR (Base Address Register) eines PCI-Geräts aus.
 *
 * Ein PCI-Gerät kann bis zu sechs BARs besitzen. Jedes BAR enthält die
 * Beschreibung einer Ressource (IO-Ports oder Speicher).
 *
 * @param bus Bus-Nummer des Geräts
 * @param device Gerätenummer des Geräts
 * @param func Funktionsnummer des Geräts
 * @param bar_number Nummer des BARs (0 bis 5)
 *
 * @return BAR des gegebenen Geräts mit der gegebenen Nummer oder NULL, wenn
 * ein Fehler auftritt.
 */
struct bar* pci_get_bar(uint32_t bus, uint32_t device, uint32_t func,
    uint32_t bar_number)
{
    struct bar* bar = malloc(sizeof(struct bar));
    uint32_t bar_value;
    uint32_t base_address_mask;
    uint32_t headertype =
        pci_config_read_dword(bus, device, func, PCI_HEADERTYPE);

    // Wir wollen nur ganz normale PCI-Geräte mit Header-Typ 0x0 (keine
    // PCI-to-PCI-Bridges)
    // Bit 7 darf gesetzt sein, dieses kennzeichnet eine Gerät, das mehrere
    // Funktionen unterstützt.
    if (!bar || headertype & 0x3F) {
        return NULL;
    }

    // Auslesen des BAR
    // Bit 0 gibt an, ob das BAR einen Speicherbereich (0) oder IO-Ports (1)
    // bezeichnet. Bits 4 bis 31 (Speicher) bzw. Bits 2 bis 31 (IO-Ports) 
    // bezeichnen die Basisadresse der Ressource.
    bar_value = pci_config_read_dword(bus, device, func, 
        PCI_BAR0 + (4 * bar_number));        
    bar->type = bar_value & 1;
    bar->addr = (void*) (bar_value & ~0xF);

    // Bestimmen der Größe der Ressource
    // Die Größe kann nicht direkt ausgelesen werden, sie muß indirekt
    // ermittelt werden:
    // 1) BAR auslesen und alten Wert sichern.
    // 2) BAR mit Binäreinsen füllen. Alle Bits, die außerhalb des benutzbaren
    //    Bereichs sind, sind fest verdrahtet und können von Software nicht
    //    geändert werden.
    // 3) BAR auslesen und prüfen, wie viele Einsen gesetzt werden konnten.
    //    Daraus läßt sich die Größe der Ressource ableiten.
    // 4) Originalwert des BARs zurückschreiben

    if (bar->type == bar_mem) {
        base_address_mask = 0xFFFFFFF0;
    } else {
        base_address_mask = 0xFFFF & ~0x3;
    }

    pci_config_write_dword(bus, device, func,
        PCI_BAR0 + (4 * bar_number),
        base_address_mask | (bar_value & ~base_address_mask));

    bar->size = pci_config_read_dword(bus, device, func,
        PCI_BAR0 + (4 * bar_number));

    bar->size = ((~bar->size & base_address_mask)
              | (~base_address_mask & 0xF)) + 1;

    pci_config_write_dword(bus, device, func, 
        PCI_BAR0 + (4 * bar_number), bar_value);

    // Eingelesene Information zurückgeben
    return bar;
}

/**
 * Gibt Basisinformationen über ein PCI-Gerät zurück. Dazu zählen insbesondere
 * Hersteller- und Geräte-ID. Die Ressourcen-Liste wird nicht angelegt.
 *
 * @param bus Bus-Nummer des Geräts
 * @param device Gerätenummer des Geräts
 * @param func Funktionsnummer des Geräts
 *
 * @return Basisinformationen über das PCI-Gerät
 */
struct pci_device pci_get_device(uint32_t bus, uint32_t device, uint32_t func)
{
    struct pci_device pci_device = {
        .bus        = bus,
        .device     = device,
        .function   = func,

        .vendor_id  = pci_config_read_word(bus, device, func, PCI_VENDOR_ID),
        .device_id  = pci_config_read_word(bus, device, func, PCI_DEVICE_ID),

        .class_id   = pci_config_read_byte(bus, device, func, PCI_CLASS),
        .subclass_id  = pci_config_read_byte(bus, device, func, PCI_SUBCLASS),
        .interface_id = pci_config_read_byte(bus, device, func, PCI_INTERFACE),

        .rev_id     = pci_config_read_byte(bus, device, func, PCI_REVISION),
        .irq        = pci_config_read_byte(bus, device, func, PCI_INTERRUPT),

        .resources  = NULL
    };

    return pci_device;
}
