/*  
 * Copyright (c) 2007 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
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

#ifndef _UNISTD_H_
#define _UNISTD_H_
#include <types.h>
#define isatty(x) (1)

/// PID des aktuellen Prozesses auslesen
pid_t getpid();

/// PID des Elternprozesses auslesen
pid_t getppid();

/// Hardlink erstellen
int link(const char* oldpath, const char* newpath);

/// Aktuelles Arbeitsverzeichnis auslesen
char* getcwd(char* dest, size_t size);

/// Arbeitsverzeichnis wechseln
int chdir(const char* path);

/// Prozess sofort und ohne leeren der Buffer loeschen
void _exit(int result);

/// Eine Datei loeschen
int unlink(const char* filename);

/// Ein Verzeichnis loeschen
int rmdir(const char* dirname);

/// Prueft ob der aktuelle Prozess auf die Datei zugreiffen darf
int access(const char *pathname, int mode);

/// Eigentuemer einer Datei aendern
int chown(const char* path, uid_t owner, gid_t group);


// UNIX-Dateifunktionen: ACHTUNG: Emuliert Unix-Dateien =>
// Geschwindigkeitsbremse!

/// Daten aus einer Datei auslesen
ssize_t read(int fd, void* buffer, size_t size);

/// Daten in eine Datei schreiben
ssize_t write(int fd, const void* buffer, size_t size);

/// Unix-Datei schliessen
int close(int fd);

#endif

