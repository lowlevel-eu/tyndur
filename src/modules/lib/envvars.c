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

#include <stdint.h>
#include "types.h"
#include "rpc.h"
#include "syscall.h"
#include "stdlib.h"
#include "string.h"
#include "collections.h"
#include <io.h>
#include <errno.h>
#include <env.h>
#include <assert.h>
#include <lock.h>
#include <init.h>

typedef struct {
    char* name;
    char* value;
} envvar_t;


static mutex_t envvar_lock = 0;
static list_t* envvar_list;

/**
 * Liste mit den Umgebungsvariablen erstellen, und von dem Elternprozess holen
 */
void init_envvars(void* ppb, size_t ppb_size)
{
    envvar_list = list_create();
    if (ppb != NULL) {
        ppb_extract_env(ppb, ppb_size);
    }
}

/**
 * Pointer auf die Umgebungsvariable zurueckgeben. (Nur intern)
 * Der Aufrufer muss envvar_lock halten.
 *
 * @param name Name der Umgebungsvariable
 *
 * @return Pointer auf die Umgebungsvariable, oder NULL wenn sie nicht
 *          existiert.
 */
static envvar_t* getenvvar(const char* name)
{
    int i = 0;
    envvar_t* envvar = NULL;

    p();
    assert(envvar_lock);
    while ((envvar = list_get_element_at(envvar_list, i)) != NULL) {
        //Namen vergleichen
        if (strcmp(name, envvar->name) == 0) {
            goto out;
        }
        i++;
    }

out:
    v();
    return envvar;
}


/**
 * Wert einer Umgebungsvariable auslesen
 *
 * @param name Name der Umgebungsvariable
 *
 * @return Pointer auf den Inhalt, oder NULL wenn sie nicht existiert.
 */
char* getenv(const char* name)
{
    envvar_t* envvar;
    char* value = NULL;

    p();
    mutex_lock(&envvar_lock);
    envvar = getenvvar(name);
    if (envvar == NULL) {
        goto out;
    }

    value = envvar->value;
out:
    mutex_unlock(&envvar_lock);
    v();
    return value;
}

/**
 * Wert einer Umgebungsvariable auslesen
 *
 * @param index Index der Umgebungsvariable
 *
 * @return Pointer auf den Inhalt, oder NULL wenn sie nicht existiert.
 */
char* getenv_index(int index)
{
    envvar_t* envvar;
    char* value = NULL;

    p();
    mutex_lock(&envvar_lock);
    envvar = list_get_element_at(envvar_list, index);
    if (envvar == NULL) {
        goto out;
    }

    value = envvar->value;
out:
    mutex_unlock(&envvar_lock);
    v();
    return value;
}

/**
 * Name einer Umgebungsvariable auslesen
 *
 * @param index Index der Umgebungsvariable
 *
 * @return Pointer auf den Namen, oder NULL wenn die Variable nicht existiert.
 */
char* getenv_name_by_index(int index)
{
    envvar_t* envvar;
    char* name = NULL;

    p();
    mutex_lock(&envvar_lock);
    envvar = list_get_element_at(envvar_list, index);
    if (envvar == NULL) {
        goto out;
    }

    name = envvar->name;
out:
    mutex_unlock(&envvar_lock);
    v();
    return name;
}

/**
 * Anzahl der Umgebungsvariablen abfragen
 *
 * @return Anzahl der definierten Umgebungsvariablen
 */
int getenv_count()
{
    return list_size(envvar_list);
}


/**
 * Wert einer Umgebungsvariable setzen oder eine anlegen.
 *
 * @param name Name der Umgebungsvariable
 * @param value Der neue Wert
 * @param overwrite Gibt an, ob die Umgebungsvariable ueberschrieben werden
 *                  soll, wenn sie schon exisitert. 0 = Nicht ueberschreiben.
 *
 * @return 0 Wenn der Wert gesetzt wurde. -1 Wenn nicht genug Speicher
 *          vorhanden war.
 */
