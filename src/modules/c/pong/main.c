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
#include <init.h>
#include <kbd.h>
#include <lostio.h>

//------------------------------------------------------------------------------
// Types und Structs
//------------------------------------------------------------------------------

typedef struct Vector2 {
    int x;
    int y;
} Vector2;

typedef struct Player {
    Vector2 position;
    Vector2 size;
    int lives;
} Player;

typedef struct Ball {
    Vector2 position;
    Vector2 speed;
    Vector2 size;
    bool active;
} Ball;

typedef struct Key {
    bool down;
    bool pressed;
} Key;

typedef struct Keyboard {
    pid_t kbc_pid;
    Key arrow_up;
    Key arrow_down;
    Key enter;
    Key escape;
} Keyboard;

//------------------------------------------------------------------------------
// Gloable Variablen
//------------------------------------------------------------------------------

static const int screen_width = 640;
static const int screen_height = 480;
static const int time_per_frame = 20 * 1000; // 20ms
static const int player_max_live = 5;

static bool game_over = false;

static Player player = {0};
static Ball ball = {0};
static Keyboard keyboard = {0};

static video_bitmap_t* front;

//------------------------------------------------------------------------------
// Funktionsdefinitionen
//------------------------------------------------------------------------------

static void update_key(Key* key, bool down)
{
    if (!down && key->down) {
        key->pressed = true;
    }

    key->down = down;
}

static bool is_key_pressed(Key* key)
{
    if (key->pressed) {
        key->pressed = false;
        return true;
    }

    return false;
}

static void update_player(void)
{
    if (is_key_pressed(&keyboard.escape)) {
        game_over = true;
    }

    if (!game_over) {
        if (player.lives <= 0) {
            game_over = true;
        }
        if (keyboard.arrow_up.down) {
            player.position.y -= 5;
        }
        if (keyboard.arrow_down.down) {
            player.position.y += 5;
        }
        if ((player.position.y + player.size.y) >= screen_height) {
            player.position.y = screen_height - player.size.y;
        }
        if (player.position.y <= 0) {
            player.position.y = 0;
        }
    }
}

static void update_ball(void)
{
    if (!game_over)
    {
        if (is_key_pressed(&keyboard.enter) && !ball.active) {
            ball.speed = (Vector2){5,0};
            ball.active = true;
        }
        if (ball.active) {
            ball.position.x += ball.speed.x;
            ball.position.y += ball.speed.y;
        }
        if ((ball.position.y + ball.size.y) >= screen_height ||
            ball.position.y <= 0)
        {
            ball.speed.y = -ball.speed.y;
        }

        if ((ball.position.x + ball.size.x) >= screen_width) {
            ball.speed.x = -ball.speed.x;
        }

        // Tooooor!
        if (ball.position.x <= 0) {
            ball.position = (Vector2){screen_width/2, screen_height/2};
            ball.speed = (Vector2){0, 0};
            ball.active = false;
            player.lives--;
        }

        // Kollision mit Spieler
        // Der Ball kann auch von schräg oben bzw. schräg unten kommen.
        // Daher wird der gesamte Kollisonsbereich überprüft.
        if (player.position.x - ball.size.x <= ball.position.x &&
            player.position.x + player.size.x >= ball.position.x &&
            player.position.y - ball.size.y <= ball.position.y &&
            player.position.y + player.size.y >= ball.position.y)
        {
            if (ball.speed.x < 0) {
                ball.speed.x = -ball.speed.x;
            }

            // Erhöhen der Geschwindigkeit
            // und damit auch des Schwierigkeitsgrads
            if (ball.speed.y > 0) {
                ball.speed.y += 1;
            } else {
              ball.speed.y -= 1;
            }

            ball.speed.x += 1;
        }
    }
}

