#include <syscall.h>
#include "types.h"
#include "stdlib.h"
#include "stdio.h"
#include "lostio.h"
#include "ports.h"
#include "init.h"


#include "keyboard.h"
#include "mouse.h"

int main(int argc, char* argv[])
{
    request_ports(0x60, 1);
    request_ports(0x64, 1);

    lostio_init();
    lostio_type_ramfile_use();
    lostio_type_directory_use();
    
    keyboard_init();
    mouse_init();
    
    init_service_register("kbc");
    
    while(1) {
        wait_for_rpc();
    }
}
