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
#include <types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <syscall.h>
#include <rpc.h>
#include <init.h>

#define BLOCK_SIZE 512

void display_usage(void);

int client(void);
int syscall(void);
void display_stats(uint32_t time, uint64_t data_size);

int server(void);
void server_get_data(pid_t pid, uint32_t correlation_id, size_t data_size,
    void* data);
void server_shutdown(pid_t pid, uint32_t correlation_id, size_t data_size,
    void* data);

bool server_exit;
char buffer[2][BLOCK_SIZE];


int main(int argc, char* argv[])
{
    if (argc != 2) {
        display_usage();
        return -1;
    }
    
    if (strcmp(argv[1], "client") == 0) {
        return client();
    } else if (strcmp(argv[1], "server") == 0) {
        return server();
    } else if (strcmp(argv[1], "sys") == 0) {
        return syscall();
    } else {
        display_usage();
        return -1;
    }
}

void display_usage()
{
    puts("\nAufruf: rpcbench <Modus>");
    puts("\nAls modus kann entweder client oder server gewaehlt werden\n");
}

int client()
{
    //Versuchen an die PID des Serverprozesses zu kommen
    pid_t server_pid = init_service_get("rpcbench");

    if (server_pid == 0) {
        puts("Fehler: Serverprozess konnte nicht gefunden werden!");
        puts("   Wurde er gestartet?");
        return -2;
    }
    
    puts("Starte Benchmark.");

    //Jetzt werden wir 30 Sekunden lang daten vom Serverprozess anfordern
    uint64_t bytes_received = 0;
    uint64_t start_time = get_tick_count();
    uint64_t timeout = start_time + 30 * 1000000;
    while (get_tick_count() < timeout) {
        response_t* resp = rpc_get_response(server_pid, "GET_DATA", 0, NULL);
        bytes_received += resp->data_length;
        free(resp->data);
        free(resp);
    }
    uint32_t time_seconds = (uint32_t) (get_tick_count() - start_time) / 1000000;
    puts("RPC-Benchmark beendet:");
    display_stats(time_seconds, bytes_received);
    
    
    
    start_time = get_tick_count();
    timeout = start_time + 30 * 1000000;
    bytes_received = 0;
    while (get_tick_count() < timeout) {
        memcpy(buffer[0], buffer[1], BLOCK_SIZE);
        bytes_received += BLOCK_SIZE;
    }
    time_seconds = (uint32_t) (get_tick_count() - start_time) / 1000000;
    puts("memcpy-Benchmark beendet:");
    display_stats(time_seconds, bytes_received);

    send_message(server_pid, 512, 0, 9, "SHUTDOWN");
    return 0;
}

#define NUM_SYSCALL_ITERATIONS 500
int syscall(void)
{
    int i;
    uint64_t total, t1, t2;

    // Einfacher Syscall
    total = 0;
    for (i = 0; i < NUM_SYSCALL_ITERATIONS; i++) {
        asm volatile("rdtsc" : "=A" (t1));
        get_tick_count();
        asm volatile("rdtsc" : "=A" (t2));
        total += t2 - t1;
    }
    printf("get_tick_count:     Insgesamt %10lld Takte, Durchschnitt %7lld\n",
        total, total / NUM_SYSCALL_ITERATIONS);

    //Versuchen an die PID des Serverprozesses zu kommen
    pid_t server_pid = init_service_get("rpcbench");

    if (server_pid == 0) {
        puts("Fehler: Serverprozess konnte nicht gefunden werden!");
        puts("   Wurde er gestartet?");
        return -2;
    }

    // Ein RPC zum Server ohne Antwort
    total = 0;
    for (i = 0; i < NUM_SYSCALL_ITERATIONS; i++) {
        asm volatile("rdtsc" : "=A" (t1));
        send_message(server_pid, 1234, 0, 0, NULL);
        asm volatile("rdtsc" : "=A" (t2));
        total += t2 - t1;
    }
    printf("send_message:       Insgesamt %10lld Takte, Durchschnitt %7lld\n",
        total, total / NUM_SYSCALL_ITERATIONS);

    // Ein RPC zum Server und zurück
    total = 0;
    for (i = 0; i < NUM_SYSCALL_ITERATIONS; i++) {
        response_t* resp;
        asm volatile("rdtsc" : "=A" (t1));
        resp = rpc_get_response(server_pid, "GET_DATA", 0, NULL);
        asm volatile("rdtsc" : "=A" (t2));
        free(resp);
        total += t2 - t1;
    }
    printf("rpc_get_response:   Insgesamt %10lld Takte, Durchschnitt %7lld\n",
        total, total / NUM_SYSCALL_ITERATIONS);

    return 0;
}

void display_stats(uint32_t time, uint64_t data_size)
{
    printf(" %lld Bytes in %d Sekunden transportiert\n", data_size, 
        time);
    printf(" Das entspricht einer Geschwindigket von:\n");
    printf("   %lld Bytes pro Sekunde\n", data_size / time);
    printf("   %lld Kilobytes pro Sekunde\n", data_size / 1024 / 
        time);
    printf("   %lld Megabytes pro Sekunde\n", data_size / 1024 / 1024 / 
        time);

}

int server()
{
    register_message_handler("GET_DATA", &server_get_data);
    register_message_handler("SHUTDOWN", &server_shutdown);

    server_exit = false;
    init_service_register("rpcbench");
    while (server_exit == false) {
        wait_for_rpc();
    }

    return 0;
}

void server_get_data(pid_t pid, uint32_t correlation_id, size_t data_size,
    void* data)
{
   rpc_send_response(pid, correlation_id, BLOCK_SIZE, buffer[0]);
}

void server_shutdown(pid_t pid, uint32_t correlation_id, size_t data_size,
    void* data)
{
    server_exit = true;
}

