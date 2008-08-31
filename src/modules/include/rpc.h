#ifndef RPC_H
#define RPC_H

#include "types.h"

#define MAX_MESSAGE_HANDLERS 32
#define RPC_FUNCTION_NAME_LENGTH 8

#define RPC_SIGNALS_START 256
#define RPC_SIGNALS_END 511
#define RPC_MESSAGE 512
#define RPC_RESPONSE 513
#define RPC_TIMER 514

#define RPC_TRPC_FIRST 1024
#define RPC_TRPC_LAST 4095

typedef void (*handler_function_t)(pid_t, dword, size_t, void*);


typedef struct {
    dword pid;
    dword correlation_id;
    size_t data_length;
    void* data;
} response_t;

extern handler_function_t rpc_response_handler;

void init_messaging(void);
bool register_message_handler(char* fnname, handler_function_t handler);
void register_intr_handler(byte intr, void (*handler)(void));

dword rpc_get_dword(pid_t pid, char* function_name, size_t data_length, char* data);
int rpc_get_int(pid_t pid, char* function_name, size_t data_length, char* data);
char* rpc_get_string(pid_t pid, char* function_name, size_t data_length, char* data);
response_t* rpc_get_response(pid_t pid, char* function_name, size_t data_length, char* data);

void rpc_send_response(pid_t pid, dword correlation_id, size_t len, char* data);
void rpc_send_dword_response(pid_t pid, dword correlation_id, dword response);
void rpc_send_int_response(pid_t pid, dword correlation_id, int response);
void rpc_send_string_response(pid_t pid, dword correlation_id, char* response);

dword timer_register(void (*callback)(void), dword usec);
void timer_cancel(dword timer_id);

#endif
