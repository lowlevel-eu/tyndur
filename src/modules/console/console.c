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
  char* name;
  byte width;
  byte height;
  char* data[];
} console_t;

FILE* keyboard_resource;

//Hier koennen die Debug-Nachrichten aktiviert werden
#define DEBUG_MSG(s) printf("[ CONSOLE ] debug: '%s'\n", s)
//#define DEBUG_MSG(s) //

bool create_pre_open(char** path, byte unknown, pid_t pid,io_resource_t* io_res);

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
    // console-Typehandle. Zugriff auf einzelne Konsole
    typehandle_t* typehandle_console = malloc(sizeof(typehandle_t));
    
    typehandle_console->id          = 255;
    typehandle_console->not_found   = NULL;
    typehandle_console->pre_open    = NULL;
    typehandle_console->post_open   = NULL;
    typehandle_console->read        = NULL;
    typehandle_console->write       = NULL;
    typehandle_console->seek        = NULL;
    typehandle_console->close       = NULL;
    lostio_register_typehandle(typehandle_console);
    
    // create-Typehandle. Erstellt Konsole bei Öffnung
    typehandle_t* typehandle_create = malloc(sizeof(typehandle_t));
    
    typehandle_create->id          = 254;
    typehandle_create->not_found   = &create_pre_open;
    typehandle_create->pre_open    = &create_pre_open;
    typehandle_create->post_open   = NULL;
    typehandle_create->read        = NULL;
    typehandle_create->write       = NULL;
    typehandle_create->seek        = NULL;
    typehandle_create->close       = NULL;
    lostio_register_typehandle(typehandle_create);
    DEBUG_MSG("Init - Typehandles registriert");
    
    // vfstree bauen
    const char* version = "Version 0.0.1";
    vfstree_create_node("/version", LOSTIO_TYPES_RAMFILE, strlen(version) + 1, (void*)version,0);
    vfstree_create_node("/create",254, 0, NULL, LOSTIO_FLAG_BROWSABLE);
    vfstree_create_node("/consoles",255,0, NULL, LOSTIO_FLAG_BROWSABLE);
    DEBUG_MSG("Init - vfstree eingerichtet!");
    
    // Keyboard öffnen
    keyboard_resource = fopen("keyboard:/ascii","r");
    if (keyboard_resource == NULL)
    {
        // Keyboard konnte nicht geöffnet werden. Fehler!
        DEBUG_MSG("Init - Keyboard konnte nicht geoeffnet Werden. FEHLER!");
        return;
    }
    DEBUG_MSG("Init - Keyboard geoeffnet!");
    
    
    DEBUG_MSG("Modul geladen.");
    
    
    // bei init registrieren
    init_service_register("console");
        
    while(1) yield();
}

bool create_pre_open(char** path, byte unknown, pid_t pid,io_resource_t* io_res)
{
    char* console_name = NULL;
    //console_name = malloc(strlen(path))
    printf("Create-Handler for %s (Length %d)\n",(*path),strlen(*path));
    
    return FALSE;
}

