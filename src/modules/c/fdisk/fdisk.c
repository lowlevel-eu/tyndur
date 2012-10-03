/*
 * Copyright (c) 2008 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Martin Kleusberg.
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

#include <types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <readline/readline.h>

#include "partition.h"
#include "typeids.h"

/**
 * Diese Variable wird auf 1 gesetzt, wenn Aenderungen vorgenommen 
 * wurden, und sobald diese auf die Festplatte geschrieben wurden, auf 0
 * zurueckgesetzt
 */
static int unsaved_changes = 0;

/**
 * Diese Funktion wird nur vorruebergehend benoetigt, da getchar() nicht
 * auf einen Tastendruck wartet.
 */
// TODO Entfernen, sobald getchar wie erwartet funktioniert
static char keyboard_read_char(void)
{
    fflush(stdout);
    char c = 0;
    while (!fread(&c, 1, 1, stdin));
    putchar(c);
    return c;
}

/** 
 * Diese Funktion konvertiert 2 chars in einen 8bit Integer Wert,
 * z.B.: c1 = F und c2 = F => Ergebis = 255.
 */
// TODO Entfernen, sobald sscanf implementiert ist
static unsigned char hex2uint8(char c1, char c2)
{
    unsigned char temp = 0;
    char ret_val = 0;
    
    if (c1 >= '0' && c1 <= '9') {
        temp = c1 - '0';
    }
    if (c1 >= 'A' && c1 <= 'F') {
        temp = c1 - 'A' + 10;
    }
    if (c1 >= 'a' && c1 <= 'f') {
        temp = c1 - 'a' + 10;
    }
    ret_val = temp * 16;
    
    if (c2 >= '0' && c2 <= '9') {
        temp = c2 - '0';
    }
    if (c2 >= 'A' && c2 <= 'F') {
        temp = c2 - 'A' + 10;
    }
    if (c2 >= 'a' && c2 <= 'f') {
        temp = c2 - 'a' + 10;
    }
    ret_val += temp;
    
    return ret_val;
}

/** Gibt alle Partitionen auf der bearbeiten Platte nach stdout aus */
static void shell_list_partitions(void)
{
    // Allgemeine Daten und Statistiken
    printf("%s\n", device_name);
    printf("  Disk Signature: 0x%-08x\n\n", get_disk_signature());
    
    // Kopfzeile der Tabelle
    printf("# | Start      | Ende       | Boot | Typ\n");
    
    // TODO extended/logical partitions unterstuetzen
    
    // Partitionen auflisten
    int i;
    for (i=0;i<4;i++) {
        // Nur dann was anzeigen, wenn die Partition auch existiert
        if (mbr_partitions[i].exists) {
            printf("%2i|%12i|%12i| %c    | %02x\n", mbr_partitions[i].number,
                mbr_partitions[i].data.start_sector,
                get_end_sector(&mbr_partitions[i]),
                mbr_partitions[i].data.bootable == BOOTFLAG_SET ? '*' : ' ',
                    (int)mbr_partitions[i].data.type);
        }
    }
}

/** Aendert das bootable-Flag einer Partition */
static void shell_toggle_bootable(void)
{
    // TODO extended/logical partitions unterstuetzen
    
    // Den User nach der Partitionsnummer fragen. Den passenden Array Index
    // erhaelt man durch Subtraktion von 1
    printf("Partitionsnummer (0 = Abbrechen): ");
    int partition = (int)(keyboard_read_char()-'0') - 1;
    printf("\n");
    
    // Abbrechen?
    if (partition == -1) {
        return;
    }
    
    // Eingabe ueberpruefen. Die Nummer muss gueltig sein, und die
    // dazugehoerige Partition soll exisitieren
    if (partition < 0 || partition > 3 ||
        mbr_partitions[partition].exists == 0)
    {
        printf("Ungueltige Partition\n");
    } else {
        if (mbr_partitions[partition].data.bootable == BOOTFLAG_UNSET) {
            mbr_partitions[partition].data.bootable = BOOTFLAG_SET;
        } else if (mbr_partitions[partition].data.bootable == BOOTFLAG_SET) {
            mbr_partitions[partition].data.bootable = BOOTFLAG_UNSET;
        } else {
            printf("WARNUNG: Das Bootable-Flag war ungueltig! Moeglicherweise "
                "ist die Partitionstabelle beschaedigt!\n");
            printf("         Setze es auf 'nicht bootbar'.\n");
            mbr_partitions[partition].data.bootable = BOOTFLAG_UNSET;
        }
        unsaved_changes = 1;
    }
}

