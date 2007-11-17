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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <collections.h>
#include <io.h>
#include <rpc.h>
#include <errno.h>


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
    char* link_filename = io_split_filename(link_path);
    char* link_dirname = io_split_dirname(link_path);
    if ((link_filename == NULL) || (link_dirname == NULL)) {
        errno = ENOMEM;
        result = -1;
        goto end_free_path;
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
    if (link_dir->pid != target_file->pid) {
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
        request->target_id = target_file->id;
        request->dir_id = link_dir->id;

        // RPC durchfuehren und auf Ergebnis warten
        result = rpc_get_int(target_file->pid, "IO_LINK ", size, buffer);
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
    free(link_filename);
    free(link_dirname);
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
    
    // Jetzt werden Datei- und Verzeichnisname geholt
    char* link_dirname = io_split_dirname(link_path);
    char* link_filename = io_split_filename(link_path);
    if ((link_dirname == NULL) || (link_filename == NULL)) {
        errno = ENOMEM;
        result = -1;
        goto end_free_path;
    }

    // Verzeichnis oeffnen, aus dem der Link geloeschen werden soll
    FILE* link_dir = fopen(link_dirname, "rd");
    if (link_dir == NULL) {
        errno = ENOENT;
        result = -1;
        goto end_free_path;
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
        request->dir_id = link_dir->id;

        // RPC durchfuehren und auf Ergebnis warten
        result = rpc_get_int(link_dir->pid, "IO_ULINK", size, buffer);
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

    // Durch Pfade belegten Speicher freigeben
end_free_path:
    free(link_filename);
    free(link_dirname);
    return result;
}

