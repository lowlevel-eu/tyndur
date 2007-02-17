#ifndef RPC_H
#define RPC_H

#include "types.h"

#define MAX_MESSAGE_HANDLERS 10
#define RPC_FUNCTION_NAME_LENGTH 8

void init_messaging();
bool register_message_handler(char* fnname, void (*handler)(dword, void*));
void register_intr_handler(byte intr, void (*handler)());

#endif