/** 
 * Schreibt die vorgenommenen Aenderungen an der Partitionstabelle 
 * auf die Festplatte. 
 */
static void shell_write(void)
{
    // Sollen die aenderungen wirklich auf die HDD geschrieben werden?
    printf("WARNUNG: Sie sollten sich absolut sicher sein, was Sie tun! "
        "Diese Aktion koennte Ihre Daten zerstoeren und es ist schwer "
        "bis unmoeglich, diesen Schritt rueckgaengig zu machen.\n");
    printf("Wirklich schreiben? [Jn] ");
    char answer = keyboard_read_char();
    printf("\n");
    
    if (answer == 'J') {
        if (!apply_changes()) {
            printf("Gespeichert. Starten Sie das System jetzt neu.\n");
            unsaved_changes = 0;
        }
    }
}

/** Loescht eine Partition */
static void shell_delete(void)
{
    // Den User nach der Partitionsnummer fragen. Den passenden Array Index
    // erhaelt man durch Subtraktion von 1
    printf("Partitionsnummer (0 = Abbrechen): ");
    int partition = (int)(keyboard_read_char()-'0') - 1;
    printf("\n");
    
    // TODO extended/logical partitions unterstuetzen
    
    // Abbrechen?
    if (partition == -1) {
        return;
    }
    
    // Eingabe ueberpruefen. Die Nummer muss gueltig sein, und die
    // dazugehoerige Partition soll (noch) exisitieren
    if (partition < 0 || partition > 3 ||
        mbr_partitions[partition].exists == 0)
    {
        printf("Ungueltige Partition\n");
    } else {
        delete_partition(&mbr_partitions[partition]);
        unsaved_changes = 1;
    }
}

/** Listet alle bekannten Partitionstypen auf */
static void shell_typelist(void)
{
    int i;
    int types_printed = 0;
    for (i=0;i<256;i++) {
        if (partition_type_id[i].name != 0) {
            // 3 Typen pro Zeile anzeigen. Um diesen Wert zu aendern, muss
            // die 21 im printf String und die 3 in der if Abfrage geaendert
            // werden
            printf("%02x %-21s", i, partition_type_id[i].name);
            if (((++types_printed) % 3 == 0) || (i == 255)) {
                printf("\n");
            } else {
                printf(" - ");
            }
        }
    }
}

/** Setzt den Partitionstyp */
static void shell_settype(void)
{
    // Den User nach der Partitionsnummer fragen. Den passenden Array Index
    // erhaelt man durch Subtraktion von 1
    printf("Partitionsnummer (0 = Abbrechen): ");
    int partition = (int)(keyboard_read_char()-'0') - 1;
    printf("\n");
    
    // TODO extended/logical partitions unterstuetzen
    
    // Abbrechen?
    if (partition == -1) {
        return;
    }
    
    // Eingabe ueberpruefen. Die Nummer muss gueltig sein, und die
    // dazugehoerige Partition muss exisitieren
    if (partition < 0 || partition > 3 ||
        mbr_partitions[partition].exists == 0)
    {
        printf("Ungueltige Partition\n");
    } else {
        // TODO Die Eingabe sollte komfortabler werden. Daher muss das
        //      Eingabeformat eingeschraenkt werden und eine Suchfunktion
        //      bzw. etwas vergleichbares integiert werden.
        // Den User solange nach einer neuen ID fragen, wie er eine
        // ungueltige eingibt
        char good_input = 0;
        char* input;
        do {
            input = readline("Bitte den neue Partitionstyp als Hexcode "
                "eingeben (h fuer Hilfe): ");
            if (strcmp(input, "h") == 0) {
                shell_typelist();
                free(input);
            } else if (isxdigit(input[0]) && isxdigit(input[1]) &&
                input[2] == 0)
            {
                good_input = 1;
            } else {
                free(input);
            }
        } while (good_input == 0);
        // Die Eingabe konvertieren und den Partitionseintrag entsprechend
        // abaendern
        mbr_partitions[partition].data.type = hex2uint8(input[0], input[1]);
        free(input);
        
        unsaved_changes = 1;
    }
}

