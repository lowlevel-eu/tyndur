/*
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Kevin Wolf.
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
#include "rpc.h"
#include "types.h"
#include "collections.h"
#include "syscall.h"
#include "string.h"
#include "stdlib.h"
#include <lock.h>

#undef DEBUG
#ifdef DEBUG
    #include "stdio.h"
#endif

/**
 * Liste aller empfangenen, aber noch nicht verarbeiteten Antworten auf
 * synchrone RPC-Nachrichten
 */
static list_t* responses = NULL;
static mutex_t responses_lock = 0;

/** Zuordnungs-ID für die nächste zu versendende Nachricht */
static volatile uint32_t current_correlation_id = 0;

void sync_rpc_response_handler(pid_t caller_pid, uint32_t correlation_id,
    size_t data_length, void* data);

/**
 * Inititalisiert die Verarbeitung synchroner RPCs. Diese Funktion muss in
 * Anwendungen nicht explizit aufgerufen werden, dies erledigen die anderen
 * Funktionen für synchrone RPCs.
 *
 * Bei der Intialisierung wird responses auf eine leere Liste gesetzt und der
 * rpc_response_handler gesetzt.
 */
void init_sync_messages(void)
{
    responses = list_create();
    rpc_response_handler = &sync_rpc_response_handler;
}

/**
 * Verarbeitet eine eingehende RPC-Antwort: Der responses-Liste wird ein Eintrag
 * hinzugefügt, der die Antwort enthält.
 */
void sync_rpc_response_handler(pid_t caller_pid, uint32_t correlation_id,
    size_t data_length, void* data)
{
    response_t* response;
    void* saved_data;

    if (!responses) {
        init_sync_messages();
    }

#ifdef DEBUG
    FILE* old_stdout = stdout;
    stdout = NULL;
    printf("[%d] sync_rpc_response_handler(%x)\n", get_pid(), correlation_id);
    stdout = old_stdout;
#endif

    response = malloc(sizeof(response_t));
    response->pid = caller_pid;
    response->correlation_id = correlation_id;
    //printf("response  corr_id:%d pid:%d data_length:%d\n", response->correlation_id, caller_pid, data_length);
    response->data_length = data_length;

    saved_data = malloc(data_length);
    memcpy(saved_data, data, data_length);
    response->data = saved_data;

    p();
    mutex_lock(&responses_lock);
    list_push(responses, response);
    mutex_unlock(&responses_lock);
    v();
}

/**
 * Prüft, ob auf eine bestimmte RPC-Anfrage bereits eine Antwort eingetroffen
 * ist. Wenn ja, wird die Antwort zurückgegeben und aus der responses-Liste
 * gelöscht.
 *
 * @param pid PID des Tasks, von dem die Antwort erwartet wird
 * @param correlation_id Zuordnungs-ID der gesendeten Nachricht
 *
 * @return Die bereits eingetroffene Antwort oder NULL, wenn die Antwort noch
 * nicht eingetroffen ist.
 */
static response_t* sync_rpc_has_response(pid_t pid, uint32_t correlation_id)
{
    int index = 0;
    response_t* response = NULL;

    p();
    mutex_lock(&responses_lock);
    index = 0;
    while((response = list_get_element_at(responses, index))) {
        //printf("response  corr_id:%d  %d  pid:%d  %d       index: %d\n", response->correlation_id, correlation_id,response->pid, pid, index);
        if (response->correlation_id == correlation_id) {
            list_remove(responses, index);
            if (response->pid == pid) {
                break;
            }
        }
        index++;
    }
    mutex_unlock(&responses_lock);
    v();

    if ((response != NULL) && (response->correlation_id == correlation_id) && (response->pid == pid)) {
        return response;
    } else {
        return NULL;
    }
}

/**
 * Führt einen synchronen RPC durch, d.h. sendet eine Anfrage und wartet dann
 * solange, bis sync_rpc_has_response eine Antwort zurückgibt
 *
 * @param pid PID des aufgerufenen Prozesses
 * @param function_name Aufgerufener RPC-Funktionsname, der vom aufgerufenen
 * Prozess zur Verfügung gestellt wird
 * @param data_length Länge des data-Parameters in Bytes
 * @param data Zusätzliche Parameter an die RPC-Funktion
 *
 * @return Empfangene Antwort
 */
