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
#include "types.h"
#include "rpc.h"
#include "syscall.h"
#include "stdlib.h"
#include "string.h"
#include "collections.h"

typedef struct {
    char* name;
    char* value;
} envvar_t;

list_t* envvar_list;

void rpc_get_envvars(pid_t pid, dword correlation_id, size_t data_size, 
    void* data);

/**
 * Liste mit den Umgebungsvariablen erstellen, und von dem Elternprozess holen
 */
void init_envvars()
{
    envvar_list = list_create();

    //RPC-Handler einrichten, damit allfaellige Kinder-Prozesse die
    // Umgebungsvariablen erben koennen.
    register_message_handler("GET_ENVV", &rpc_get_envvars);
    
    pid_t parent_pid = get_parent_pid();
    //Umgebungsvariablen vom Elternprozess holen, falls dieser ueberhaupt
    // existiert.
    if (parent_pid != 0) {
        response_t* resp = rpc_get_response(parent_pid, "GET_ENVV", 0, NULL);
        if ((resp != NULL) && (resp->data != NULL)) {
            char* buffer_pos = resp->data;
            
            //Hier werden die Daten, welche wir als aneinander gehaengte
            // Strings erhalten wieder voneinander getrennt, und in die Liste
            // mit den Umgebungsvariablen aufgenommen.
            while ((dword) buffer_pos < ((dword) resp->data + 
                resp->data_length))
            {
                char* name = buffer_pos;
                char* value = name + strlen(name) + 1;
                setenv(name, value, 1);

                buffer_pos += strlen(name) + strlen(value) + 2;
            }

        }
    }
}


/**
 * RPC-Handler fuer das Auslesen aller Umgebungsvariabeln dieses Prozesses.
 */
void rpc_get_envvars(pid_t pid, dword correlation_id, size_t data_size,
    void* data)
{
    void* response = NULL;
    size_t response_size = 0;
    size_t name_size, value_size;
    int i = 0;
    envvar_t* envvar;

    p();
    //Die einzelnen Umgebungsvariabeln durchgehen. Dabei werden eifach immer
    // Name und Wert hintereinander, nur durch die Nullbytes getrennt, 
    // kopiert.
    while ((envvar = list_get_element_at(envvar_list, i)) != NULL) {
        //Laenge der Beiden Strings inklusive Nullbyte berechnen.
        name_size = strlen(envvar->name) + 1;
        value_size = strlen(envvar->value) + 1;
        
        //Buffer entsprechend vergroessern
        //TODO: Das koennte man sicherlich noch ein wenig eleganter loesen.
        response = realloc(response, response_size + name_size + value_size);

        //Die beiden Strings kopieren
        memcpy((void*) ((dword) response + response_size), envvar->name,
            name_size);
        memcpy((void*) ((dword) response + response_size + name_size),
            envvar->value, value_size);

        response_size += name_size + value_size;
        i++;
    }
    v();

    //Antwort absenden
    rpc_send_response(pid, correlation_id, response_size, response);

    //Buffer wieder freiegeben
    free(response);
}


/**
 * Pointer auf die Umgebungsvariable zurueckgeben. (Nur intern)
 *
 * @param name Name der Umgebungsvariable
 *
 * @return Pointer auf die Umgebungsvariable, oder NULL wenn sie nicht 
 *          existiert.
 */
static envvar_t* getenvvar(const char* name)
{
    int i = 0;
    envvar_t* envvar;
    
    p();
    while ((envvar = list_get_element_at(envvar_list, i)) != NULL) {
        //Namen vergleichen
        if (strcmp(name, envvar->name) == 0) {
            v();
            return envvar;
        }
        i++;
    }
    v();

    //Umgebungsvariable wurde nicht gefunden
    return NULL;
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
    envvar_t* envvar = getenvvar(name);
    
    if (envvar == NULL) {
        return NULL;
    }
    
    return envvar->value;
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
    envvar_t* envvar = getenvvar(name);

    //Die Umgebungsvariable exisitert noch nicht.
    if (envvar == NULL) {
        envvar = malloc(sizeof(envvar_t));

        //Wenn nicht genuegend Speicher Vorhanden ist, wird abgebrochen
        if (envvar == NULL) {
            return -1;
        }
        
        envvar->name = malloc(strlen(name) + 1);
        envvar->value = malloc(strlen(value) + 1);

        //Wenn nicht genuegend Speicher Vorhanden ist, wird abgebrochen
        if ((envvar->name == NULL) || (envvar->value == NULL)) {
            free(envvar->name);
            free(envvar->value);
            free(envvar);
            return -1;
        }
        
        //Name und Wert kopieren
        memcpy(envvar->name, name, strlen(name) + 1);
        memcpy(envvar->value, value, strlen(value) + 1);

        //In die Liste einfuegen
        envvar_list = list_push(envvar_list, envvar);
    } else {
        char* new_value = malloc(strlen(value) + 1);
        memcpy(new_value, value, strlen(value) + 1);
        
        //Wenn nicht genuegend Speicher Vorhanden ist, wird abgebrochen
        if (value == NULL) {
            return -1;
        }
        
        //Freigeben und ueberschreiben
        free(envvar->value);
        envvar->value = new_value;
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
    if ((value == NULL) || (strlen(value) + 1 > size)) {
        return NULL;
    }
    
    //Inhalt kopieren
    memcpy(dest, value, strlen(value) + 1);
    return dest;
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
    return setenv("CWD", path, 1);
}

