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

#ifndef _UNISTD_H_
#define _UNISTD_H_
#include <sys/types.h>
#include <lost/config.h>

#define __unistd_only
#include <getopt.h>
#undef __unistd_only

/// Nummer der des Posix-Deskriptors fuer stdin
#define STDIN_FILENO 0

/// Nummer der des Posix-Deskriptors fuer stdout
#define STDOUT_FILENO 1

/// Nummer der des Posix-Deskriptors fuer stderr
#define STDERR_FILENO 2

/// Fuer ed
#define _PC_PATH_MAX 4096


#ifdef __cplusplus
extern "C" {
#endif

///  Prüft, ob es sich um ein Terminal handelt
int isatty(int fd);

/// PID des aktuellen Prozesses auslesen
pid_t getpid(void);

/// PID des Elternprozesses auslesen
pid_t getppid(void);

/// TID des aktuellen Threads auslesen
tid_t gettid(void);

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

/// Ziel eines symbolischen Links auslesen
ssize_t readlink(const char* path, char* buf, size_t bufsize);

// Makros fuer access()
#define F_OK 1
#define R_OK 2
#define W_OK 4
#define X_OK 8

/// Prueft ob der aktuelle Prozess auf die Datei zugreiffen darf
int access(const char *pathname, int mode);

/// Eigentuemer einer Datei aendern
int chown(const char* path, uid_t owner, gid_t group);

/// Warten bis zeit abgelaufen ist, oder Signal eintrifft
unsigned int sleep(unsigned int sec);

// UNIX-Dateifunktionen: ACHTUNG: Emuliert Unix-Dateien =>
// Geschwindigkeitsbremse!

/// Daten aus einer Datei auslesen
ssize_t read(int fd, void* buffer, size_t size);

/// Daten in eine Datei schreiben
ssize_t write(int fd, const void* buffer, size_t size);

/// Datei-Zeiger verschieben
off_t lseek(int fd, off_t offset, int whence);

/// Unix-Datei schliessen
int close(int fd);

/// In eine Datei mit gegebenem Offset lesen
ssize_t pread(int fd, void *buf, size_t count, off_t offset);

/// In eine Datei mit gegebenem Offset schreiben
ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset);

/// Timer setzen nach dem ein SIGALARM gesendet werden soll
long alarm(long seconds);

/// Gibt die Groesse einer Page zurueck
int getpagesize(void);

/**
 * Einen symbolischen Link anlegen
 *
 * @param oldpath Ziel des Links
 * @param newpath Hier wird der Link erstellt
 *
 * @return 0 bei Erfolg; im Fehlerfall -1 und errno wird gesetzt
 */
int symlink(const char* oldpath, const char* newpath);

#ifndef CONFIG_LIBC_NO_STUBS
/// Prozess klonen
pid_t fork(void);

/// Pipe einrichten
int pipe(int mode[2]);

/// Ersetzt das aktuelle Prozessimage
int execvp(const char* path, char* const argv[]);

/// Dateideskriptor duplizieren
int dup(int fd);

/// Dateideskriptor duplizieren
int dup2(int fd, int new_fd);

/// Datei umbenennen oder verschieben
int rename(const char* path_old, const char* path_new);

/// Gibt die aktuelle Benutzer-ID zurueck
uid_t getuid(void);

/// Gibt die effektive Benutzer-ID zurueck
uid_t geteuid(void);

/// Gibt die aktuelle Gruppen-ID zurueck
gid_t getgid(void);
//
/// Gibt die effektive Gruppen-ID zurueck
gid_t getegid(void);

/// Setzt die Gruppen-ID
int setgid(gid_t gid);

/// Setzt die Benutzer-ID
int setuid(uid_t uid);

/// Setzt die effektive Benutzer-ID
int seteuid(uid_t euid);

/// Setzt die effektive Gruppen-ID
int setegid(gid_t egid);

/// Gibt den Hostnamen dieses Rechners zurueck
int gethostname(char* buf, size_t len);

#endif

#ifdef __cplusplus
}; // extern "C"
#endif

#endif

