#include "rpc.h"
#include "types.h"
#include "collections.h"
#include "syscall.h"
#include "string.h"
#include "stdlib.h"

list_t* responses = NULL;
static dword current_correlation_id = 0;

void sync_rpc_response_handler(dword correlation_id, pid_t caller_pid, size_t data_length, void* data);

void init_sync_messages()
{
    responses = list_create();
    rpc_response_handler = &sync_rpc_response_handler;
}

void sync_rpc_response_handler(dword correlation_id, pid_t caller_pid, size_t data_length, void* data)
{
    response_t* response;
    void* saved_data;
    
    p();
    if (!responses) {
        init_sync_messages();
    }

    response = malloc(sizeof(response));
    response->pid = caller_pid;
    response->correlation_id = correlation_id;
    response->data_length = data_length;

    saved_data = malloc(data_length);
    memcpy(saved_data, data, data_length);
    response->data = saved_data;

    list_push(responses, response);
    v();
}

static response_t* sync_rpc_has_response(pid_t pid, dword correlation_id)
{
    int index;
    response_t* response;
    
    p();
    for(index = 0; (response = list_get_element_at(responses, index)); index++) {
        if (response->correlation_id == correlation_id) {
            list_remove(responses, index);

            if (response->pid == pid) {
                break;
            }
        }
    }
    v();
    
    return response;
}

static response_t* sync_rpc(pid_t pid, char* function_name, size_t data_length, char* data)
{
    dword correlation_id = current_correlation_id++;    
    
    if (!responses) {
        p();
        init_sync_messages();
        v();
    }
    
    char* rpc_data = malloc(RPC_FUNCTION_NAME_LENGTH + data_length);
    strncpy(rpc_data, function_name, RPC_FUNCTION_NAME_LENGTH);
    memcpy(rpc_data + RPC_FUNCTION_NAME_LENGTH, data, data_length);
    
    send_message(pid, 512, correlation_id, data_length + RPC_FUNCTION_NAME_LENGTH, rpc_data);

    response_t* response;
    do {
        yield();
        response = sync_rpc_has_response(pid, correlation_id);
    } while(!response);

    return response;
}

dword rpc_get_dword(pid_t pid, char* function_name, size_t data_length, char* data)
{
   dword value = 0;
   response_t* response = sync_rpc(pid, function_name, data_length, data);

   if (response && response->data_length >= sizeof(dword)) {
       value = *((dword*) response->data);
   }
   
   free(response->data);
   free(response);
   return value;
}

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

char* rpc_get_string(pid_t pid, char* function_name, size_t data_length, char* data)
{
   char* value = NULL;
   response_t* response = sync_rpc(pid, function_name, data_length, data);

   dword actual_len = strnlen(response->data, response->data_length);
   if (response && (actual_len < response->data_length)) {
       value = malloc(actual_len + 1);
       strncpy(value, data, actual_len);
       value[actual_len] = '\0';
   }
   
   free(response->data);
   free(response);
   return value;
}

response_t* rpc_get_response(pid_t pid, char* function_name, size_t data_length, char* data)
{
   response_t* response = sync_rpc(pid, function_name, data_length, data);
   return response;
}

void rpc_send_response(pid_t pid, dword correlation_id, size_t len, char* data)
{
    send_message(pid, RPC_RESPONSE, correlation_id, len, data);
}

void rpc_send_dword_response(pid_t pid, dword correlation_id, dword response)
{
    send_message(pid, RPC_RESPONSE, correlation_id, sizeof(response), (char*) &response);
}

void rpc_send_int_response(pid_t pid, dword correlation_id, int response)
{
    send_message(pid, RPC_RESPONSE, correlation_id, sizeof(response), (char*) &response);
}

void rpc_send_string_response(pid_t pid, dword correlation_id, char* response)
{
    send_message(pid, RPC_RESPONSE, correlation_id, strlen(response), response);
}
