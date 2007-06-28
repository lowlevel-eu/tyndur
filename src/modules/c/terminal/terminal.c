
#define _USE_START_
#include "init.h"

#include "lostio.h"
#include "stdlib.h"

#include "gui/gui.h"

void callback(event_t event);

size_t stdout_write(lostio_filehandle_t *file, size_t blocksize,
                    size_t blockcount, void *data);
size_t stderr_write(lostio_filehandle_t *file, size_t blocksize,
                    size_t blockcount, void *data);
read_hdl_reply stdin_read(lostio_filehandle_t *file, size_t blocksize,
                          size_t blockcount);

typedef struct
{
    pid_t pid;
    enum {
        STDIN,
        STDOUT,
        STDERR 
    } type;
} console_ctrl_t;

char screen[25][81];

int posx = 0;
int posy = 0;

widget_t textbox = 0;

void scroll(int lines)
{
    posy-=lines;
}

int update = 0;

void write_text(char *text)
{
    while (*text)
    {
        if (*text == '\n') {
            posy++;
            posx = 0;
            if (posy == 25) {
                scroll(1);
            }
        } else {
            screen[posy][posx] = *text;
            posx++;
            if (posx == 80) {
                posy++;
                posx = 0;
                if (posy == 25) {
                    scroll(1);
                }
            }
        }
        text++;
    }
    if (textbox) {
        update++;
        //if (update  == 10) {
        //    update = 0;
            gui_set_label(textbox, screen);
        //}
    }
}

int main(int argc, char **argv)
{
    lostio_init();
    lostio_type_ramfile_use();
    lostio_type_directory_use();
    
    //stdout
    typehandle_t* typehandle = malloc(sizeof(typehandle_t));
    typehandle->id          = 255;
    typehandle->not_found   = NULL;
    typehandle->pre_open    = NULL;
    typehandle->post_open   = NULL; 
    typehandle->read        = NULL; 
    typehandle->write       = &stdout_write;
    typehandle->seek        = NULL;
    typehandle->close       = NULL;
    lostio_register_typehandle(typehandle);
    
    //stderr
    typehandle = malloc(sizeof(typehandle_t));
    typehandle->id          = 254;
    typehandle->not_found   = NULL;
    typehandle->pre_open    = NULL;
    typehandle->post_open   = NULL; 
    typehandle->read        = NULL; 
    typehandle->write       = &stderr_write;
    typehandle->seek        = NULL;
    typehandle->close       = NULL;
    lostio_register_typehandle(typehandle);

    //stdin
    typehandle = malloc(sizeof(typehandle_t));
    typehandle->id          = 253;
    typehandle->not_found   = NULL;
    typehandle->pre_open    = NULL;
    typehandle->post_open   = NULL; 
    typehandle->read        = &stdin_read; 
    typehandle->write       = NULL;
    typehandle->seek        = NULL;
    typehandle->close       = NULL;
    lostio_register_typehandle(typehandle);
    
    // vfstree bauen
    vfstree_create_node("/stdout" , 255, 0, NULL, 0);
    vfstree_create_node("/stderr" , 254, 0, NULL, 0);
    vfstree_create_node("/stdin" , 253, 0, NULL, 0);
    
    //Namen suchen
    char name[10];
    int i = 1;
    for(;;) {
        sprintf(name, "term%d", i);
        if (!init_service_get(name)) {
            break;
        }
        i++;
    }
    
    init_service_register(name);
    
    //Fenster aufbauen
    if (!init_gui()) {
        return 1;
    }
    gui_set_callback(callback);
    position_t position;
    position.x = 10;
    position.y = 10;
    dimension_t size;
    size.width = 300;
    size.height = 200;
    widget_t window = gui_create_widget_ex("window", 0, position, size);
    if (!window) {
        printf("Fenster konnte nicht erstellt werden!\n");
        return 1;
    }
    gui_set_label(window, "Terminal");
    memset(screen, ' ', 81 * 25);
    for (i = 0; i < 24; i++) {
        screen[i][80] = '\n';
    }
    screen[24][80] = 0;
    position.x = 5;
    position.y = 20;
    size.width = 290;
    size.height = 175;
    textbox = gui_create_widget_ex("textbox", window, position, size);
    gui_set_label(textbox, screen);
    
    //Prozess starten
    while (!init_service_get("console")) yield();
    
    int conpid = init_service_get("console");
    char data[100];
    sprintf(data + sizeof(console_ctrl_t), "%s:/stdout", name);
    console_ctrl_t *cmd = (console_ctrl_t*)data;
    cmd->type = STDOUT;
    pid_t pid = init_execute("floppy:/devices/fd0|fat:/apps/sh");
    cmd->pid = pid;
    msleep(10);
    rpc_get_int(conpid,"CONS_SET",
        sizeof(console_ctrl_t) + strlen(data + sizeof(console_ctrl_t)) + 1,
        (char*)data);
    sprintf(data + sizeof(console_ctrl_t), "%s:/stderr", name);
    msleep(10);
    cmd->type = STDERR;
    rpc_get_int(conpid, "CONS_SET",
        sizeof(console_ctrl_t) + strlen(data + sizeof(console_ctrl_t)) + 1,
        (char*)data);
    sprintf(data + sizeof(console_ctrl_t), "%s:/stdin", name);
    msleep(10);
    cmd->type = STDIN;
    /*rpc_get_int(conpid, "CONS_SET",
        sizeof(console_ctrl_t) + strlen(data + sizeof(console_ctrl_t)) + 1,
        (char*)data);*/
    
    while(TRUE) {
        wait_for_rpc();
    }
}

void callback(event_t event)
{
    switch (event.type)
    {
        case EVENT_BUTTON_CLICKED:
            break;
    }
}

size_t stdout_write(lostio_filehandle_t *file, size_t blocksize,
                    size_t blockcount, void *data)
{
    printf("[ TERMINAL ] ");
    char string[blocksize * blockcount + 1];
    memcpy(string, data, blocksize * blockcount);
    string[blocksize * blockcount] = 0;
    write_text(string);
    //puts(string);
    return blocksize * blockcount;
}
size_t stderr_write(lostio_filehandle_t *file, size_t blocksize,
                    size_t blockcount, void *data)
{
    printf("[ TERMINAL ] %s\n", (char*)data);
    return blocksize * blockcount;
}
read_hdl_reply stdin_read(lostio_filehandle_t *file, size_t blocksize,
                          size_t blockcount)
{
    read_hdl_reply ret;
    ret.data = 0;
    ret.size = 0;
    return ret;
}
