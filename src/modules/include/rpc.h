#ifndef RPC_H
#define RPC_H

#include "types.h"

void init_messaging();
bool register_message_handler(char* fnname, void (*handler)(int, void*));
void register_intr_handler(byte intr, void (*handler)());

#endif
