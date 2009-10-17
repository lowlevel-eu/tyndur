/**
 * Copyright (c) 2009 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Paul Lange.
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

#ifndef COMMAND_H
#define COMMAND_H

// listet die Dateien aus dem aktuellen Verzeichniss des FTP-Servers auf
void ftp_ls(void);

// auf dem FTP-Server einloggen
void ftp_user(void);

// erzeugt ein Verzeichniss auf dem FTP-Server
// @param Pfadname+Dateiname
void ftp_mkdir(const char*);

// loescht eine Datei auf dem FTP-Server
// @param Pfadname+Dateiname
void ftp_rm(const char*);

// loescht ein Verzeichnis auf dem FTP-Server
// @param Pfadname
void ftp_rmdir(const char*);

// wechselt das Verzeichniss auf dem FTP-Server
// @param Pfadname
void ftp_cd(const char*);

// zeigt das Hauptverzeichniss auf dem FTP-Server an
void ftp_cdup(void);

// gibt den Namen des Betriebssystems vom FTP-Server aus
void ftp_sys(void);

// zeigt das akutell geoeffnete Verzeichniss auf dem FTP-Server an
void ftp_pwd(void);

// uebertraegt eine Datei auf den FTP-Server
// @param Pfadname+Dateiname
void ftp_get(const char*);

// speichert eine Datei vom FTP-Server
// @param Pfadname+Dateiname
void ftp_put(const char*);

#endif