static response_t* sync_rpc(pid_t pid, char* function_name, size_t data_length,
                            char* data)
{
    p();

    uint32_t correlation_id = locked_increment(&current_correlation_id);

#ifdef DEBUG
    FILE* old_stdout = stdout;
    stdout = NULL;
    pid_t mypid = get_pid();
#endif

    if (!responses) {
        init_sync_messages();
    }

    char rpc_data[RPC_FUNCTION_NAME_LENGTH + data_length];
    strncpy(rpc_data, function_name, RPC_FUNCTION_NAME_LENGTH);
    memcpy(rpc_data + RPC_FUNCTION_NAME_LENGTH, data, data_length);

#ifdef DEBUG
    printf("[%d] sync_rpc_begin(%x) zu %d\n", mypid, correlation_id, pid);
#endif

    // Während dem RPC selbst nicht blockieren, da es sonst bei Self-RPC dazu
    // führt, dass Code geblockt ist, der dazu nicht vorgesehen ist.
    v();
    send_message(pid, 512, correlation_id, data_length + RPC_FUNCTION_NAME_LENGTH, rpc_data);
    p();

    response_t* response;
    while ((response = sync_rpc_has_response(pid, correlation_id)) == NULL)
    {
#ifdef DEBUG
        printf("[%d] sync_rpc_before_wait(%x)\n", mypid, correlation_id);
#endif
        // Zwischen Auswertung der while-Bedingung und wait_for_rpc darf der
        // Handler nicht aufgerufen werden. Das v und das wait_for_rpc müssen
        // genau gleichzeitig kommen (Vereinigung in einen Syscall)
        v_and_wait_for_rpc();
        p();
#ifdef DEBUG
        printf("[%d] sync_rpc_after_wait(%x)\n", mypid, correlation_id);
#endif
    }

#ifdef DEBUG
    printf("[%d] sync_rpc_end(%x) zu %d\n", mypid, correlation_id, pid);
    stdout = old_stdout;
#endif
    v();

    return response;
}

/**
 * Führt einen RPC durch und liefert einen dword als Ergebnis zurück
 *
 * @param pid PID des aufgerufenen Prozesses
 * @param function_name Aufgerufener RPC-Funktionsname, der vom aufgerufenen
 * Prozess zur Verfügung gestellt wird
 * @param data_length Länge des data-Parameters in Bytes
 * @param data Zusätzliche Parameter an die RPC-Funktion
 *
 * @return Rückgabewert der RPC-Funktion als dword oder 0 im Fehlerfall.
 */
uint32_t rpc_get_dword(pid_t pid, char* function_name, size_t data_length,
    char* data)
{
    uint32_t value = 0;
    response_t* response = sync_rpc(pid, function_name, data_length, data);

   if (response && response->data_length >= sizeof(uint32_t)) {
       value = *((uint32_t*) response->data);
   }

   free(response->data);
   free(response);
   return value;
}

/**
 * Führt einen RPC durch und liefert einen int als Ergebnis zurück
 *
 * @param pid PID des aufgerufenen Prozesses
 * @param function_name Aufgerufener RPC-Funktionsname, der vom aufgerufenen
 * Prozess zur Verfügung gestellt wird
 * @param data_length Länge des data-Parameters in Bytes
 * @param data Zusätzliche Parameter an die RPC-Funktion
 *
 * @return Rückgabewert der RPC-Funktion als int oder 0 im Fehlerfall.
 */
int rpc_get_int(pid_t pid, char* function_name, size_t data_length, char* data)
{
   int value = 0;
   response_t* response = sync_rpc(pid, function_name, data_length, data);

   if (response && response->data_length >= sizeof(int)) {
       value = *((int*) response->data);
   }

   free(response->data);
   free(response);
   return value;
}

/**
 * Führt einen RPC durch und liefert einen String als Ergebnis zurück
 *
 * @param pid PID des aufgerufenen Prozesses
 * @param function_name Aufgerufener RPC-Funktionsname, der vom aufgerufenen
 * Prozess zur Verfügung gestellt wird
 * @param data_length Länge des data-Parameters in Bytes
 * @param data Zusätzliche Parameter an die RPC-Funktion
 *
 * @return Rückgabewert der RPC-Funktion als char* oder NULL im Fehlerfall.
 * Der Aufrufer ist selbst dafür verantwortlich, den durch den zurückgegebenen
 * String reservierten Speicherplatz freizugeben.
 */
