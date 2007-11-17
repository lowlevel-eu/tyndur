#include <types.h>
#include "syscall.h"
#include <stdio.h>
#include "lostio.h"

#define _NO_STDIO_
#define _USE_START_
#include "init.h"

#define LOSTIO_TYPES_OUT 255

void con_init();
void con_ansi_putc(char c);
void con_putsn(unsigned int n, const char* s);
void con_puts(const char* s);
void con_clear_screen();


size_t terminal_write(lostio_filehandle_t* handle, size_t blocksize,
    size_t blockcount, void* data);


int main(int argc, char* argv[])
{
    // Videotreiber initialisieren
    con_init();
    
    lostio_init();
    lostio_type_directory_use();

    typehandle_t* typehandle = malloc(sizeof(typehandle_t));
    typehandle->id          = LOSTIO_TYPES_OUT;
    typehandle->not_found   = NULL;
    typehandle->pre_open    = NULL;
    typehandle->post_open   = NULL;
    typehandle->read        = NULL;
    typehandle->write       = &terminal_write;
    typehandle->seek        = NULL;
    typehandle->close       = NULL;
    typehandle->link        = NULL;
    typehandle->unlink      = NULL;
    lostio_register_typehandle(typehandle);
    vfstree_create_node("/echo" , LOSTIO_TYPES_OUT, 0, NULL, 0);
    
    init_service_register("textterm");
    while(TRUE) {
        wait_for_rpc();
	}
}

size_t terminal_write(lostio_filehandle_t* file, size_t blocksize,
    size_t blockcount, void* data)
{
    con_putsn(blocksize * blockcount, (char*) data);
    return blocksize * blockcount;
}


