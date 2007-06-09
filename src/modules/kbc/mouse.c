
#include "types.h"
#include "rpc.h"
#include "stdlib.h"
#include "stdio.h" 
#include "lostio.h"
#include "ports.h"

#include "mouse.h"


byte mouse_cycle=0;
sbyte mouse_byte[3];
sbyte mouse_x=0;
sbyte mouse_y=0;

//Mausdaten
//Bis auf buttons werden alle bei jedem Lesen der Daten zurückgesetzt
byte buttons = 0;
byte buttons_pressed = 0;
byte buttons_released = 0;
int rel_x = 0;
int rel_y = 0;
byte data_buffer[12];

void mouse_init(void)
{
    //Speicher fuer ein neues typehandle allokieren
    typehandle_t* typehandle = malloc(sizeof(typehandle_t));
    typehandle->id          = 254;
    typehandle->not_found   = NULL;
    typehandle->pre_open    = NULL;
    typehandle->post_open   = NULL;
    typehandle->read        = &mouse_read_handler;
    typehandle->write       = NULL;
    typehandle->seek        = NULL;
    typehandle->close       = NULL;
    lostio_register_typehandle(typehandle);

    vfstree_create_node("/mouse", LOSTIO_TYPES_DIRECTORY, 0, 0, 0);
    vfstree_create_node("/mouse/data", 254, 0, 0, 0);
    
    
    register_intr_handler(32 + 12, &mouse_irq_handler);
    
    mouse_install();
}

//Quelle:
//http://www.osdev.org/phpBB2/viewtopic.php?t=10247

void mouse_wait(byte a_type)
{
	dword _time_out=100000;
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

void mouse_write(byte a_write)
{
    //Auf Bereitschaft der Maus warten
    mouse_wait(1);
    //Der Maus sagen, dass ein Befehl folgt
    outb(0x64, 0xD4);
    mouse_wait(1);
    //Befehl schreiben
    outb(0x60, a_write);
}

byte mouse_read()
{
    //Antwort der Maus abfragen
    mouse_wait(0);
    return inb(0x60);
}

void mouse_install()
{
    byte _status;
    
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
    
    mouse_cycle = 0;
}

void mouse_irq_handler()
{
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
        rel_x += mouse_x;
        mouse_y=mouse_byte[2];
        rel_y -= mouse_y; // kleine Zahl = oben
        if ((mouse_byte[0] & 0x07) != buttons)
        {
            if ((buttons & 0x01) != (mouse_byte[0] & 0x01))
            {
                if (buttons & 0x01) buttons_released |= 0x01;
                else buttons_pressed |= 0x01;
            }
            if ((buttons & 0x02) != (mouse_byte[0] & 0x02))
            {
                if (buttons & 0x02) buttons_released |= 0x02;
                else buttons_pressed |= 0x02;
            }
            if ((buttons & 0x04) != (mouse_byte[0] & 0x04))
            {
                if (buttons & 0x04) buttons_released |= 0x04;
                else buttons_pressed |= 0x04;
            }
      	    buttons = mouse_byte[0] & 0x07;
        }
        mouse_cycle=0;
        break;
    }
}

read_hdl_reply  mouse_read_handler(lostio_filehandle_t* filehandle,
                                   size_t blocksize,
                                   size_t blockcount)
{
    read_hdl_reply reply;
    size_t size = blocksize * blockcount;
    if (size < 12)
    {
        //Festgelegte Größe der Ausgabe
        //TODO: Nur Position abfragen (Größe = 8)?
        reply.size = 0;
        reply.data = 0;
        return reply;
    }
    
    p();
    
    reply.size = 11;
    reply.data = data_buffer;

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
    
    return reply;
}
