
#include "types.h"
#include "stdlib.h"
#include "stdio.h"
#include "lostio.h"
#include "ports.h"

#define _USE_START_
#include "init.h"


#include "keyboard.h"
#include "mouse.h"

extern size_t kbd_buffer_pos;

int main(int argc, char* argv[])
{
    request_ports(0x60, 5);

    lostio_init();
    lostio_type_ramfile_use();
    lostio_type_directory_use();
    
    keyboard_init();
    mouse_init();
    
    init_service_register("kbc");
    
    kbd_buffer_pos = 0;
    
    while(1) {
        wait_for_rpc();
    }
}
