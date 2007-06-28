
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

widget_t textbox[25];

//Scrollt um eine bestimmte Anzahl Zeilen nach oben
void scroll(int lines)
{
    widget_t tmplines[lines];
    int i;
    for (i = 0; i < lines; i++) {
        tmplines[i] = textbox[i];
    }
    position_t position;
    position.x = 5;
    position.y = 20;
    for (i = lines; i < 25; i++) {
        gui_set_position(textbox[i], position);
        textbox[i - lines] = textbox[i];
        position.y += 15;
        memcpy(screen[i - lines], screen[i], 80);
    }
    for (i = 0; i < lines; i++) {
        textbox[25 - lines + i] = tmplines[i];
        gui_set_position(tmplines[i], position);
        position.y += 15;
        gui_set_label(tmplines[i], "");
        memset(screen[25 - lines + i], ' ', 80);
    }
    posy-=lines;
}

int save_cursor_x = 0;
int save_cursor_y = 0;
//Führt eine Escape-Sequenz aus und liefert die Länge der Sequenz zurück
int parse_escape_sequence(char *str)
{
    if (*str != 0x1b) {
        return 0;
    }
    str++;
    if (*str != '[') {
        return 1;
    }
    
    int n1 = 0, n2 = 0;
    int have_n1 = 0, have_n2 = 0, have_delimiter = 0;
    int i;
    
    int length = 2;
    str++;
    while (*str) {
        switch (*str)
        {
            case '0' ... '9':
                if(!have_delimiter)
                {
                    n1 = n1 * 10 + (*str - '0');
                    have_n1 = 1;
                }
                else
                {
                    n2 = n2 * 10 + (*str - '0');
                    have_n2 = 1;
                }
                break;
            case ';':
                if(have_delimiter)
                {
                    length++;
                    return length;
                }
                have_delimiter = 1;
                break;
            case 'A': // ESC[#A - Cursor um # Zeilen nach oben bewegen
                if(have_delimiter || !have_n1 || have_n2)
                {
                    length++;
                    return length;
                }
                posy -= n1;
                if (posy < 0) posy = 0;
                length++;
                return length;
            case 'B': // ESC[#B - Cursor um # Zeilen nach unten bewegen
                if(have_delimiter || !have_n1 || have_n2)
                {
                    length++;
                    return length;
                }
                posy += n1;
                if (posy >= 25) posy = 24;
                length++;
                return length;
            case 'C': // ESC[#C - Cursor um # Zeilen nach rechts bewegen
                if(have_delimiter || !have_n1 || have_n2)
                {
                    length++;
                    return length;
                }
                posx += n1;
                if (posx >= 80) posx = 79;
                length++;
                return length;
            case 'D': // ESC[#D - Cursor um # Zeilen nach links bewegen
                if(have_delimiter || !have_n1 || have_n2)
                {
                    length++;
                    return length;
                }
                posx -= n1;
                if (posx < 0) posx = 0;
                length++;
                return length;
            case 'H': // ESC[#;#H - Cursor zur Zeile # und Spalte # bewegen
            case 'f':
                if(!have_n1 || !have_n2)
                {
                    length++;
                    return length;
                }
                posx = n1;
                posy = n2;
                if (posx < 0) posx = 0;
                if (posx >= 80) posx = 79;
                if (posy < 0) posy = 0;
                if (posy >= 25) posy = 24;
                length++;
                return length;
            case 'J': // ESC[2J - Bildschirm leeren
                if(have_delimiter || !have_n1 || have_n2 || n1 != 2)
                {
                    length++;
                    return length;
                }
                memset(screen, ' ', 81 * 25);
                for (i = 0; i < 25; i++) {
                    screen[i][80] = 0;
                }
                for (i = 0; i < 25; i++) {
                    gui_set_label(textbox[i], screen[i]);
                }
                posx = 0;
                posy = 0;
                length++;
                return length;
            case 'K': // ESC[K - Bis Zeile von der Cursor Position zum Ende leeren
                if(have_delimiter || have_n1 || have_n2)
                {
                    length++;
                    return length;
                }
                memset(&screen[posx][posy], ' ', 80 - posx);
                length++;
                return length;
            case 'm': // ESC[#m oder ESC[#;#m - Vorder- und Hintergrundfarbe ändern, Fettschrift, Blinken
                //TODO
                length++;
                return length;
            case 's': // ESC[s - Cursor-Position speichern
                if(have_delimiter || have_n1 || have_n2)
                {
                    length++;
                    return length;
                }
                save_cursor_x = posx;
                save_cursor_y = posy;
                length++;
                return length;
            case 'u': // ESC[u - Cursor-Position wiederherstellen
                if(have_delimiter || have_n1 || have_n2)
                {
                    length++;
                    return length;
                }
                posx = save_cursor_x;
                posy = save_cursor_y;
                length++;
                return length;
            default:
                return length;
        }
        length++;
        str++;
    }
    return length;
}

//Schreibt Text in das Terminal
void write_text(char *text)
{
    while (*text)
    {
        if (*text == '\n') {
            gui_set_label(textbox[posy], screen[posy]);
            posy++;
            posx = 0;
            if (posy == 25) {
                scroll(1);
            }
        } else if (*text == 0x1b) {
            text += parse_escape_sequence(text);
            continue;
        } else {
            screen[posy][posx] = *text;
            posx++;
            if (posx == 80) {
                gui_set_label(textbox[posy], screen[posy]);
                posy++;
                posx = 0;
                if (posy == 25) {
                    scroll(1);
                }
            }
        }
        text++;
    }
    gui_set_label(textbox[posy], screen[posy]);
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
    size.width = 640;
    size.height = 400;
    widget_t window = gui_create_widget_ex("window", 0, position, size);
    if (!window) {
        printf("Fenster konnte nicht erstellt werden!\n");
        return 1;
    }
    gui_set_label(window, "Terminal");
    memset(screen, ' ', 81 * 25);
    for (i = 0; i < 25; i++) {
        screen[i][80] = 0;
    }
    position.x = 3;
    position.y = 20;
    size.width = 290;
    size.height = 15;
    for (i = 0; i < 25; i++) {
        textbox[i] = gui_create_widget_ex("textbox", window, position, size);
        position.y += 15;
        gui_set_label(textbox[i], screen[i]);
    }
    
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
