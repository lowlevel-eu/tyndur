
#define _USE_START_
#include "init.h"

#include "lostio.h"

#include "gui/gui.h"

void callback(event_t event);

widget_t button = 0;

int main(int argc, char **argv)
{
    lostio_init();
    init_gui();
    position_t position;
    position.x = 10;
    position.y = 10;
    dimension_t size;
    size.width = 300;
    size.height = 200;
    widget_t window = gui_create_widget_ex("window", 0, position, size);
    gui_set_label(window, "Fenster 1");
    position.y = 30;
    size.width = 100;
    size.height = 30;
    button = gui_create_widget_ex("button", window, position, size);
    gui_set_label(button, "Button 1");
    
    gui_set_callback(callback);
    
    while(TRUE) {
        wait_for_rpc();
    }
}

void callback(event_t event)
{
    printf("[ GUITEST ] Event eingetroffen!\n");
    switch (event.type)
    {
        case EVENT_BUTTON_CLICKED:
            if (event.sender == button) {
            }
            break;
    }
}
