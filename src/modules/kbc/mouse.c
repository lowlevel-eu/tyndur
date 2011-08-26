
#include "types.h"
#include "rpc.h"
#include "stdlib.h"
#include "stdio.h" 
#include "lostio.h"
#include "ports.h"
#include "string.h"
#include <syscall.h>

#include "mouse.h"


uint8_t mouse_cycle=0;
int8_t mouse_byte[3];
int8_t mouse_x=0;
int8_t mouse_y=0;

//Mausdaten
//Bis auf buttons werden alle bei jedem Lesen der Daten zurückgesetzt
uint8_t buttons = 0;
uint8_t buttons_pressed = 0;
uint8_t buttons_released = 0;
int rel_x = 0;
int rel_y = 0;
uint8_t cb_buffer[12];

FILE **cbfiles = 0;
char **cbfilenames = 0;
int cb_count = 0;

void mouse_init(void)
{
    //Abfrage per Polling
    typehandle_t* typehandle = malloc(sizeof(typehandle_t));
    typehandle->id          = 254;
    typehandle->not_found   = NULL;
    typehandle->pre_open    = NULL;
    typehandle->post_open   = NULL;
    typehandle->read        = &mouse_read_handler;
    typehandle->write       = NULL;
    typehandle->seek        = NULL;
    typehandle->close       = NULL;
    typehandle->link        = NULL;
    typehandle->unlink      = NULL;
    lostio_register_typehandle(typehandle);

    //Registrierung von Callback-Dateien
    typehandle = malloc(sizeof(typehandle_t));
    typehandle->id          = 253;
    typehandle->not_found   = NULL;
    typehandle->pre_open    = NULL;
    typehandle->post_open   = NULL;
    typehandle->read        = NULL;
    typehandle->write       = &mouse_callback_handler;
    typehandle->seek        = NULL;
    typehandle->close       = NULL;
    typehandle->link        = NULL;
    typehandle->unlink      = NULL;
    lostio_register_typehandle(typehandle);

    vfstree_create_node("/mouse", LOSTIO_TYPES_DIRECTORY, 0, 0, 0);
    vfstree_create_node("/mouse/data", 254, 0, 0, 0);
    vfstree_create_node("/mouse/callback", 253, 0, 0, 0);


    register_intr_handler(32 + 12, &mouse_irq_handler);

    mouse_install();
}

//Quelle:
//http://www.osdev.org/phpBB2/viewtopic.php?t=10247

void mouse_wait(uint8_t a_type)
{
	uint32_t _time_out=100000;
	if(a_type==0)
	{
		while(_time_out--) //Daten
		{
			if((inb(0x64) & 1)==1)
			{
				return;
			}
		}
		return;
	}
	else
	{
		while(_time_out--) //Signal
		{
			if((inb(0x64) & 2)==0)
			{
			return;
			}
		}
		return;
	}
}

void mouse_write(uint8_t a_write)
{
    //Auf Bereitschaft der Maus warten
    mouse_wait(1);
    //Der Maus sagen, dass ein Befehl folgt
    outb(0x64, 0xD4);
    mouse_wait(1);
    //Befehl schreiben
    outb(0x60, a_write);
}

uint8_t mouse_read()
{
    //Antwort der Maus abfragen
    mouse_wait(0);
    return inb(0x60);
}

void mouse_install()
{
    uint8_t _status;

    p();

    //Mauscontroller aktivieren
    mouse_wait(1);
    outb(0x64, 0xA8);

    //Interrupts aktivieren
    mouse_wait(1);
    outb(0x64, 0x20);
    mouse_wait(0);
    _status=(inb(0x60) | 2);
    mouse_wait(1);
    outb(0x64, 0x60);
    mouse_wait(1);
    outb(0x60, _status);

    //Standardeinstellungen der Maus auswählen
    mouse_write(0xF6);
    mouse_read();

    //Maus aktivieren
    mouse_write(0xF4);
    mouse_read();

    v();
    mouse_cycle = 0;
}

