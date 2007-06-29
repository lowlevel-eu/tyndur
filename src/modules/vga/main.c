
#include "types.h"
#include "stdlib.h"
#include "stdio.h"
#include "lostio.h"
#include "ports.h"
#include "syscall.h"

#include "vga.h"
#include "video/commands.h"
#include "drawing.h"

#define _USE_START_
#include "init.h"

read_hdl_reply information_read(lostio_filehandle_t *file, size_t blocksize,
    size_t blockcount);

//Interface über RPC
void rpc_driver_interface(pid_t pid,
                          dword correlation_id,
                          size_t data_size,
                          void* data);

int owner = -1;

char *information = "VGA 0.0.1\n"; //TODO: Mehr (dynamisches) Info

int main(int argc, char **argv)
{
    lostio_init();
    lostio_type_ramfile_use();
    lostio_type_directory_use();
    
    //registriere Ports
    if (!request_ports(0x3BF, 0x20)) {
        puts("[ VGA ] Konnte Ports nicht belegen!");
        return -1;
    }
    
    //Informationen über den Treiber
    typehandle_t* typehandle_information = malloc(sizeof(typehandle_t));
    typehandle_information->id          = 255;
    typehandle_information->not_found   = NULL;
    typehandle_information->pre_open    = NULL;
    typehandle_information->post_open   = NULL; 
    typehandle_information->read        = &information_read; 
    typehandle_information->write       = NULL;
    typehandle_information->seek        = NULL;
    typehandle_information->close       = NULL;
    lostio_register_typehandle(typehandle_information);
    
    vfstree_create_node("/information" , 255, 0, NULL, 0);
    
    register_message_handler("VIDEODRV", rpc_driver_interface);
    
    init_service_register("vga");
    
    while(TRUE) {
        wait_for_rpc();
    }
}

read_hdl_reply information_read(lostio_filehandle_t *file, size_t blocksize,
    size_t blockcount)
{
    read_hdl_reply ret;

    size_t totalsize = blocksize * blockcount;
    if (totalsize < strlen(information) + 1) {
        ret.data = 0;
        ret.size = 0;

        return ret;
    }
    ret.data = information;
    ret.size = strlen(information) + 1;
    return ret;
}


int loading_bitmap = 0;
int tmp_bitmap_width = 0;
int tmp_bitmap_height = 0;
int tmp_data_size = 0;
int *tmp_data;

