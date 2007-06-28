
#include "video/video.h"

#include "types.h"
#include "stdio.h"
#include "stdlib.h"
#include "rpc.h"
#include "init.h"

FILE *video_driver_control = 0;
int video_command_data[10];

int video_init(char *driver_name)
{
    if (!driver_name) {
        return 0;
    }
    //Suche Treiber
    qword timeout = get_tick_count() + 5000000;
    while (init_service_get(driver_name) == 0) {
        yield();
        if(get_tick_count() > timeout) {
            puts("[ LIBVIDEO ] Konnte Videotreiber nicht finden!");
            return 0;
        }
    }
    //Öffne Videotreiber
    char tmp[30];
    sprintf(tmp, "%s:/information", driver_name);
    video_driver_control = fopen(tmp, "r");
    if (!video_driver_control) {
        printf("[ LIBVIDEO ] Konnte Videotreiber nicht oeffnen.\n");
        return 0;
    } else {
        video_command_data[0] = 15;
        if (!rpc_get_dword(video_driver_control->pid,
                           "VIDEODRV",
                           sizeof(int),
                           (char*)video_command_data)) {
            fclose(video_driver_control);
            video_driver_control = 0;
            printf("[ LIBVIDEO ] Konnte Prozess nicht registrieren.\n");
            return 0;
        }
        video_command_data[0] = 12;
        if (!rpc_get_dword(video_driver_control->pid,
             "VIDEODRV",
             sizeof(int) * 4,
             (char*)video_command_data)) {
            video_command_data[0] = 16;
            rpc_get_dword(video_driver_control->pid,
                          "VIDEODRV",
                          sizeof(int),
                          (char*)video_command_data);
            fclose(video_driver_control);
            video_driver_control = 0;
            printf("[ LIBVIDEO ] Konnte Videotreiber nicht initialisieren.\n");
            return 0;
        }

    }
    return 1;
}
void video_close(void)
{
    if (!video_driver_control) {
        return;
    }
    //TODO: Registrierung auflösen
    //Schließe Verbindung zum Treiber
    fclose(video_driver_control);
    video_driver_control = 0;
}

int video_draw_dot(int x, int y, int color)
{
    if (!video_driver_control) {
        return 0;
    }
    video_command_data[0] = 2;
    video_command_data[1] = x;
    video_command_data[2] = y;
    video_command_data[3] = color;
    return rpc_get_dword(video_driver_control->pid,
                         "VIDEODRV",
                         sizeof(int) * 4,
                         (char*)video_command_data);
}
int video_draw_rect(int x, int y, int width, int height, int color)
{
    if (!video_driver_control) {
        return 0;
    }
    video_command_data[0] = 3;
    video_command_data[1] = x;
    video_command_data[2] = y;
    video_command_data[3] = width;
    video_command_data[4] = height;
    video_command_data[5] = color;
    return rpc_get_dword(video_driver_control->pid,
                         "VIDEODRV",
                         sizeof(int) * 6,
                         (char*)video_command_data);
}
int video_clear_screen(int color)
{
    if (!video_driver_control) {
        return 0;
    }
    video_command_data[0] = 8;
    video_command_data[1] = color;
    return rpc_get_dword(video_driver_control->pid,
                         "VIDEODRV",
                         sizeof(int) * 2,
                         (char*)video_command_data);
}

int video_draw_bitmap(int bitmap, int x, int y, int transparent)
{
    if (!video_driver_control) {
        return 0;
    }
    if (!bitmap) {
        return 0;
    }
    video_command_data[0] = 5;
    video_command_data[1] = bitmap;
    video_command_data[2] = x;
    video_command_data[3] = y;
    video_command_data[4] = transparent;
    return rpc_get_dword(video_driver_control->pid,
                         "VIDEODRV",
                         sizeof(int) * 5,
                         (char*)video_command_data);}

int video_load_bitmap(int width, int height, void *data)
{
    int tmpdata[1025];
    int datasent = 0;
    if (!video_driver_control) {
        return 0;
    }
    if (!data) {
        return 0;
    }
    if (!width || !height) {
        return 0;
    }
    
    //Kopiere Befehl und Bitmap in temporären Speicher
    video_command_data[0] = 6;
    video_command_data[1] = width;
    video_command_data[2] = height;
    dword result = rpc_get_dword(video_driver_control->pid,
                                 "VIDEODRV",
                                 sizeof(int) * 3,
                                 (char*)video_command_data);
    if (!result) {
        return 0;
    }
    while (datasent < (width * height)) {
        tmpdata[0] = 10;
        int size = 1024;
        if ((width * height - datasent) < 1024) {
            size = width * height - datasent;
        }
        memcpy(&tmpdata[1], &((int*)data)[datasent], size * sizeof(int));
        result = rpc_get_dword(video_driver_control->pid,
                               "VIDEODRV",
                               sizeof(int) * (size + 1),
                               (char*)tmpdata);
        datasent += size;
    }
    video_command_data[0] = 11;
    result = rpc_get_dword(video_driver_control->pid,
                           "VIDEODRV",
                           sizeof(int),
                           (char*)video_command_data);
    return result;
}
int video_delete_bitmap(int bitmap)
{
    if (!video_driver_control) {
        return 0;
    }
    video_command_data[0] = 7;
    video_command_data[1] = bitmap;
    return rpc_get_dword(video_driver_control->pid,
                         "VIDEODRV",
                         sizeof(int) * 2,
                         (char*)video_command_data);
}

int video_update_screen(void)
{
    if (!video_driver_control) {
        return 0;
    }
    video_command_data[0] = 1;
    return rpc_get_dword(video_driver_control->pid,
                         "VIDEODRV",
                         sizeof(int),
                         (char*)video_command_data);
}
int video_update_partly(int x, int y, int width, int height)
{
    video_command_data[0] = 17;
    video_command_data[1] = x;
    video_command_data[2] = y;
    video_command_data[3] = width;
    video_command_data[4] = height;
    return rpc_get_dword(video_driver_control->pid,
                         "VIDEODRV",
                         sizeof(int) * 5,
                         (char*)video_command_data);
}
