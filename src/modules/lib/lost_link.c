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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <collections.h>
#include <lostio.h>
#include <io.h>
#include <rpc.h>
#include <errno.h>

static int lio2_symlink(const char* target, const char* name,
    const char* parent_path)
{
    lio_resource_t parent;
    lio_resource_t link;

    if ((parent = lio_resource(parent_path, 1)) < 0) {
        return -1;
    }

    if ((link = lio_mksymlink(parent, name, target)) < 0) {
        return -1;
    }

    return 0;
}

static int lio_symlink(const char* link_path, const char* target_path)
{
    io_resource_t* link_dir = lio_compat_open(link_path,
        IO_OPEN_MODE_WRITE | IO_OPEN_MODE_CREATE |
        IO_OPEN_MODE_TRUNC | IO_OPEN_MODE_LINK);

    if (link_dir == NULL) {
        return -1;
    }

    lio_compat_write(target_path, 1, strlen(target_path) + 1, link_dir);
    lio_compat_close(link_dir);

    return 0;
}

/**
 * Link erstellen
 *
 * @param target_path Pfad auf den der Link zeigen soll
 * @param link_path Pfad an dem der Link erstellt werden soll
 * @param hardlink TRUE, falls ein Hardlink erstellt werden soll, FALSE sonst
 *
 * @return 0 bei Erfolg, im Fehlerfall -1 und errno wird entsprechend gesetzt
 */
int io_create_link(const char* target_path, const char* link_path,
    bool hardlink)
{
    int result;
    FILE* target_file;
    FILE* link_dir;

    // Jetzt werden Datei- und Verzeichnisname geholt
    char* abs_linkpath = io_get_absolute_path(link_path);
    char* link_filename = io_split_filename(abs_linkpath);
    char* link_dirname = io_split_dirname(abs_linkpath);
    if ((link_filename == NULL) || (link_dirname == NULL)) {
        errno = ENOMEM;
        result = -1;
        goto end_free_path;
    }

    if (!hardlink) {
        return (lio2_symlink(target_path, link_filename, link_dirname) &&
            lio_symlink(link_path, target_path) ? -1 : 0);
    }


    // Link-Ziel oeffnen
    target_file = fopen(target_path, "r");
    if (target_file == NULL) {
        // Wenn das Oeffnen nicht klappt ist die Datei nicht vorhanden,
        // folglich kann auch kein Link darauf erstellt werden ;-)
        errno = ENOENT;
        result = -1;
        goto end_free_path;
    }
    
    // Verzeichnis oeffnen, in dem der Link angelegt werden soll
    link_dir = fopen(link_dirname, "rd");
    if (link_dir == NULL) {
        errno = ENOENT;
        result = -1;
        goto end_close_file;
    }
    
    // Wenn die Beiden nicht im Selben Treiber liegen, ist der Fall eh
    // erledigt.
    if (link_dir->res->pid != target_file->res->pid) {
        errno = EXDEV;
        result = -1;
        goto end_close_dir;
    }

    // Hier muss ein Block hin, weil gcc sonst mit dem buffer-Array und den
    // gotos durcheinander kommt
    {
        // Groesse der RPC-Daten errechnen
        size_t link_len = strlen(link_filename) + 1;
        size_t size = sizeof(io_link_request_t) + link_len + 1;
        char buffer[size];

        // Netten Pointer auf den Buffer eirichten
        io_link_request_t* request = (io_link_request_t*) buffer;
    
        // Pfad kopieren
        request->name_len = link_len - 1;
        memcpy(request->name, link_filename, link_len);
    
        // Ziel eintragen
        request->target_id = target_file->res->id;
        request->dir_id = link_dir->res->id;

        // RPC durchfuehren und auf Ergebnis warten
        result = rpc_get_int(target_file->res->pid, "IO_LINK ", size, buffer);
        switch (result) {
            // Ziel oder Verzeichnis nicht in Ordnung
            case -1:
                errno = ENOENT;
                result = -1;
                break;
        
            // Kein Link-Handler eingetragen
            case -2:
                errno = EPERM;
                result = -1;
                break;

            // RPC-Daten ungueltig
            case -3:
                errno = EFAULT;
                result = -1;
                break;
            
            // Fehler im Handler
            case -4:
                // Nicht korrekt, aber irgendwas muss hier genommen werden
                errno = EPERM;
                result = -1;
                break;
        }
    }


    // Geoeffnetes Link-Ziel und Link-Verzeichnis schliessen
end_close_dir:
    fclose(link_dir);
end_close_file:
    fclose(target_file);

end_free_path:
    // Durch Pfade belegten Speicher freigeben
    free(abs_linkpath);
    free(link_filename);
    free(link_dirname);
    return result;
}

static int lio2_remove_link(const char* link_dirname, const char* link_filename)
{
    lio_resource_t parent;

    if ((parent = lio_resource(link_dirname, 1)) < 0) {
        return -1;
    }

    return lio_unlink(parent, link_filename);
}

static int lio_remove_link(char* link_dirname, char* link_filename)
{
    int result;
    FILE* link_dir = fopen(link_dirname, "rd");
    if (link_dir == NULL) {
        errno = ENOENT;
        return -1;
    }

    // Auch hier wieder ein Block, damit gcc keine Probleme macht
    {
        // Groesse der RPC-Daten errechnen
        size_t link_len = strlen(link_filename) + 1;
        size_t size = sizeof(io_unlink_request_t) + link_len + 1;
        char buffer[size];

        // Netten Pointer auf den Buffer eirichten
        io_unlink_request_t* request = (io_unlink_request_t*) buffer;
    
        // Pfad kopieren
        request->name_len = link_len - 1;
        memcpy(request->name, link_filename, link_len);
    
        // Verzeichnis eintragen
        request->dir_id = link_dir->res->id;

        // RPC durchfuehren und auf Ergebnis warten
        result = rpc_get_int(link_dir->res->pid, "IO_ULINK", size, buffer);
        switch (result) {
            // Ziel oder Verzeichnis nicht in Ordnung
            case -1:
                errno = ENOENT;
                result = -1;
                break;
        
            // Kein Unlink-Handler eingetragen
            case -2:
                errno = EPERM;
                result = -1;
                break;

            // RPC-Daten ungueltig
            case -3:
                errno = EFAULT;
                result = -1;
                break;
            
            // Fehler im Handler
            case -4:
                // Nicht korrekt, aber irgendwas muss hier genommen werden
                errno = EPERM;
                result = -1;
                break;
        }

    }

    // Geoeffnetes Link-Verzeichnis schliessen
    fclose(link_dir);

    return result;
}

/**
 * Link loeschen
 *
 * @param link_path Pfad der geloescht werden soll
 *
 * @return 0 bei Erfolg, im Fehlerfall -1 und errno wird entsprechend gesetzt
 */
int io_remove_link(const char* link_path)
{
    int result;
    char* abs_path;
    char* link_dirname;
    char* link_filename;

    // Jetzt werden Datei- und Verzeichnisname geholt
    abs_path = io_get_absolute_path(link_path);
    link_dirname = io_split_dirname(abs_path);
    link_filename = io_split_filename(abs_path);
    if (!abs_path || !link_dirname || !link_filename) {
        errno = ENOMEM;
        result = -1;
        goto end_free_path;
    }

    result = lio2_remove_link(link_dirname, link_filename);
    if (result) {
        result = lio_remove_link(link_dirname, link_filename);
    }

    // Durch Pfade belegten Speicher freigeben
end_free_path:
    free(abs_path);
    free(link_filename);
    free(link_dirname);
    return result;
}