void rpc_driver_interface(pid_t pid,
                          dword correlation_id,
                          size_t data_size,
                          void* data)
{
    //printf("VGA-RPC!\n");
    p();
    int newbitmap;
    int result;
    //Befehlsinformation auslesen und Befehl ausführen
    int *command = data;
    if (data_size < sizeof(int)) {
        rpc_send_dword_response(pid, correlation_id, 0);
        return;
    }
    switch (command[0]) {
        case VIDEO_REGISTER:
            if (owner == -1) {
                owner = pid;
                rpc_send_dword_response(pid, correlation_id, 1);
            } else {
                rpc_send_dword_response(pid, correlation_id, 0);
            }
            break;
        case VIDEO_UNREGISTER:
            if (owner == pid) {
                owner = -1;
                rpc_send_dword_response(pid, correlation_id, 1);
            } else {
                rpc_send_dword_response(pid, correlation_id, 0);
            }
            break;
        case VIDEO_INIT:
            if (owner != pid) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            result = vga_init();
            rpc_send_dword_response(pid, correlation_id, result);
            break;
        case VIDEO_CLOSE:
            if (owner != pid) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            result = vga_close();
            rpc_send_dword_response(pid, correlation_id, result);
            break;
        case VIDEO_UPDATE_SCREEN:
            if (owner != pid) {
                rpc_send_dword_response(pid, correlation_id, 0);
            }
            result = update_screen();
            rpc_send_dword_response(pid, correlation_id, result);
            break;
        case VIDEO_UPDATE_PARTLY:
            if (owner != pid) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            if (data_size < 5 * sizeof(int)) {
                rpc_send_dword_response(pid, correlation_id, 0);
                return;
            }
            result = update_partly(command[1],//x
                                   command[2],//y
                                   command[3],//Breite
                                   command[4]);//Höhe
            rpc_send_dword_response(pid, correlation_id, result);
            break;
        case VIDEO_SET_RESOLUTION:
            if (owner != pid) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            if (data_size < 4 * sizeof(int)) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            result = vga_set(command[1],//Breite
                             command[2],//Höhe
                             command[3]);//bpp
            rpc_send_dword_response(pid, correlation_id, result);
            break;
        case VIDEO_CLEAR_SCREEN:
            if (owner != pid) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            if (data_size < 2 * sizeof(int)) {
                clear_screen(0xFF000000);
            } else {
                clear_screen(command[1]); //Farbe
            }
            rpc_send_dword_response(pid, correlation_id, 1);
            break;
        case VIDEO_DRAW_DOT:
            if (owner != pid) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            if (data_size < 4 * sizeof(int)) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            draw_dot(command[1],//x
                     command[2],//y
                     command[3]);//Farbe
            rpc_send_dword_response(pid, correlation_id, 1);
            break;
        case VIDEO_DRAW_BITMAP:
            if (owner != pid) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            if (data_size < 5 * sizeof(int)) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            draw_bitmap(command[1],//Bitmap
                        command[2],//x
                        command[3],//y
                        command[4]);//Flags
            rpc_send_dword_response(pid, correlation_id, 1);
            break;
        case VIDEO_CREATE_BITMAP:
            if (owner != pid) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            if (data_size < 3 * sizeof(int)) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            tmp_data = malloc(command[1] * command[2] * sizeof(int));
            if (!tmp_data) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            loading_bitmap = 1;
            tmp_bitmap_width = command[1];
            tmp_bitmap_height = command[2];
            tmp_data_size = 0;
            rpc_send_dword_response(pid, correlation_id, 1);
            break;
        case VIDEO_MORE_DATA:
            if (owner != pid) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            if (!loading_bitmap) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            memcpy(&tmp_data[tmp_data_size], &command[1], data_size - 4);
            tmp_data_size += (data_size - 4) / 4;
            //TODO: Überlauf?
            rpc_send_dword_response(pid, correlation_id, 0);
            break;
        case VIDEO_DATA_SENT:
            if (owner != pid) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            newbitmap = create_bitmap(tmp_bitmap_width, tmp_bitmap_height, tmp_data);
            tmp_data = 0;//Daten werden weiter benutzt!
            tmp_bitmap_width = 0;
            tmp_bitmap_height = 0;
            tmp_data_size = 0;
            loading_bitmap = 0;
            rpc_send_dword_response(pid, correlation_id, newbitmap);
            break;
        case VIDEO_DELETE_BITMAP:
            if (owner != pid) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            if (data_size < 2 * sizeof(int)) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            delete_bitmap(command[1]);//Bitmap
            rpc_send_dword_response(pid, correlation_id, 1);
            break;
        case VIDEO_DRAW_RECT:
            if (owner != pid) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            if (data_size < 6 * sizeof(int)) {
                rpc_send_dword_response(pid, correlation_id, 0);
                break;
            }
            draw_rect(command[1],//x
                      command[2],//y
                      command[3],//Breite
                      command[4],//Höhe
                      command[5]);//Farbe
            rpc_send_dword_response(pid, correlation_id, 1);
            break;
        case VIDEO_COMMAND_CHAIN:
            data_size-= sizeof(int);
            command++;
            while (data_size) {
                switch (*command)
                {
                    case VIDEO_CLEAR_SCREEN:
                        if (data_size < 2 * sizeof(int)) {
                            data_size = 0;
                            break;
                        }
                        clear_screen(command[1]);
                        data_size-= 2 * sizeof(int);
                        command+= 2;
                        break;
                    case VIDEO_DRAW_DOT:
                        if (data_size < 4 * sizeof(int)) {
                            data_size = 0;
                            break;
                        }
                        draw_dot(command[1], command[2], command[3]);
                        data_size-= 4 * sizeof(int);
                        command+= 4;
                        break;
                    case VIDEO_DRAW_RECT:
                        if (data_size < 6 * sizeof(int)) {
                            data_size = 0;
                            break;
                        }
                        draw_rect(command[1], command[2], command[3],
                            command[4], command[5]);
                        data_size-= 6 * sizeof(int);
                        command+= 6;
                        break;
                    case VIDEO_DRAW_LINE:
                        if (data_size < 6 * sizeof(int)) {
                            data_size = 0;
                            break;
                        }
                        //TODO
                        data_size-= 6 * sizeof(int);
                        command+= 6;
                        break;
                    case VIDEO_DRAW_BITMAP:
                        if (data_size < 5 * sizeof(int)) {
                            data_size = 0;
                            break;
                        }
                        draw_bitmap(command[1], command[2], command[3],
                            command[4]);
                        data_size-= 5 * sizeof(int);
                        command+= 5;
                        break;
                    case VIDEO_UPDATE_SCREEN:
                        update_screen();
                        data_size-= sizeof(int);
                        command++;
                        break;
                    case VIDEO_UPDATE_PARTLY:
                        if (data_size < 5 * sizeof(int)) {
                            data_size = 0;
                            break;
                        }
                        update_partly(command[1], command[2], command[3],
                            command[4]);
                        data_size-= 5 * sizeof(int);
                        command+= 5;
                        break;
                    default:
                        printf("[ VGA ] Unbekannter Befehl (%d)!\n", *command);
                        data_size = 0;
                        break;
                }
            }
            rpc_send_dword_response(pid, correlation_id, 1);
            break;
        default:
            rpc_send_dword_response(pid, correlation_id, 0);
            break;
    }
    v();
    //printf("VGA-RPC Ende!\n");
}