static void render(void)
{
    // Male Hintergrund
    libvideo_change_color(0, 0, 0, 0);
    libvideo_draw_rectangle(0, 0, screen_width, screen_height);

    // Male Spieler
    libvideo_change_color(0, 255, 0, 0);
    libvideo_draw_rectangle(player.position.x, player.position.y,
                            player.size.x, player.size.y);

    // Male Ball
    libvideo_change_color(0, 0xab, 0xcd, 0xef);
    libvideo_draw_rectangle(ball.position.x, ball.position.y,
                            ball.size.x, ball.size.y);

    // Male Leben
    for (int i = 1; i < player.lives; i++) {
        libvideo_draw_rectangle(screen_width - 40 * i, 16, 20, 20);
    }

    libvideo_do_command_buffer();
    libvideo_flip_buffers(front);
}

static bool init_game(void)
{

    // Initialisiere Spieler
    player.position = (Vector2){screen_width*1/10, screen_height/2};
    player.size = (Vector2){20, screen_height/5};
    player.lives = player_max_live;

    // Initialisiere Ball
    ball.position = (Vector2){screen_width/2, screen_height/2};
    ball.speed = (Vector2){0, 0};
    ball.size = (Vector2){20, 20};
    ball.active = false;

    return true;
}

static void rpc_kbd_callback(pid_t pid, uint32_t cid, size_t data_size,
    void* data)
{
    uint8_t* kbd_data = data;
    char buf[16];

    if ((pid != keyboard.kbc_pid) || (data_size != 2)) {
        return;
    }

    p();

    if (kbd_data[0] == KEYCODE_ARROW_UP) {
        update_key(&keyboard.arrow_up, !kbd_data[1]);
    } else if (kbd_data[0] == KEYCODE_ARROW_DOWN) {
        update_key(&keyboard.arrow_down, !kbd_data[1]);
    } else if (kbd_data[0] == KEYCODE_ENTER) {
        update_key(&keyboard.enter, !kbd_data[1]);
    } else if (kbd_data[0] == KEYCODE_ESC) {
        update_key(&keyboard.escape, !kbd_data[1]);
    }

    v();

    /*
     *  Und stdin auslesen, sonst landen alle Tastendrücke nach dem Beenden in
     *  der Shell. Benutzt werden sie nicht.
     */
    if (stdin->res) {
        lio_readf(stdin->res->lio2_stream, 0, sizeof(buf), buf, LIO_REQ_FILEPOS);
    }

}

static bool init_keyboard(void)
{
    // RPC-Handler fuer die Callbacks von KBD einrichten
    register_message_handler(KBD_RPC_CALLBACK, rpc_kbd_callback);

    keyboard.kbc_pid = init_service_get("kbc");
    if (!keyboard.kbc_pid) {
        return false;
    }

    // Callback registrtrieren bei KBC
    return rpc_get_dword(keyboard.kbc_pid, KBD_RPC_REGISTER_CALLBACK, 0, NULL);
}

static bool init_screen(void)
{
    stdout = 0;
    driver_context_t* main_context;
    main_context = libvideo_create_driver_context("vesa");
    if (!main_context) {
        fprintf(stderr, "Konnte Grafiktreiberkontext nicht erstellen\n");
        return false;
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

    if (libvideo_change_display_resolution(0, screen_width,
                                           screen_height, 24) != 0)
    {
        printf("Modus setzen fehlgeschlagen\n");
        return false;
    }

    printf("Modus gesetzt.\n");

    front = libvideo_get_frontbuffer_bitmap(0);
    int buffers = libvideo_request_backbuffers(front, 2);

    printf("Got frontbuffer. %d Backbuffer.\n", buffers);

    libvideo_change_target(front);
    return true;
}

static void close_screen(void)
{
    libvideo_close_upload_buffer(front);
    libvideo_destroy_bitmap(front);
    libvideo_restore_text_mode();
}

static void run(void)
{
    uint64_t time_last_update = get_tick_count();
    uint64_t delta_time = 0;

    while (!game_over) {

        uint64_t current_time = get_tick_count();
        delta_time += current_time - time_last_update;
        time_last_update = current_time;

        while (delta_time >= time_per_frame) {
            delta_time -= time_per_frame;
            update_player();
            update_ball();
        }
        render();
    }
    close_screen();
}

int main(int argc, char** argv)
{
    init_screen();
    init_keyboard();
    init_game();

    run();

    return 0;
}