int setenv(const char* name, const char* value, int overwrite)
{
    envvar_t* envvar;
    int ret = -1;

    mutex_lock(&envvar_lock);
    envvar = getenvvar(name);

    //Die Umgebungsvariable exisitert noch nicht.
    if (envvar == NULL) {
        envvar = malloc(sizeof(envvar_t));

        //Wenn nicht genuegend Speicher Vorhanden ist, wird abgebrochen
        if (envvar == NULL) {
            goto out;
        }

        envvar->name = malloc(strlen(name) + 1);
        envvar->value = malloc(strlen(value) + 1);

        //Wenn nicht genuegend Speicher Vorhanden ist, wird abgebrochen
        if ((envvar->name == NULL) || (envvar->value == NULL)) {
            free(envvar->name);
            free(envvar->value);
            free(envvar);
            goto out;
        }

        //Name und Wert kopieren
        memcpy(envvar->name, name, strlen(name) + 1);
        memcpy(envvar->value, value, strlen(value) + 1);

        //In die Liste einfuegen
        envvar_list = list_push(envvar_list, envvar);
    } else if (overwrite) {
        char* new_value = malloc(strlen(value) + 1);
        memcpy(new_value, value, strlen(value) + 1);

        //Wenn nicht genuegend Speicher Vorhanden ist, wird abgebrochen
        if (value == NULL) {
            goto out;
        }

        //Freigeben und ueberschreiben
        free(envvar->value);
        envvar->value = new_value;
    }

    ret = 0;
out:
    mutex_unlock(&envvar_lock);
    return ret;
}

/**
 * Umgebungsvariable setzen
 *
 * @param str String in der Form variable=wert
 *
 * @return 0 bei Erfolg, -1 im Fehlerfall
 */
int putenv(const char* str)
{
    char* sep = strchr(str, '=');

    // Wenn kein Gleichheitszeichen gefunden wurde, wird abgebrochen
    if (sep == NULL) {
        return -1;
    }


    {
        // Name herauskopieren
        size_t namelen = sep - str;
        char name[namelen + 1];
        memcpy(name, str, namelen);
        name[namelen] = 0;

        // Variable setzen
        if (setenv(name, str + namelen + 1, 1) == -1) {
            errno = ENOMEM;
            return -1;
        }
    }

    return 0;
}

/**
 * Umgebungsvariable loeschen.
 *
 * @param name
 */
void unsetenv(const char* name)
{
    int i = 0;
    envvar_t* envvar;

    p();
    mutex_lock(&envvar_lock);
    while ((envvar = list_get_element_at(envvar_list, i)) != NULL) {
        //Namen vergleichen
        if (strcmp(name, envvar->name) == 0) {
            //Speicher freigeben
            free(envvar->name);
            free(envvar->value);
            free(envvar);

            //Aus der Liste loeschen
            list_remove(envvar_list, i);
            break;
        }
        i++;
    }
    mutex_unlock(&envvar_lock);
    v();
}


/**
 * Aktuelles Arbeitsverzeichnis des Prozesses ausfindig machen
 * @param dest Pointer auf den Buffer, in den der Pfad hineinkopiert werden
 *     soll.
 * @param size Gibt an, wie gross der Buffer ist
 *
 * @return dest, wenn der Pfad fehlerfrei in den Buffer kopiert wurde. NULL
 *          falls ein Fehler aufgetreten ist (z.B. Buffer zu klein, oder kein
 *          CWD gesetzt).
 */
char* getcwd(char* dest, size_t size)
{
    //Das aktuelle Arbeitsverzeichnis, CWD, ist in der Umgebungsvariable CWD
    // gespeichert, und wird vom Elternprozess geerbt.
    char* value = getenv("CWD");

    //Ueberpruefen, ob die Variable ueberhaupt gesetzt ist, und ob der Inhalt
    //in den Buffer passt. Sonst wird abgebrochen.
    if (value == NULL) {
        return NULL;
    }

    //Wenn dest == NULL wird Speicher alloziiert
    if (dest == NULL) {
        size = strlen(value) + 1;
        dest = malloc(strlen(value) + 1);
    }

    //Wenn der Buffer gross genug ist, werden die Daten dorthin
    // kopiert.
    if (strlen(value) + 1 <= size) {
        //Wenn der Buffer gross genug ist, werden die Daten dorthin
        // kopiert.

        //Inhalt kopieren
        memcpy(dest, value, strlen(value) + 1);
        return dest;
    }

    return NULL;
}


/**
 * Aktuelles Arbeitsverzeichnis des Prozesses festlegen.
 *
 * @param path Pointer auf den Pfad
 *
 * @return 0 wenn der Pfad fehlerfrei gesetzt wurde, sonst 1.
 */
int chdir(const char* path)
{
    //TODO: Hier muesste der Pfad geprueft werden, und ggf. errno entsprechend
    // gesetzt werden.
    //
    char* normalized_path = io_get_absolute_path(path);
    int result = setenv("CWD", normalized_path, 1);
    free(normalized_path);

    return result;
}
