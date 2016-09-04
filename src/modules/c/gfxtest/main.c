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
    stdout = NULL;

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

    if (libvideo_change_display_resolution(0, 1024, 768, 24) != 0) {
        printf("Modus setzen fehlgeschlagen\n");
        return -1;
    }

    printf("Modus gesetzt.\n");

    video_bitmap_t *front;
    front = libvideo_get_frontbuffer_bitmap(0);
    printf("Got frontbuffer.\n");

    video_bitmap_t *drawing_test_bitmap;
    drawing_test_bitmap = libvideo_create_bitmap(1024, 768, 0, NULL);
    printf("Got drawing bitmap.\n");


/*
    void *databuffer = malloc(20 * 20 * 4);
    memset(databuffer, 0xdd, 20*20*4);
    video_bitmap_t *loaded_bitmap;
    loaded_bitmap = libvideo_create_bitmap(20, 20, 20*20*4, databuffer);*/

    libvideo_change_target(drawing_test_bitmap);

    // Sternförmig Linien malen.
    int x = 100, y = 100;
    for (a = 0; a < 360; a += 1) {
        x = cos(a * M_PI / 180) * 50;
        y = sin(a * M_PI / 180) * 50;
 /*       int component;
        component = (a%120) * 255 / 120;
        if (a < 120) {
            libvideo_change_color(0, component, 0, 0);
        }
        else if (a < 240) {
            libvideo_change_color(0, 0, component, 0);
        }
        else {
            libvideo_change_color(0, 0, 0, component);
        }*/
        libvideo_change_color(0, a * 255 / 360, (a-120) * 255 / 360, (a-240) * 255 / 360);
        libvideo_draw_line(100, 100, 100 + x, 100 + y);
    }

    libvideo_change_target(front);
//    libvideo_draw_rectangle(200, 200, 200, 200);
//    libvideo_draw_pixel(800, 600);
    libvideo_do_command_buffer();

    int i = 0;
    uint64_t usec_start = get_tick_count();

    for (i = 1; i <= 10; i++) {

//    libvideo_draw_bitmap_part(drawing_test_bitmap, 50, 50, 50, 50, 100, 100);
//    libvideo_draw_bitmap(loaded_bitmap, 90, 90);
    libvideo_draw_bitmap_part(drawing_test_bitmap, 0, 0, 0, 0, 1024, 768);
//    libvideo_draw_rectangle(0,0,1024,768);
    libvideo_do_command_buffer();
    //sleep(2);
    libvideo_draw_bitmap_part(drawing_test_bitmap, 0, 0, 0, 0, 1024, 768);
    libvideo_change_rop(VIDEO_ROP_AND);
    libvideo_change_color(0, 255, 0, 0);
    libvideo_draw_rectangle(0,0,1024,768);
    libvideo_change_rop(VIDEO_ROP_COPY);
    libvideo_do_command_buffer();
    //sleep(2);
    libvideo_draw_bitmap_part(drawing_test_bitmap, 0, 0, 0, 0, 1024, 768);
    libvideo_change_rop(VIDEO_ROP_AND);
    libvideo_change_color(0, 0, 255, 0);
    libvideo_draw_rectangle(0,0,1024,768);
    libvideo_change_rop(VIDEO_ROP_COPY);
    libvideo_do_command_buffer();
    //sleep(2);
    libvideo_draw_bitmap_part(drawing_test_bitmap, 0, 0, 0, 0, 1024, 768);
    libvideo_change_rop(VIDEO_ROP_AND);
    libvideo_change_color(0, 0, 0, 255);
    libvideo_draw_rectangle(0,0,1024,768);
    libvideo_change_rop(VIDEO_ROP_COPY);
    libvideo_do_command_buffer();
    //sleep(2);
//libvideo_draw_pixel(i, i);

    uint64_t used_end_int = get_tick_count();
    int mid = used_end_int - usec_start;
    mid /= i;
    printf("%d: %d us\n", i, mid);

    }
    libvideo_do_command_buffer();
    uint64_t usec_end = get_tick_count();

    int used = usec_end - usec_start;
    used /= 1000;
    printf("Finished Pass 1 in %d ms\n", front->id, used);
    libvideo_destroy_bitmap(front);
    libvideo_destroy_bitmap(drawing_test_bitmap);

    //sleep(5);

    libvideo_restore_text_mode();
    printf("Set Text Mode\n");

    return 0;

    usec_start = get_tick_count();
    libvideo_change_display_resolution(0, 800, 600, 24);
    usec_end = get_tick_count();
    used = usec_end - usec_start;
    printf("Resolution Change took %d usec", used);
    front = libvideo_get_frontbuffer_bitmap(0);

	libvideo_destroy_bitmap(drawing_test_bitmap);
	drawing_test_bitmap = libvideo_create_bitmap(800, 600, 0, NULL);
    printf("Got drawing bitmap.\n");


/*
    void *databuffer = malloc(20 * 20 * 4);
    memset(databuffer, 0xdd, 20*20*4);
    video_bitmap_t *loaded_bitmap;
    loaded_bitmap = libvideo_create_bitmap(20, 20, 20*20*4, databuffer);*/

    libvideo_change_target(drawing_test_bitmap);

    // Sternförmig Linien malen.
    x = 100; y = 100;
    for (a = 0; a < 360; a += 1) {
        x = cos(a * M_PI / 180) * 50;
        y = sin(a * M_PI / 180) * 50;
 /*       int component;
        component = (a%120) * 255 / 120;
        if (a < 120) {
            libvideo_change_color(0, component, 0, 0);
        }
        else if (a < 240) {
            libvideo_change_color(0, 0, component, 0);
        }
        else {
            libvideo_change_color(0, 0, 0, component);
        }*/
        libvideo_change_color(0, a * 255 / 360, (a-120) * 255 / 360, (a-240) * 255 / 360);
        libvideo_draw_line(100, 100, 100 + x, 100 + y);
    }

    libvideo_change_target(front);
//    libvideo_draw_rectangle(200, 200, 200, 200);
//    libvideo_draw_pixel(800, 600);
    libvideo_do_command_buffer();

    usec_start = get_tick_count();

    for (i = 1; i <= 10; i++) {

//    libvideo_draw_bitmap_part(drawing_test_bitmap, 50, 50, 50, 50, 100, 100);
//    libvideo_draw_bitmap(loaded_bitmap, 90, 90);
    libvideo_draw_bitmap_part(drawing_test_bitmap, 0, 0, 0, 0, 1024, 768);
//    libvideo_draw_rectangle(0,0,1024,768);
    libvideo_do_command_buffer();

    sleep(2);
    libvideo_change_rop(VIDEO_ROP_AND);
    libvideo_change_color(0, 255, 255, 0);
    libvideo_draw_rectangle(0,0,1024,768);
    libvideo_change_rop(VIDEO_ROP_COPY);
    libvideo_do_command_buffer();
    sleep(2);
    //libvideo_draw_pixel(i, i);

    uint64_t used_end_int = get_tick_count();
    int mid = used_end_int - usec_start;
    mid /= i;
    printf("%d: %d us\n", i, mid);

    }
    libvideo_do_command_buffer();
    usec_end = get_tick_count();

    used = usec_end - usec_start;
    used /= 1000;
    printf("Finished Pass 2 in %d ms\n", front->id, used);

    libvideo_destroy_bitmap(drawing_test_bitmap);
    libvideo_destroy_bitmap(front);

    return 0;
}
