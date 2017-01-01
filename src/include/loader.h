/*
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Antoine Kaufmann.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#ifndef _LOADER_H_
#define _LOADER_H_

#include <types.h>
#include <stdbool.h>

/// Ueberprueft ob die Datei vom Typ ELF32 ist
bool loader_is_elf32(vaddr_t image_start, size_t image_size);

/// Laedt ein ELF32-Image
bool loader_elf32_load_image(pid_t process, vaddr_t image_start,
    size_t image_size);


/// Ueberprueft ob die Datei vom Typ ELF64 ist
bool loader_is_elf64(vaddr_t image_start, size_t image_size);

/// Laedt ein ELF64-Image
bool loader_elf64_load_image(pid_t process, vaddr_t image_start,
    size_t image_size);


/// Laedt eine flache Binaerdatei
bool loader_load_flat_bin_image(pid_t process, vaddr_t image_start,
    size_t image_size);


/// Ausfuehrbare Datei laden
bool loader_load_image(pid_t process, vaddr_t image_start, size_t image_size);



// Hilfsfunktionen fuer den Loader:

/**
 * Speicher allozieren um ihn spaeter in einen neuen Prozess zu mappen. Diese
 * Funktion sollte nicht fuer "normale" Allokationen benutzt werden, da immer
 * ganze Pages alloziert werden.
 *
 * @param size minimale Groesse des Bereichs
 *
 * @return Adresse, oder NULL falls ein Fehler aufgetreten ist
 */
vaddr_t loader_allocate_mem(size_t size);

/**
 * Ein Stueck Speicher in den Prozess mappen. Dieser darf dazu noch nicht
 * gestartet sein. Der Speicher muss zuerst mit loader_allocate_mem alloziert
 * worden sein, denn sonst kann nicht garantiert werden, dass der Speicher
 * uebertragen werden kann.
 *
 * @param process PID des Prozesses
 * @param dest_address Adresse an die der Speicher im Zielprozess soll
 * @param src_address Adresse im aktuellen Kontext die uebetragen werden soll
 * @param size Groesse des Speicherbereichs in Bytes
 *
 * @return TRUE, wenn der bereich gemappt wurde, FALSE sonst
 */
bool loader_assign_mem(pid_t process, vaddr_t dest_address,
    vaddr_t src_address, size_t size);

/**
 * Lädt eine Shared Library in den Speicher.
 *
 * @param name Name der Shared Library
 * @param image Enthält bei Erfolg einen Pointer auf ein Binärimage
 * @param size Enthält bei Erfolg die Größe des Binärimages in Bytes
 *
 * @return 0 bei Erfolg, -errno im Fehlerfall
 */
int loader_get_library(const char* name, void** image, size_t* size);

/**
 * Erstellt einen neuen Thread.
 *
 * @param process PID
 * @param address Einsprungsadresse des Threads
 *
 * @return bool TRUE, wenn der Thread erstellt wurde, FALSE sonst
 */
bool loader_create_thread(pid_t process, vaddr_t address);


#endif //ifndef _LOADER_H_