/** Erstellt eine neue Partition */
static void shell_new(void)
{
    // TODO extended/logical partitions unterstuetzen
    // TODO Funktion etwas aufraeumen
    
    // Den User nach der Partitionsnummer fragen. Den passenden Array Index
    // erhaelt man man durch Subtraktion von 1
    printf("Partitionsnummer (0 = Abbrechen): ");
    int partition = (int)(keyboard_read_char()-'0') - 1;
    printf("\n");
    
    // Abbrechen?
    if (partition == -1) {
        return;
    }
    
    // Eingabe ueberpruefen. Die Nummer muss gueltig sein, und die
    // dazugehoerige Partition darf noch nicht exisitieren
    if (partition < 0 || partition > 3 ||
        mbr_partitions[partition].exists != 0)
    {
        printf("Ungueltige Partitionsnummer\n");
    } else {
        // TODO Nur gueltige Werte akzeptieren, so dass es nicht mehr moeglich
        //      ist, Partitionen zu erstellen, die zu gross sind, nicht
        //      vollstaendig auf der Festplatte liegen, die sich mit anderen
        //      Partitionen ueberlappen oder die ihren Startpunkt nach ihrem
        //      Ende haben
        
        // In welchem Format soll die Startadresse eingegeben werden
        printf("Partitionsanfang: Angabe in CHS oder LBA? [CL]  ");
        char input_type = 0;
        do {
            printf("\b"); // NOTE Diese Zeile sollte eigentlich das Zeichen von
                          // der letzten (falschen) Eingabe entfernen.
                          // Das \b funktioniert momentan aber nicht richtig
            input_type = keyboard_read_char();
        } while (input_type != 'C' && input_type != 'c' && input_type != 'L' &&
            input_type != 'l');
        printf("\n");
        
        // Start Adresse
        unsigned int start_lba;
        if (input_type == 'C' || input_type == 'c') {
            // User will Adresse im CHS Format eingeben
            char* input = readline("Partitionsanfang: Zylinder? ");
            unsigned int start_cylinder = atoi(input);
            free(input);
            input = readline("Partitionsanfang: Kopf? ");
            unsigned int start_head = atoi(input);
            free(input);
            input = readline("Partitionsanfang: Sektor? ");
            unsigned int start_sector = atoi(input);
            free(input);
            
            start_lba = chs2lba(start_cylinder, start_head, start_sector);
        } else {
            // Eingabe im LBA Format
            char* input = readline("Partitionsanfang: LBA-Adresse? ");
            start_lba = atoi(input);
            free(input);
        }
        
        // Soll die Endadresse im CHS oder im LBA Format eingegeben werden?
        // Oder anhand der Groesse berechnet werden?
        printf("Partitionsende: Angabe CHS, LBA oder als Groesse? [CLS]  ");
        do {
            printf("\b");
            input_type = keyboard_read_char();
        } while (input_type != 'C' && input_type != 'c' && input_type != 'L' &&
            input_type != 'l' && input_type != 'S' && input_type != 's');
        printf("\n");
        
        // Endposition bzw. Groesse ermitteln
        unsigned int end_lba;
        if (input_type == 'C' || input_type == 'c') {
            // User will Adresse im CHS Format eingeben
            char* input = readline("Partitionsende: Zylinder? ");
            unsigned int end_cylinder = atoi(input);
            free(input);
            input = readline("Partitionsende: Kopf? ");
            unsigned int end_head = atoi(input);
            free(input);
            input = readline("Partitionsende: Sektor? ");
            unsigned int end_sector = atoi(input);
            free(input);
            
            end_lba = chs2lba(end_cylinder, end_head, end_sector);
        } else if (input_type == 'L' || input_type == 'l') {
            // Eingabe im LBA Format
            char* input = readline("Partitionsende: LBA-Adresse? ");
            end_lba = atoi(input);
            free(input);
        } else {
            // Endposition anhand der Partitionsgroesse berechnen
            char* input = readline("Partitionsende: Groesse "
                "(in Sectoren)? ");
            end_lba = start_lba + atoi(input) - 1;
            free(input);
        }
        
        // Neue Partition erstellen
        create_partition(&mbr_partitions[partition], partition + 1, start_lba,
            end_lba);
        
        unsaved_changes = 1;
    }
}

