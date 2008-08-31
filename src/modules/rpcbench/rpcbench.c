#include "types.h"
#include "syscall.h"
#include "rpc.h"
#include "stdlib.h"
#include "stdio.h"
#include "init.h"

#define BLOCK_SIZE 512

void display_usage(void);

int client(void);
void display_stats(dword time, dword data_size);

int server(void);
void server_get_data(pid_t pid, dword correlation_id, size_t data_size, void* data);
void server_shutdown(pid_t pid, dword correlation_id, size_t data_size, void* data);

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
        free(resp->data);
        free(resp);
    }
    dword time_seconds = (dword) (get_tick_count() - start_time) / 1000000;
    puts("RPC-Benchmark beendet:");
    display_stats(time_seconds, bytes_received);
    
    
    
    start_time = get_tick_count();
    timeout = start_time + 30 * 1000000;
    bytes_received = 0;
    while (get_tick_count() < timeout) {
        memcpy(buffer[0], buffer[1], BLOCK_SIZE);
        bytes_received += BLOCK_SIZE;
    }
    time_seconds = (dword) (get_tick_count() - start_time) / 1000000;
    puts("memcpy-Benchmark beendet:");
    display_stats(time_seconds, bytes_received);

    send_message(server_pid, 512, 0, 9, "SHUTDOWN");
    return 0;
}

void display_stats(dword time, dword data_size)
{
    printf(" %d Bytes in %d Sekunden transportiert\n", data_size, 
        time);
    printf(" Das entspricht einer Geschwindigket von:\n");
    printf("   %d Bytes pro Sekunde\n", data_size / time);
    printf("   %d Kilobytes pro Sekunde\n", data_size / 1024 / 
        time);
    printf("   %d Megabytes pro Sekunde\n", data_size / 1024 / 1024 / 
        time);

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
   rpc_send_response(pid, correlation_id, BLOCK_SIZE, buffer[0]);
}

void server_shutdown(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    server_exit = TRUE;
}

