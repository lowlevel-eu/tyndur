#ifndef RPC_H
#define RPC_H

#include "types.h"

#define MAX_MESSAGE_HANDLERS 10
#define RPC_FUNCTION_NAME_LENGTH 8

#define RPC_MESSAGE 512
#define RPC_RESPONSE 513

typedef void (*handler_function_t)(dword, dword, dword, void*);

typedef struct {
    dword pid;
    dword correlation_id;
    dword data_length;
    void* data;
} response_t;

extern handler_function_t rpc_response_handler;

void init_messaging();
bool register_message_handler(char* fnname, handler_function_t handler);
void register_intr_handler(byte intr, void (*handler)());

dword rpc_get_dword(dword pid, char* function_name, dword data_length, char* data);
int rpc_get_int(dword pid, char* function_name, dword data_length, char* data);
char* rpc_get_string(dword pid, char* function_name, dword data_length, char* data);
response_t* rpc_get_response(dword pid, char* function_name, dword data_length, char* data);

void rpc_send_response(dword pid, dword correlation_id, dword len, char* data);
void rpc_send_dword_response(dword pid, dword correlation_id, dword response);
void rpc_send_int_response(dword pid, dword correlation_id, int response);
void rpc_send_string_response(dword pid, dword correlation_id, char* response);

#endif
