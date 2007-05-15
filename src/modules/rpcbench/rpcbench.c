#include "types.h"
#include "syscall.h"
#include "rpc.h"
#include "stdlib.h"
#include "stdio.h"

#define _USE_START_
#include "init.h"


void display_usage();

int client();

int server();
void server_get_data(pid_t pid, dword correlation_id, size_t data_size, void* data);
void server_shutdown(pid_t pid, dword correlation_id, size_t data_size, void* data);

bool server_exit;
char server_buffer[512];


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
    dword bytes_received = 0;
    qword start_time = get_tick_count();
    qword timeout = start_time + 30 * 1000000;
    while (get_tick_count() < timeout) {
        response_t* resp = rpc_get_response(server_pid, "GET_DATA", 0, NULL);
        bytes_received += resp->data_length;
        free(resp);
        free(resp->data);
    }
    dword time_seconds = (dword) (get_tick_count() - start_time) / 1000000;

    puts("Benchmark beendet:");
    printf(" %d Bytes in %d Sekunden transportiert\n", bytes_received, 
        time_seconds);
    printf(" Das entspricht einer Geschwindigket von:\n");
    printf("   %d Bytes pro Sekunde\n", bytes_received / time_seconds);
    printf("   %d Kilobytes pro Sekunde\n", bytes_received / 1024 / 
        time_seconds);
    printf("   %d Megabytes pro Sekunde\n", bytes_received / 1024 / 1024 / 
        time_seconds);
    
    send_message(server_pid, 512, 0, 9, "SHUTDOWN");
    return 0;
}

int server()
{
    register_message_handler("GET_DATA", &server_get_data);
    register_message_handler("SHUTDOWN", &server_shutdown);

    server_exit = FALSE;
    init_service_register("rpcbench");
    while (server_exit == FALSE) {
        wait_for_rpc();
    }

    return 0;
}

void server_get_data(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
   rpc_send_response(pid, correlation_id, 512, server_buffer);
}

void server_shutdown(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    server_exit = TRUE;
}