/** fdisk-Shell, das Hauptmenue. Ruft alle anderen Funktionen auf. */
static void shell(void)
{
    printf("h fuer Hilfe\n");
    
    while (1) {
        // Naechsten Befehl holen
        char* input = readline("> ");
        
        // Eingegebenen Befehl ermitteln
        if (strcmp(input, "h") == 0) {             // Hilfe
            printf("Es stehen die folgenden Befehle zur Verfuegung:\n");
            printf("b\t\t\tBootable-Flag aendern\n");
            printf("d\t\t\tPartition loeschen\n");
            printf("h\t\t\tHilfe\n");
            printf("l\t\t\tPartitionen anzeigen\n");
            printf("n\t\t\tNeue Partition\n");
            printf("q\t\t\tBeenden\n");
            printf("t\t\t\tPartitionstyp aendern\n");
            printf("tl\t\t\tMoegliche Partitionstypen anzeigen\n");
            printf("w\t\t\tAenderungen auf die Platte schreiben\n");
        } else if (strcmp(input, "q") == 0) {      // Beenden
            if (unsaved_changes == 1) {
                printf("Alle ungespeicherten Aenderungen gehen verloren. "
                    "Wirklich beenden? [Jn] ");
                char answer = keyboard_read_char();
                printf("\n");
                if (answer == 'J') {
                    free(input);
                    break;
                }
            } else {
                free(input);
                break;
            }
        } else if (strcmp(input, "l") == 0) {      // Partitionen auflisten
            shell_list_partitions();
        } else if (strcmp(input, "b") == 0) {      // Bootable Flag aendern
            shell_toggle_bootable();
        } else if (strcmp(input, "w") == 0) {      // aenderungen speichern
            shell_write();
        } else if (strcmp(input, "d") == 0) {      // Partition loeschen
            shell_delete();
        } else if (strcmp(input, "t") == 0) {      // Change partition type
            shell_settype();
        } else if (strcmp(input, "n") == 0) {      // Neue Partition erstellen
            shell_new();
        } else if (strcmp(input, "tl") == 0) {     // Partitionstypen auflisten
            shell_typelist();
        } else {
            printf("Unbekannter Befehl (h fuer Hilfe)\n");
        }
        
        free(input);
    }
}

/**
 * Initialisiert fdisk fuer die angegebene Festplatte und startet anschließend
 * die fdisk-Shell
 */
int main(int argc, char* argv[])
{
    printf("fdisk\nAchtung: Dieses Programm befindet sich noch in "
        "der Entwicklung\n\n");
    
    // Command Line ueberpruefen
    if (argc != 2) {
        printf("Aufruf: fdisk <Platte>\n");
        printf("Beispiel: fdisk ata:/ata00\n");
        return 0;
    }
    
    // Aktuelle Partitionierung einlesen
    if (read_partitions(argv[1])) {
        return 1;
    }
    
    // Anzahl der Heads erfragen
    // TODO Automatisch erledigen
    char* input = readline("Anzahl der Koepfe der Platte [255]: ");
    if (strlen(input)) {
        device_numheads = atoi(input);
    } else {
        device_numheads = 255;
    }
    free(input);
    
    // Initialisierung fertig. Starte den interaktiven Teil
    shell();
    
    // Beenden, kein Fehler aufgetreten
    return 0;
}
