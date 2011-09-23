/*
 * Copyright (c) 2008 The tyndur Project. All rights reserved.
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
#include <stdlib.h>
#include <stdio.h>
#include <syscall.h>
#include <string.h>
#include <dir.h>
#include <assert.h>
#include <collections.h>
#include <sleep.h>
#include "servmgr.h"

#define TMS_MODULE config
#include <tms.h>

static list_t* config_list;
char* full_config_path;

/**
 * Alle Vorkommen von einer Zeichenkette in einem gegebenen Puffer ersetzen.
 *
 * Der alte Puffer muss per malloc alloziert worden sein und wird freigegeben.
 * Dafuer wird ein neuer Puffer mit der neuen Stringlaenge alloziert.
 */
static void buffer_replace(char** buf, const char* search, const char* replace)
{
    size_t len, i, j;
    size_t search_len = strlen(search);
    size_t replace_len = strlen(replace);
    char* old = *buf;
    char* new;
    int found = 0;

    len = 0;
    for (i = 0; i < strlen(old); i++) {
        if (!strncmp(old + i, search, search_len)) {
            len += replace_len;
            i += search_len - 1;
            found = 1;
        } else {
            len++;
        }
    }

    if (!found) {
        return;
    }

    new = malloc(len + 1);
    new[len] = '\0';
    for (i = 0, j = 0; i < strlen(old); i++) {
        if (!strncmp(old + i, search, search_len)) {
            memcpy(new + j, replace, replace_len);
            j += replace_len;
            i += search_len - 1;
        } else {
            new[j] = old[i];
            j++;
        }
    }

    free(old);
    *buf = new;
}

/**
 * Dateiinhalt auslesen und den Angegebenen Pointer auf einen Buffer mit dem
 * Inhalt setzen. Wenn die Datei nicht geoeffnet werden konnte, wird er auf
 * NULL gesetzt.
 *
 * @param path Pfad zur Datei
 * @param dest Pointer auf den Pointer, er gesetzt werden soll
 */
static void config_read_file(const char* path, char** ptr)
{
    FILE* f = fopen(path, "r");
    if (!f) {
        *ptr = NULL;
    } else {
        // FIXME: Das koennte man noch etwas eleganter loesen ;-)
        char buffer[513];
        size_t size;

        size = fread(buffer, 1, sizeof(buffer) - 1, f);
        buffer[size] = 0;

        // Einen eventuellen Zeilenumbruch entfernen
        if (buffer[size - 1] == '\n') {
            buffer[size - 1] = 0;
        }

        *ptr = strdup(buffer);
        assert(*ptr != NULL);

        buffer_replace(ptr, "$ROOT", root_dir);

        fclose(f);
    }
}

/**
 * Datei mit Abhaengigkeiten fuer einen Service einlesen
 *
 * @param path Pfad zur Datei
 * @param conf_serv Pointer auf die Kofigurationstruktur
 */
static void config_parse_deps(const char* path,
    struct config_service* conf_serv)
{
    struct config_service* dep;
    FILE* f;
    char buffer[33];

    conf_serv->deps = list_create();

    f = fopen(path, "r");
    if (!f) {
        return;
    }

    // FIXME: Das koennte man noch etwas eleganter loesen ;-)
    while (fgets(buffer, sizeof(buffer), f)) {
        size_t i = strlen(buffer);
        if ((i > 0) && (buffer[i - 1] == '\n')) {
            buffer[i - 1] = '\0';
        }

        dep = config_service_get(buffer);
        if (dep == NULL) {
            printf(TMS(service_depends, "servmgr: Service '%s' haengt ab von "
                "'%s'; Dieser ist aber nicht vorhanden in der "
                "Konfiguration!\n"), conf_serv->name, buffer);
            continue;
        }

        list_push(conf_serv->deps, dep);
    }
    fclose(f);
}

/**
 * Datei mit Konfiguration fuer einen Service einlesen
 *
 * @param path Pfad zur Datei
 * @param conf_serv Pointer auf die Kofigurationstruktur
 */
static void config_parse_conf(const char* path,
    struct config_service* conf_serv)
{
    FILE* f;
    char buffer[33];

    conf_serv->conf.wait = SERVSTART;

    f = fopen(path, "r");
    if (!f) {
        return;
    }

    // FIXME: Das koennte man noch etwas eleganter loesen ;-)
    while (fgets(buffer, sizeof(buffer), f)) {
        size_t i = strlen(buffer);
        if ((i > 0) && (buffer[i - 1] == '\n')) {
            buffer[i - 1] = '\0';
        }

        if (!strcmp(buffer, "nowait")) {
            conf_serv->conf.wait = NOWAIT;
        } else if (!strcmp(buffer, "waitterminate")) {
            conf_serv->conf.wait = TERMINATE;
        }
    }
    fclose(f);
}


/**
 * Konfigurationsverzeichnis parsen
 *
 * @param dir Verzeichnis-Handle
 *
 * @return true wenn erfolgreich abgeschlossen wurde, false sonst.
 */
static bool config_dir_parse(struct dir_handle* dir)
{
    io_direntry_t* entry;
    struct config_service* conf_serv;
    int i;

    config_list = list_create();

    // In einem ersten Schritt werden alle Servicenamen eingelesen
    while ((entry = directory_read(dir))) {
        // Alles was nicht Verzeichnis ist, wird uebersprungen
        if ((entry->type != IO_DIRENTRY_DIR) || (entry->name[1] == '.')) {
            free(entry);
            continue;
        }

        // Service in der Liste eintragen
        conf_serv = calloc(1, sizeof(*conf_serv));
        assert(conf_serv != NULL);

        // Namen setzen
        conf_serv->name = strdup(entry->name);
        assert(conf_serv->name != NULL);

        list_push(config_list, conf_serv);

        free(entry);
    }

    // In einem zweiten schritt werden die Befehle zum Starten und die
    // Abhaengigkeiten eingetragen
    for (i = 0; (conf_serv = list_get_element_at(config_list, i)); i++) {
        int result;
        char* cmd_path;
        char* deps_path;
        char* conf_path;

        result = asprintf(&cmd_path, "%s/%s/cmd", full_config_path, conf_serv->
            name);
        assert(result != -1);
        result = asprintf(&deps_path, "%s/%s/deps", full_config_path,
            conf_serv->name);
        assert(result != -1);
        result = asprintf(&conf_path, "%s/%s/conf", full_config_path,
            conf_serv->name);
        assert(result != -1);

        config_read_file(cmd_path, &conf_serv->start_cmd);
        config_parse_deps(deps_path, conf_serv);
        config_parse_conf(conf_path, conf_serv);
        free(deps_path);
        free(cmd_path);
        free(conf_path);
    }

    return true;
}


bool config_read()
{
    const char* config_path = "/config/servmgr";
    struct dir_handle* dir;
    bool result;
    int ires;

    ires = asprintf(&full_config_path, "%s%s", root_dir, config_path);
    assert(ires != -1);


    // Verzeichnis mit der Konfiguration oeffnen
    dir = directory_open(full_config_path);
    if (!dir) {
        printf(TMS(dir_open, "servmgr: Konnte Konfigurationsverzeichnis nicht "
            "oeffnen: '%s'\n"), full_config_path);
        return false;
    }

    result = config_dir_parse(dir);
    directory_close(dir);
    return result;
}

struct config_service* config_service_get(const char* name)
{
    struct config_service* conf_serv;
    int i;

    for (i = 0; (conf_serv = list_get_element_at(config_list, i)); i++) {
        if (!strcmp(name, conf_serv->name)) {
            return conf_serv;
        }
    }
    return NULL;
}