void mouse_irq_handler(uint8_t irq)
{
    uint8_t status = inb(0x64);
    if (!(status & 0x20)) {
        return;
    }
    int i;
    switch(mouse_cycle)
    {
    case 0:
         mouse_byte[0]=inb(0x60);
         mouse_cycle++;
         break;
    case 1:
        mouse_byte[1]=inb(0x60);
        mouse_cycle++;
        break;
    case 2:
        mouse_byte[2]=inb(0x60);
        mouse_x=mouse_byte[1];
        ((int*)cb_buffer)[0] = mouse_x;
        rel_x += mouse_x;
        mouse_y=mouse_byte[2];
        ((int*)cb_buffer)[1] = -mouse_y;
        rel_y -= mouse_y; // kleine Zahl = oben
        
        cb_buffer[9] = 0;
        cb_buffer[10] = 0;
        if ((mouse_byte[0] & 0x07) != buttons)
        {
            if ((buttons & 0x01) != (mouse_byte[0] & 0x01))
            {
                if (buttons & 0x01) {
                    buttons_released |= 0x01;
                    cb_buffer[10] |= 0x01;
                } else {
                    buttons_pressed |= 0x01;
                    cb_buffer[9] |= 0x01;
                }
            }
            if ((buttons & 0x02) != (mouse_byte[0] & 0x02))
            {
                if (buttons & 0x02) {
                    buttons_released |= 0x02;
                    cb_buffer[10] |= 0x02;
                } else {
                    buttons_pressed |= 0x02;
                    cb_buffer[9] |= 0x02;
                }
            }
            if ((buttons & 0x04) != (mouse_byte[0] & 0x04))
            {
                if (buttons & 0x04) {
                    buttons_released |= 0x04;
                    cb_buffer[10] |= 0x04;
                } else {
                    buttons_pressed |= 0x04;
                    cb_buffer[9] |= 0x04;
                }
            }
      	    buttons = mouse_byte[0] & 0x07;
        }
        cb_buffer[8] = buttons;
        //Callbacks
        for (i = 0; i < cb_count; i++) {
            fwrite(cb_buffer,12,1,cbfiles[i]);
        }
        mouse_cycle=0;
        break;
    }
}

size_t mouse_read_handler(lostio_filehandle_t* filehandle, void* buf,
    size_t blocksize, size_t blockcount)
{
    size_t size = blocksize * blockcount;
    uint8_t* data_buffer = buf;

    if (size < 12)
    {
        //Festgelegte Größe der Ausgabe
        //TODO: Nur Position abfragen (Größe = 8)?
        return 0;
    }

    p();

    ((int*)data_buffer)[0] = rel_x;
    rel_x = 0;
    ((int*)data_buffer)[1] = rel_y;
    rel_y = 0;
    data_buffer[8] = buttons;
    data_buffer[9] = buttons_pressed;
    buttons_pressed = 0;
    data_buffer[10] = buttons_released;
    buttons_released = 0;
    data_buffer[11] = 0;
    v();
    
    return 11;
}

size_t mouse_callback_handler(lostio_filehandle_t *file, size_t blocksize,
    size_t blockcount, void *data)
{
    if (blocksize * blockcount > 5) {
        if (!strncmp(data, "add ", 4)) {
            
            FILE *newcb = fopen(data + 4, "w");
            if (!newcb) return blocksize * blockcount;
            
            FILE **tmp_cbfiles = malloc(sizeof(FILE *) * (cb_count + 1));
            memcpy(tmp_cbfiles, cbfiles, cb_count * sizeof(FILE *));
            tmp_cbfiles[cb_count] = newcb;
            if (cb_count > 0) {
                free(cbfiles);
            }
            cbfiles = tmp_cbfiles;
            
            char **tmp_cbfilenames = malloc(sizeof(char *) * (cb_count + 1));
            memcpy(tmp_cbfilenames, cbfilenames, cb_count * sizeof(char *));
            tmp_cbfilenames[cb_count] = strdup(data + 4);
            if (cb_count > 0) {
                free(cbfilenames);
            }
            cbfilenames = tmp_cbfilenames;
            cb_count++;
        } else if (!strncmp(data, "del ", 4)) {
            int i;
            int position = -1;
            for (i = 0; i < cb_count; i++) {
                if (!strcmp(cbfilenames[i], data + 4)) {
                    position = i;
                    break;
                }
            }
            if (position == -1) return blocksize * blockcount;
    
            fclose(cbfiles[position]);
            free(cbfilenames[position]);
            if (cb_count == 1) {
                free(cbfilenames);
                free(cbfiles);
                cbfiles = 0;
                cbfilenames = 0;
                cb_count = 0;
            } else {
                FILE **tmp_cbfiles = malloc(sizeof(FILE *) * (cb_count - 1));
                memcpy(tmp_cbfiles, cbfiles, position * sizeof(FILE *));
                memcpy(tmp_cbfiles + position, cbfiles + position + 1,
                       (cb_count - position - 1) * sizeof(FILE *));
                free(cbfiles);
                cbfiles = tmp_cbfiles;
                char **tmp_cbfilenames = malloc(sizeof(char *) * (cb_count - 1));
                memcpy(tmp_cbfilenames, cbfilenames, position * sizeof(char *));
                memcpy(tmp_cbfilenames + position, cbfilenames + position + 1,
                       (cb_count - position - 1) * sizeof(char *));
                free(cbfilenames);
                cbfilenames = tmp_cbfilenames;
                cb_count--;
            }
        }
    }
    return blocksize * blockcount;
}

