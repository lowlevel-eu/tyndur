#include "types.h"
#include "syscall.h"
#include "rpc.h"
#include "stdlib.h"
#include "stdio.h"

#define _NO_STDIO_
#define _USE_START_
#include "init.h"
#include "lostio.h"

//#define DEBUG_MSG(s) printf("[  VTERM  ] debug: '%s'\n", s)
#define DEBUG_MSG(s) //

size_t echo_write(lostio_filehandle_t *file, size_t blocksize, size_t blockcount, void *data);

int main(int argc, char* argv[])
{
    init_service_register("vterm");
    lostio_init();
    lostio_type_ramfile_use();
    lostio_type_directory_use();
    DEBUG_MSG("LostIO initialisiert");
    
    // echo-Typehandle
    typehandle_t* typehandle_echo = malloc(sizeof(typehandle_t));
    
    typehandle_echo->id          = 255;
    typehandle_echo->not_found   = NULL;
    typehandle_echo->pre_open    = NULL;
    typehandle_echo->post_open   = NULL; 
    typehandle_echo->read        = NULL; 
    typehandle_echo->write       = &echo_write; // auf stdout wird geschrieben (TODO)
    typehandle_echo->seek        = NULL;
    typehandle_echo->close       = NULL;
    lostio_register_typehandle(typehandle_echo);
        
    vfstree_create_node("/echo",255,0,NULL,0);
    
    DEBUG_MSG("Module loaded.");
    while(1) yield();
}

size_t echo_write(lostio_filehandle_t *file, size_t blocksize, size_t blockcount, void *data)
{
    /*int i = 0;
    for (; i < blocksize * blockcount; i++)
    {
        putchar(*(data+i));
    }*/

    char *buffer = malloc(blocksize * blockcount + 1);
    strncpy(buffer, data, blocksize*blockcount);
    buffer[blocksize * blockcount] = '\0';
    printf("%s", buffer);
    free(buffer);

    return blocksize * blockcount;
}