char* rpc_get_string(pid_t pid, char* function_name, size_t data_length, char* data)
{
    char* value = NULL;
    response_t* response = sync_rpc(pid, function_name, data_length, data);

    if (response == NULL)
        return NULL;

    uint32_t actual_len = strnlen(response->data, response->data_length);
    value = malloc(actual_len + 1);
    strncpy(value, response->data, actual_len);
    value[actual_len] = '\0';

    free(response->data);
    free(response);
    return value;
}

/**
 * Führt einen RPC durch und liefert das Ergebnis zurück
 *
 * @param pid PID des aufgerufenen Prozesses
 * @param function_name Aufgerufener RPC-Funktionsname, der vom aufgerufenen
 * Prozess zur Verfügung gestellt wird
 * @param data_length Länge des data-Parameters in Bytes
 * @param data Zusätzliche Parameter an die RPC-Funktion
 *
 * @return Rückgabewert der RPC-Funktion als response_t*. Der Aufrufer ist
 * selbst dafür zuständig, den dadurch belegten Speicherplatz wieder
 * freizugeben (Dies ist neben der response_t-Datenstruktur vor allem das
 * data-Array)
 */
response_t* rpc_get_response(pid_t pid, char* function_name, size_t data_length, char* data)
{
   response_t* response = sync_rpc(pid, function_name, data_length, data);
   return response;
}

/**
 * Sendet eine Antwortnachricht für einen synchronen RPC-Aufruf. Die Antwort
 * besteht dabei aus beliebigen Daten.
 *
 * @param pid PID des Prozesses, an den die Antwort geschickt werden soll
 * @param correlation_id Zuordnungs-ID der Nachricht. Die Zuordnungs-ID der
 * empfangenen RPC-Anfrage stimmt immer mit der Zuordnungs-ID der Antwort
 * überein.
 * @param len Länge der Antwort in Bytes
 * @param data Zu übertragende Daten
 */
void rpc_send_response(pid_t pid, uint32_t correlation_id, size_t len,
    char* data)
{
    send_message(pid, RPC_RESPONSE, correlation_id, len, data);
}

/**
 * Sendet eine Antwortnachricht für einen synchronen RPC-Aufruf. Die Antwort
 * besteht dabei nur aus einem dword.
 *
 * @param pid PID des Prozesses, an den die Antwort geschickt werden soll
 * @param correlation_id Zuordnungs-ID der Nachricht. Die Zuordnungs-ID der
 * empfangenen RPC-Anfrage stimmt immer mit der Zuordnungs-ID der Antwort
 * überein.
 * @param response Zu sendender Rückgabewert
 */
void rpc_send_dword_response(pid_t pid, uint32_t correlation_id,
    uint32_t response)
{
    send_message(pid, RPC_RESPONSE, correlation_id, sizeof(response), (char*) &response);
}

/**
 * Sendet eine Antwortnachricht für einen synchronen RPC-Aufruf. Die Antwort
 * besteht dabei nur aus einem int.
 *
 * @param pid PID des Prozesses, an den die Antwort geschickt werden soll
 * @param correlation_id Zuordnungs-ID der Nachricht. Die Zuordnungs-ID der
 * empfangenen RPC-Anfrage stimmt immer mit der Zuordnungs-ID der Antwort
 * überein.
 * @param response Zu sendender Rückgabewert
 */
void rpc_send_int_response(pid_t pid, uint32_t correlation_id, int response)
{
    send_message(pid, RPC_RESPONSE, correlation_id, sizeof(response), (char*) &response);
}

/**
 * Sendet eine Antwortnachricht für einen synchronen RPC-Aufruf. Die Antwort
 * besteht dabei nur aus einem nullterminierten String.
 *
 * @param pid PID des Prozesses, an den die Antwort geschickt werden soll
 * @param correlation_id Zuordnungs-ID der Nachricht. Die Zuordnungs-ID der
 * empfangenen RPC-Anfrage stimmt immer mit der Zuordnungs-ID der Antwort
 * überein.
 * @param response Zu sendender Rückgabewert
 */
void rpc_send_string_response(pid_t pid, uint32_t correlation_id,
    char* response)
{
    send_message(pid, RPC_RESPONSE, correlation_id, strlen(response), response);
}
