#include "types.h"
#include "syscall.h"
#include "rpc.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#define _USE_START_
#include "init.h"
#include "lostio.h"

// Einzelne Konsole.
typedef struct 
{
  pid_t pid;
  byte width;
  byte height;
  FILE* stdin;
  FILE* stdout;
  FILE* stderr;
} console_t;

FILE* keyboard_resource;

list_t* consoles;

//Hier koennen die Debug-Nachrichten aktiviert werden
#define DEBUG_MSG(s) printf("[ CONSOLE ] debug: '%s'\n", s)
//#define DEBUG_MSG(s) //

size_t stdouterr_write (lostio_filehandle_t *, size_t, size_t, void *);
// read_hdl_reply stdin_read (lostio_filehandle_t *file, size_t blocksize, size_t blockcount);

int main(int argc, char* argv[])
{
    
    
    // LostIO initialisieren
    lostio_init();
    lostio_type_ramfile_use();
    lostio_type_directory_use();
    
    // bei init registrieren
    init_service_register("console");
    
    DEBUG_MSG("Init - LostIO initialisiert");
    
    // Typehandle für Konsolen einrichten
    // stdin-Typehandle
    typehandle_t* typehandle_stdin = malloc(sizeof(typehandle_t));
    
    typehandle_stdin->id          = 255;
    typehandle_stdin->not_found   = NULL;
    typehandle_stdin->pre_open    = NULL;
    typehandle_stdin->post_open   = NULL;
    //typehandle_stdin->read        = &stdin_read;
    typehandle_stdin->write       = NULL;
    typehandle_stdin->seek        = NULL;
    typehandle_stdin->close       = NULL;
    lostio_register_typehandle(typehandle_stdin);
    
    // stdout-Typehandle
    typehandle_t* typehandle_stdout = malloc(sizeof(typehandle_t));
    
    typehandle_stdout->id          = 254;
    typehandle_stdout->not_found   = NULL;
    typehandle_stdout->pre_open    = NULL;
    typehandle_stdout->post_open   = NULL; 
    typehandle_stdout->read        = NULL; 
    typehandle_stdout->write       = &stdouterr_write; // auf stdout wird geschrieben (TODO)
    typehandle_stdout->seek        = NULL;
    typehandle_stdout->close       = NULL;
    lostio_register_typehandle(typehandle_stdout);
    
    // stderr-Typehandle
    typehandle_t* typehandle_stderr = malloc(sizeof(typehandle_t));
    typehandle_stderr->id           = 253;
    typehandle_stderr->not_found    = NULL;
    typehandle_stderr->pre_open     = NULL; 
    typehandle_stderr->post_open    = NULL;
    typehandle_stderr->read         = NULL; 
    typehandle_stderr->write        = &stdouterr_write; // auf stderr wird geschrieben (TODO)
    typehandle_stderr->seek         = NULL; 
    typehandle_stderr->close        = NULL; 
    lostio_register_typehandle(typehandle_stderr);
    
    DEBUG_MSG("Init - Typehandles registriert");
    
    // vfstree bauen
    vfstree_create_node("/stdin" , 255, 0, NULL, 0);
    vfstree_create_node("/stdout", 254, 0, NULL, 0);
    vfstree_create_node("/stderr", 253, 0, NULL, 0);
    DEBUG_MSG("Init - vfstree eingerichtet!");
    
    /*
    // Keyboard öffnen
    *keyboard_resource = fopen("keyboard:/ascii","r");
    if (keyboard_resource == NULL)
    {
        // Keyboard konnte nicht geöffnet werden. Fehler!
        DEBUG_MSG("Init - Keyboard konnte nicht geoeffnet Werden. FEHLER!");
        return 1;
    }
    DEBUG_MSG("Init - Keyboard geoeffnet!");
<<<<<<< .mine
    */
    
    consoles = list_create();
    DEBUG_MSG("Init - Liste erstellt.");
    
    DEBUG_MSG("Modul geladen.");
    
    
    // bei init registrieren
    init_service_register("console");
    
    while(1) yield();
}

size_t stdouterr_write (lostio_filehandle_t *file, size_t blocksize, size_t blockcount, void *data)
{
    int consolectr = 0;
    console_t *console = NULL;
    while ((console = list_get_element_at(consoles,consolectr++)) != NULL)
    {
        if (console->pid == file->pid)
        {
          //printf("Konsole in Liste gefunden!");
          break;
        }
    }
    if (console)
    {
        printf("%d hat schonmal auf console:/ zugegriffen!\n",file->pid);
    }
    else
    {
        printf("%d benötigt eine neue Konsole...\n",file->pid);
        console = malloc(sizeof(console_t));
        console->pid = file->pid;
        console->stdout = NULL;
        console->stdin  = NULL;
        console->stderr = NULL;
        list_push(consoles,console);
    }
    size_t totalsize = blockcount * blocksize;
    if (!strcmp(file->node->name,"stdout"))
    {
        //printf("(PID %d) %s",file->pid,data);
        return fwrite(data,blocksize,blockcount,console->stdout);
    } else {
        //printf("(PID %d) ERROR: %s",file->pid,data);
        return fwrite(data,blocksize,blockcount,console->stderr);
    }
    return 0;
}

/*
read_hdl_reply stdin_read (lostio_filehandle_t *file, size_t blocksize, size_t blockcount)
{
    size_t totalsize = blocksize * blockcount;
    void *return_data = malloc(totalsize);
        size_t returnsize = fread(console->stdin,blocksize,blockcount);
    read_hdl_reply *ret;
    ret->data = return_data;
    ret->size = returnsize;
    return *ret;
}
*/
