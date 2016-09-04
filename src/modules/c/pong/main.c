/*
 * Copyright (c) 2009 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Alexander Siol.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.*/

#include <video/video.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <syscall.h>
#include <types.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    stdout = 0;
    driver_context_t* main_context;
    main_context = libvideo_create_driver_context("vesa");
    if (!main_context) {
        fprintf(stderr, "Konnte Grafiktreiberkontext nicht erstellen\n");
        return EXIT_FAILURE;
    }
    libvideo_use_driver_context(main_context);
    libvideo_get_command_buffer(1024);

    printf("Habe Kontext:\n pid: %d\n context: %d\n", main_context->driverpid,
        main_context->drivercontext);
    printf("Kommandobuffer: %d an 0x%x, Länge %d\n", main_context->cmdbufferid,
        main_context->cmdbuffer, main_context->cmdbufferlen);

    int num_devices = libvideo_get_num_devices();
    int a, b, c;
    for (a = 0; a < num_devices; a++) {
        printf("Device %d / %d\n", a + 1, num_devices);
        int num_displays = libvideo_get_num_displays(a);
        for (b = 0; b < num_displays; b++) {
            uint32_t modecnt = 0;
            video_mode_t* modes = NULL;
            modecnt = libvideo_get_modes(a, b, &modes);
            printf("\tDisplay %d / %d, %d Modi verfügbar\n", b + 1,
                num_displays, modecnt);
            for (c = 0; c < modecnt; c++) {
                printf("\t\tModus: %dx%dx%d\n", modes[c].width,
                    modes[c].height, modes[c].bpp);
            }
            /*
            uint32_t* modes = libvideo_get_modes(0, 0);
            printf("\tDisplay %d / %d, %d Modi verfügbar\n", b + 1,
                num_displays, modes[0]);
            for (c = 0; c < modes[0]; c++) {
                printf("\t\tModus: %dx%dx%d\n", modes[1 + c*4],
                    modes[1 + c*4 + 1], modes[1 + c*4 + 2]);
            }*/
        }
    }

    libvideo_change_device(0);
    printf("Gerät gesetzt.\n");

    libvideo_change_color(0, 255, 0, 0);
    libvideo_do_command_buffer();

    printf("Farbe gesetzt.\n");

    if (libvideo_change_display_resolution(0, 640, 480, 24) != 0) {
        printf("Modus setzen fehlgeschlagen\n");
        return -1;
    }

    printf("Modus gesetzt.\n");

    video_bitmap_t *front;
    front = libvideo_get_frontbuffer_bitmap(0);
    int buffers = libvideo_request_backbuffers(front, 2);

    printf("Got frontbuffer. %d Backbuffer.\n", buffers);

    int angle = 0;
    int speed = 100;

    int x = 1024/2;
    int y = 768/2;

    int dx, dy;

    int frames = 0;

    uint64_t start = get_tick_count();

    int oldsec = 0;

    libvideo_change_target(front);

    while (1) {
//        libvideo_change_target(back);
        libvideo_change_color(0,0,0,0);
        libvideo_draw_rectangle(0,0,1024,768);

        libvideo_change_color(0, 255, 0, 0);
        libvideo_draw_rectangle(0,0,1024,20);
        libvideo_draw_rectangle(0, 748, 1024, 20);
        libvideo_draw_rectangle(1004, 0, 20, 768);

        dx = sin(angle * M_PI / 180) * speed;
        dy = cos(angle * M_PI / 180) * speed;

        angle += 1;
        if (angle >= 360) angle=0;

        libvideo_change_color(0, 0xab, 0xcd, 0xef);
        libvideo_draw_rectangle(x-10+dx, y-10+dy, 20, 20);

        dx <<= 1;
        dy <<= 1;
        libvideo_change_color(0, 0, 255, 255);
        libvideo_draw_rectangle(x+dx, y+dy, 3, 3);

//        libvideo_change_target(front);
//        libvideo_draw_bitmap(back, 0,0 );
        libvideo_do_command_buffer();
        libvideo_flip_buffers(front);

        frames = frames + 1;
        uint64_t now = get_tick_count();
        uint64_t diff = now - start;
        // In Sek
        diff /= 1000000;
        if (diff > oldsec) {
            oldsec = diff;
            int fps = frames / diff;
            printf("Sek %d : %d Frames so far, Overall %d FPS\n", diff, fps, frames);
        }
    }
    return 0;

}
