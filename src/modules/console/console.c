#include "types.h"
#include "syscall.h"
#include "rpc.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#define _NO_STDIO_
#define _USE_START_
#include "init.h"
#include "lostio.h"

// Einzelne Konsole.
typedef struct 
{
    pid_t pid;
    byte width;
    byte height;
    FILE *stdin;
    FILE *stdout;
    FILE *stderr;
} console_t;

typedef struct
{
    pid_t pid;
    enum {
        STDIN,
        STDOUT,
        STDERR 
    } type;
} console_ctrl_t;

//Hier koennen die Debug-Nachrichten aktiviert werden
//#define DEBUG_MSG(s) printf("[ CONSOLE ] debug: '%s'\n", s)
#define DEBUG_MSG(s) //

size_t stdouterr_write (lostio_filehandle_t *, size_t, size_t, void *);
read_hdl_reply stdin_read (lostio_filehandle_t *file, size_t blocksize, size_t blockcount);
void rpc_console_set(pid_t pid, dword correlation_id, size_t data_size, void* data);

list_t* consoles;
FILE* keyboard_resource;
FILE* echo_resource;

int main(int argc, char* argv[])
{
    //int i = 0;
    //for (i = 0; i < 100; i++)
    //  yield(1);
    
    // LostIO initialisieren
    lostio_init();
    lostio_type_ramfile_use();
    lostio_type_directory_use();
    
    
    
    DEBUG_MSG("Init - LostIO initialisiert");
    
    // Typehandle für Konsolen einrichten
    // stdin-Typehandle
    // TODO: Globale Variablen tun es auch
    typehandle_t* typehandle_stdin = malloc(sizeof(typehandle_t));
    
    typehandle_stdin->id          = 255;
    typehandle_stdin->not_found   = NULL;
    typehandle_stdin->pre_open    = NULL;
    typehandle_stdin->post_open   = NULL;
    typehandle_stdin->read        = &stdin_read;
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
    
    register_message_handler("CONS_SET", &rpc_console_set);
    DEBUG_MSG("Init - Messagehandler/RPC eingerichtet!");
    
    
    qword timeout = get_tick_count() + 5000000;
    while((init_service_get("vterm") == 0) || (init_service_get("keyboard") == 0))
    {
        yield();
        if(get_tick_count() > timeout)
        {
            if(init_service_get("vterm") == 0)
            {
                puts("[ CONSOLE ] Konnte vterm nicht finden!");
            }

            if(init_service_get("keyboard") == 0)
            {
                puts("[ CONSOLE ] Konnte keyboard nicht finden!");
            }


            return -1;
        }
    }
     

    
    // Keyboard �ffnen
    keyboard_resource = fopen("keyboard:/ascii","r");
    if (keyboard_resource == NULL)
    {
        // Keyboard konnte nicht ge�ffnet werden. Fehler!
        DEBUG_MSG("Init - Keyboard konnte nicht geoeffnet Werden. FEHLER!");
        return 1;
    }
    DEBUG_MSG("Init - Keyboard geoeffnet!");
                                                                                       
    // echo öffnen
    echo_resource = fopen("vterm:/echo","w");
    if (echo_resource == NULL)
    {
        // Keyboard konnte nicht geöffnet werden. Fehler!
        DEBUG_MSG("Init - vterm:/echo konnte nicht geoeffnet Werden. FEHLER!");
        return 1;
    }
    DEBUG_MSG("Init - vterm:/echo geoeffnet!");
    
    consoles = list_create();
    DEBUG_MSG("Init - Liste erstellt.");
    
    DEBUG_MSG("Modul geladen.");
    
    // bei init registrieren
    init_service_register("console");
    
    
    while(TRUE) {
        wait_for_rpc();
    }
}

/**
 * Gibt eine Konsole f�r den �bergebenen Proze� zur�ck, Wenn dem Proze� 
 * bereits eine Konsole zugeordnet war, wird diese zur�ckgegeben, ansonsten
 * wird eine neue erzeugt
 *
 * @param pid PID des Prozesses, dessen Konsole zur�ckgegeben werden soll
 */
static console_t* get_console(pid_t pid)
{
    int consolectr = 0;
    console_t *console = NULL;

    // Pr�fen, ob der Proze� bereits eine Konsole hat
    while ((console = list_get_element_at(consoles, consolectr++)) != NULL)
    {
        if (console->pid == pid)
        {
            //DEBUG_MSG("Prozess hat schonmal auf console:/ zugegriffen!");
            return console;
        }
    }

    // Wenn nicht, dann mu� ein neue Konsole her
    DEBUG_MSG("Prozess benoetigt eine neue Konsole...");

    console = malloc(sizeof(console_t));
    console->pid = pid;
    console->stdout = echo_resource;
    console->stdin  = keyboard_resource;
    console->stderr = echo_resource;
    list_push(consoles, console);

    return console;
}


size_t stdouterr_write (lostio_filehandle_t *file, size_t blocksize, size_t blockcount, void *data)
{
    console_t *console = get_console(file->pid);
    
    if (!strcmp(file->node->name,"stdout"))
    {
        //printf("(PID %d) %d\n", file->pid, blocksize*blockcount);
        return fwrite(data,blocksize,blockcount,console->stdout);
    } else {
        //printf("(PID %d) ERROR: %s",file->pid,data);
        return fwrite(data,blocksize,blockcount,console->stderr);
    }
    return 0;
}


read_hdl_reply stdin_read (lostio_filehandle_t *file, size_t blocksize, size_t blockcount)
{
    console_t *console = get_console(file->pid);
    
    read_hdl_reply ret;

    size_t totalsize = blocksize * blockcount;
    
    //FIXME:Das ist nur ein Workarund, um das Problem mit der nicht gefreeten
    // Rueckgabe zu umgehen.
    // FIXME Fehlender Nullcheck nach realloc, Memleak im Fehlerfall
    static void *return_data = NULL;
    return_data = realloc(return_data, totalsize);
    
    size_t returnsize = fread(return_data,blocksize,blockcount,console->stdin);

    
    ret.data = return_data;
    ret.size = returnsize;

    return ret;
}

void rpc_console_set(pid_t pid, dword correlation_id, size_t data_size, void* data)
{
    console_ctrl_t *command = data;
    console_t *console = get_console(command->pid);

    data      += sizeof(console_ctrl_t);
    data_size -= sizeof(console_ctrl_t);

    // Wir brauchen einen nullterminierten String f�r den Dateinamen
    char filename[data_size];
    filename[data_size] = 0;
    strncpy(filename, data, data_size);

    // Au�erdem brauchen wir das neu zu setzende Filehandle
    FILE** handle;
    switch (command->type)
    {
        case STDIN:
            handle = &console->stdin;
            break;

        case STDOUT:
            handle = &console->stdout;
            break;

        case STDERR:
            handle = &console->stderr;
            break;

        default:
            return;
    }
    
    // Setzen des neuen Handles
    if (*handle != NULL)
    {
        fclose(*handle);
    }
    *handle = fopen((char*) data,"r");
}

