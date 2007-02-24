#include "types.h"
#include "syscall.h"
#include "rpc.h"
#include "stdlib.h"

#define _USE_START_
#include "init.h"

#define BUFFER_SIZE 255

void irq_handler();
void rpc_get_buffer(pid_t pid, dword correlation_id, size_t data_size, void* data);


size_t current_buffer_pos = 0;
byte* buffer;

int main(int argc, char* argv[])
{
    //puts("Keyboard");
    init_messaging();
    init_memory_manager();
    init_service_register("keyboard");

    buffer = malloc(BUFFER_SIZE + 1);
    current_buffer_pos = 0;
    
    register_message_handler("GET_BUFF", &rpc_get_buffer);
    register_intr_handler(0x21, &irq_handler);

    while(1) yield();
}



void rpc_get_buffer(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    rpc_send_string_response(pid, correlation_id, (char*)buffer);
}



void irq_handler() {
    dword scancode;

    //puts("Keyboard IRQ.");
    asm volatile (
        "xor %%eax, %%eax;"
        "inb $0x60, %%al;"
    : "=a" (scancode));

    if(scancode < 0x80)
    {
        if(current_buffer_pos + 1 == BUFFER_SIZE)
        {
            printf("\n[KEYBOADRD] Buffer full\n");
        }
        else
        {
            buffer[current_buffer_pos++] = scancode;
            buffer[current_buffer_pos] = 0;
        }
    }
}
