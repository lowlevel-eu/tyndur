/*  
 * Copyright (c) 2008 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Alexander Siol.
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

#include <init.h>
#include "stdio.h"
#include "types.h"
#include "stdlib.h"
#include "lostio.h"
#include "string.h"
#include <lost/config.h>
#include "syscall.h"

#include "cpuid.h"


/**
 * Legt /cpu im vfstree an, und fuellt die Datei mit Inhalt.
 */
void sysinfo_create_cpuid_file(void)
{
    char filename[] = "/cpu";
    char* content = malloc(4096);
    int size;
    
    cpuid_0 basic;
    cpuid_1 basic_flags;
    cpuid_e0 extended;
    cpuid_e1 extended_flags;
    cpuid_e2_e4 brand;
    
    // Alle Infos via cpuid abfragen
    basic = cpuid_level_0();
    basic_flags = cpuid_level_1();
    extended = cpuid_level_e0();
    extended_flags = cpuid_level_e1();
    brand = cpuid_level_e2_e4();

    // Wenn der extended level 0x8000004 gar nicht unterstuetzt ist, ist auch 
    // der processorBrandString ungueltig. Dieser ist dann zu ueberschreiben, um 
    // den Benutzer nicht mit einer leeren (oder gar zufaelligen) Ausgabe zu
    // konfrontieren.
    if (extended.max_level < 0x80000004) {
        sprintf((char*)&brand.proc_brand_string,"Unsupported.\0");
    }

    // Infoblock schreiben.
    size = sprintf(content, "%s: %s\n%s: %s\n%s: %i\n%s: 0x%08X\n%s:",
            "VendorID",  basic.vendorID,
            "Brand Name", brand.proc_brand_string,
            "Standardlevel", basic.max_level,
            "Erweitertes Level", extended.max_level,
            "Flags");

    // Die Diversen Flags ausgeben
    int i = 0;

    // Basic Flags Register EDX ueberpruefen
    for (i = 0; i < 32; i++) {
        if ((basic_flags.edx & (1 << i)) == (1 << i)) {
            size += sprintf(content+size, " %s", flags_01_edx[i]);
        }
    }

    // Basic Flags Register ECX ueberpruefen
    for (i = 0; i < 32; i++) {
        if ((basic_flags.ecx & (1 << i)) == (1 << i)) {
            size += sprintf(content+size, " %s", flags_01_ecx[i]);
        }
    } 

    // Extended Flags abfragen

    // Extended Flags von AMD
    if (strncmp(basic.vendorID,"AuthenticAMD",12) == 0) {
        // Register EDX
        for (i = 0; i < 32; i++) {
            if ((extended_flags.edx & (1 << i)) == (1 << i)) {
                size += sprintf(content+size, " %s", flags_e01_edx_amd[i]);
            }
        } 

        // Register ECX
        for (i = 0; i < 32; i++) {
            if ((extended_flags.ecx & (1 << i)) == (1 << i)) {
                size += sprintf(content+size, " %s", flags_e01_ecx_amd[i]);
            }
        }
    } 

    // Neue Linie als Dateiende
    size += sprintf(content+size,"\n");

    // Datei registrieren, Groesse ist Laenge aller geschriebenen Daten.
    vfstree_create_node(filename, LOSTIO_TYPES_RAMFILE, size, content, 0);
}

/**
 * Legt die Datei /ram unter sysinfo:/ ein, welche den Arbeitsspeicher in
 * Byte angibt
 **/
void sysinfo_create_ram_file(void)
{
    memory_info_t meminfo = memory_info();

    char filename[] = "/ram";
    int size;
    char* content = malloc(4096);

    size = sprintf(content, "%i", meminfo.total);

    vfstree_create_node(filename, LOSTIO_TYPES_RAMFILE, size, content, 0);

}

/**
 * main von sysinfo. Initialisiert LostIO und exportiert alle vorhandenen
 * Systeminfos nach aussen.
 *
 * @param argc Argumentzaehler. Hier Irrelevant.
 * @param argv Argumente. Hier Irrelevant.
 * @return Beendigungsstatus des Moduls.
 */
int main(int argc, char* argv[])
{
    // LostIO initialisieren
    lostio_init();

    // LostIO Standardtypen einbinden
    lostio_type_ramfile_use();
    lostio_type_directory_use();

    // /cpu anlegen - enthaelt Infos ueber die CPU, Basis hierfuer ist cpuid.c
    sysinfo_create_cpuid_file();

    // /ram anlegen - enthaelt Infos ueber den verfuegbaren Arbeitsspeicher
    sysinfo_create_ram_file();

    // Letztendlich bei Init registrieren
    init_service_register("sysinfo");

    // Warteschleife
    while(1) {
        wait_for_rpc();
    }

    return 0;
}
