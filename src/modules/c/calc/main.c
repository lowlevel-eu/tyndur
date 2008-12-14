/*  
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Mathias Gottschlag.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *     This product includes software developed by the tyndur Project
 *     and its contributors.
 * 4. Neither the name of the tyndur Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "types.h"
#include "stdio.h"
#include "lostio.h"
#include "gui/gui.h"
#include "gui/widgets.h"

#include "video/bitmap.h"

#define _USE_START_
#include "init.h"

void guicb(event_t event);

window_t *window = 0;

widget_t *addition = 0;
widget_t *subtraction = 0;
widget_t *multiplication = 0;
widget_t *division = 0;
widget_t *equal = 0;

widget_t *numbers[10];

widget_t *clear = 0;

int number1 = 0;
int number2 = 0;

char display[20];
int displayposition = 0;

int status = 0;

int operation = 0;

int running = 1;

void type_digit(int number)
{
    if (displayposition >= 19) {
        return;
    }
    if (status == 2) {
        //Vorheriges Ergebnis in number1 löschen
        clear_screen();
        number1 = 0;
        number2 = 0;
        status = 0;
        operation = 0;
    }
    if (status == 0) {
        number1 = number1 * 10 + number;
        display[displayposition] = '0' + number;
        displayposition++;
    }
    if (status == 1) {
        number2 = number2 * 10 + number;
        display[displayposition] = '0' + number;
        displayposition++;
    }

    gui_repaint_window(window);
}

void clear_screen(void)
{
    memset(display, 0, 20);
    display[0] = '0';
    displayposition = 0;
}

void compute_result(void)
{
    if (operation == 1) {
        number1 = number1 + number2;
    } else if (operation == 2) {
        number1 = number1 - number2;
    } else if (operation == 3) {
        number1 = number1 * number2;
    } else if (operation == 4) {
        if (number2 != 0) {
            number1 = number1 / number2;
        } else {
            number1 = 0;
        }
    }
    status = 2;
    number2 = 0;
    snprintf(display, 20, "%d", (int)number1);
    displayposition = 0;
}

int main(int argc, char* argv[])
{
    init_gui();
    gui_set_callback(guicb);
    
    //Fenster aufbauen
    window = gui_create_window(30, 30, 160, 160, GUI_WINDOW_FRAME | GUI_WINDOW_CLOSE);
    gui_set_window_title(window, "Taschenrechner");
    
    numbers[1] = create_button(window, 10, 40, 20, 20, "1");
    numbers[2] = create_button(window, 40, 40, 20, 20, "2");
    numbers[3] = create_button(window, 70, 40, 20, 20, "3");
    numbers[4] = create_button(window, 10, 70, 20, 20, "4");
    numbers[5] = create_button(window, 40, 70, 20, 20, "5");
    numbers[6] = create_button(window, 70, 70, 20, 20, "6");
    numbers[7] = create_button(window, 10, 100, 20, 20, "7");
    numbers[8] = create_button(window, 40, 100, 20, 20, "8");
    numbers[9] = create_button(window, 70, 100, 20, 20, "9");
    numbers[0] = create_button(window, 40, 130, 20, 20, "0");
    
    division = create_button(window, 100, 40, 20, 20, "/");
    multiplication = create_button(window, 130, 40, 20, 20, "x");
    subtraction = create_button(window, 100, 70, 20, 20, "-");
    addition = create_button(window, 130, 70, 20, 20, "+");
    clear = create_button(window, 100, 100, 50, 20, "C");
    equal = create_button(window, 100, 130, 50, 20, "=");
    
    //Rechner initialisieren
    memset(display, 0, 20);
    display[0] = '0';
    
    gui_repaint_window(window);
    
    while(running) {
        yield();
    }
    close_gui();
}

void guicb(event_t event)
{
    switch (event.type)
    {
        case EVENT_WINDOW_PAINT:
            if (event.windowid == window->id) {
                bitmap_draw_rect(window->bitmap, 0xFFFFFF, 0, 0, window->width, window->height);
                dword length = get_text_width(titlefont, display);
                render_text(window->bitmap, titlefont, display, 150 - length, 30);
            }
            break;
        case EVENT_MOUSE_BUTTON_PRESSED:
            break;
        case EVENT_BUTTON_PRESSED:
            if (event.data[0] == numbers[1]->id) {
                type_digit(1);
            } else if (event.data[0] == numbers[2]->id) {
                type_digit(2);
            } else if (event.data[0] == numbers[3]->id) {
                type_digit(3);
            } else if (event.data[0] == numbers[4]->id) {
                type_digit(4);
            } else if (event.data[0] == numbers[5]->id) {
                type_digit(5);
            } else if (event.data[0] == numbers[6]->id) {
                type_digit(6);
            } else if (event.data[0] == numbers[7]->id) {
                type_digit(7);
            } else if (event.data[0] == numbers[8]->id) {
                type_digit(8);
            } else if (event.data[0] == numbers[9]->id) {
                type_digit(9);
            } else if (event.data[0] == numbers[0]->id) {
                type_digit(0);
            } else if (event.data[0] == clear->id) {
                number1 = 0;
                number2 = 0;
                clear_screen();
                status = 0;
            } else if (event.data[0] == equal->id) {
                if (status == 1) {
                    compute_result();
                }
            } else if (event.data[0] == addition->id) {
                if (status == 1) {
                    compute_result();
                }
                if ((status == 0) || (status == 2)) {
                    operation = 1;
                    status = 1;
                    clear_screen();
                }
            } else if (event.data[0] == subtraction->id) {
                if (status == 1) {
                    compute_result();
                }
                if ((status == 0) || (status == 2)) {
                    operation = 2;
                    status = 1;
                    clear_screen();
                }
            } else if (event.data[0] == multiplication->id) {
                if (status == 1) {
                    compute_result();
                }
                if ((status == 0) || (status == 2)) {
                    operation = 3;
                    status = 1;
                    clear_screen();
                }
            } else if (event.data[0] == division->id) {
                if (status == 1) {
                    //Ausrechnen
                }
                if ((status == 0) || (status == 2)) {
                    operation = 4;
                    status = 1;

                    clear_screen();
                }
            }
            break;
        case EVENT_CLOSE_WINDOW:
            running = 0;
            break;
    }
}
