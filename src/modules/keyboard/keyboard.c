#include "types.h"
#include "syscall.h"
#include "rpc.h"
#include "stdlib.h"
#include "stdio.h" 
#include "lostio.h"
#include "ports.h"

#define _USE_START_
#include "init.h"

#define BUFFER_SIZE 1024
#define BUFFER_TYPE_RAW 1
#define BUFFER_TYPE_ASCII 2


void kbd_irq_handler();


byte kbd_scancode[256][2] =
{
    {0,               0},         // 0x00
    {0,               0},         // 0x01
    {'1',           '|'},         // 0x02
    {'2',           '¦'},         // 0x03 F5
    {'3',           '*'},         // 0x04 F3
    {'4',           'ç'},         // 0x05 F1
    {'5',           '%'},         // 0x06 F2
    {'6',           '&'},         // 0x07 F12
    {'7',           '/'},         // 0x08 F10
    {'8',           '('},         // 0x09
    {'9',           ')'},         // 0x0A F8
    {'0',           '='},         // 0x0B F6
    {39,            '?'},         // 0x0C'
    {'^',           '`'},         // 0x0D
    {'\b',         '\b'},         // 0x0E
    {'\t',         '\t'},         // 0x0F F4
    {'q',           'Q'},         // 0x10
    {'w',           'W'},         // 0x11
    {'e',           'E'},         // 0x12
    {'r',           'R'},         // 0x13
    {'t',           'T'},         // 0x14
    {'z',           'Z'},         // 0x15 q
    {'u',           'U'},         // 0x16 1
    {'i',           'I'},         // 0x17
    {'o',           'O'},         // 0x18
    {'p',           'P'},         // 0x19
    {'u',           'e'},         // 0x1A y
    {0,               0},         // 0x1B s
    {'\n',         '\n'},         // 0x1C a
    {0,               0},         // 0x1D w
    {'a',           'A'},         // 0x1E 2
    {'s',           'S'},         // 0x1F
    {'d',           'D'},         // 0x20
    {'f',           'F'},         // 0x21 c
    {'g',           'G'},         // 0x22 x
    {'h',           'H'},         // 0x23 d
    {'j',           'J'},         // 0x24 e
    {'k',           'K'},         // 0x25 4
    {'l',           'L'},         // 0x26 3
    {'o',           'e'},         // 0x27
    {'a',           'a'},         // 0x28
    {0,               0},      // 0x29 space
    {0,               0},      // 0x2A v
    {'$',           '£'},      // 0x2B f
    {'y',           'Y'},      // 0x2C t
    {'x',           'X'},      // 0x2D r
    {'c',           'C'},      // 0x2E 5
    {'v',           'V'},         // 0x2F
    {'b',           'B'},         // 0x30
    {'n',           'N'},      // 0x31 n
    {'m',           'M'},      // 0x32 b
    {',',           ';'},      // 0x33 h
    {'.',           ':'},      // 0x34 g
    {'-',           '_'},      // 0x35 z
    {0,               0},      // 0x36 6
    {0,               0},         // 0x37
    {0,               0},         // 0x38
    {' ',           ' '},         // 0x39
    {0,               0},      // 0x3A m
    {0,               0},      // 0x3B j
    {0,               0},      // 0x3C u
    {0,               0},      // 0x3D 7
    {0,               0},      // 0x3E 8
    {0,               0},         // 0x3F
    {0,               0},         // 0x40
    {0,               0},      // 0x41 ,
    {0,               0},      // 0x42 k
    {0,               0},      // 0x43 i
    {0,               0},      // 0x44 o
    {0,               0},      // 0x45 0
    {0,               0},      // 0x46 9
    {0,               0},         // 0x47
    {0,               0},         // 0x48
    {0,               0},      // 0x49 .
    {0,               0},      // 0x4A -
    {0,               0},      // 0x4B l
    {0,               0},      // 0x4C TODO
    {0,               0},      // 0x4D p
    {0,               0},      // 0x4E Ã?
    {0,               0},         // 0x4F
    {0,               0},         // 0x50
    {0,               0},         // 0x51
    {0,               0},      // 0x52 TODO
    {0,               0},         // 0x53
    {0,               0},      // 0x54 TODO
    {0,               0},      // 0x55 
    {0,               0},         // 0x56
    {0,               0},         // 0x57
    {0,               0},         // 0x58 capslock
    {0,               0},         // 0x59
    {0,               0},     // 0x5A enter
    {0,               0},      // 0x5B +
    {0,               0},         // 0x5C
    {0,               0},     // 0x5D #
    {0,               0},         // 0x5E
    {0,               0},         // 0x5F
    {0,               0},         // 0x60
    {0,               0},         // 0x61
    {0,               0},         // 0x62
    {0,               0},         // 0x63
    {0,               0},         // 0x64
    {0,               0},         // 0x65
    {0,               0},     // 0x66 backspace
    {0,               0},         // 0x67
    {0,               0},         // 0x68
    {0,               0},         // 0x69 numblock 1 //TODO
    {0,               0},         // 0x6A
    {0,               0},         // 0x6B numblock 6 //TODO
    {0,               0},         // 0x6C numblock 7 //TODO
    {0,               0},         // 0x6D
    {0,               0},         // 0x6E
    {0,               0},         // 0x6F
    {0,               0},         // 0x70 numblock 0 //TODO
    {0,               0},         // 0x71 numblock , //TODO
    {0,               0},         // 0x72 numblock 2 //TODO
    {0,               0},         // 0x73 numblock 5 //TODO
    {0,               0},         // 0x74 numblock 6 //TODO
    {0,               0},         // 0x75 numblock 8 //TODO
    {0,               0},         // 0x76 ESC
    {0,               0},         // 0x77 NUM
    {0,               0},         // 0x78 F11
    {0,               0},      // 0x79 numblock + //TODO
    {0,               0},         // 0x7A numblock 3 //TODO
    {0,               0},      // 0x7B numblock - //TODO
    {0,               0},      // 0x7C numblock * //TODO
    {0,               0},         // 0x7D numblock 9 //TODO
    {0,               0},         // 0x7E scroll lock
    {0,               0},         // 0x7F
    {0,               0},         // 0x80
    {0,               0},         // 0x81
    {0,               0},         // 0x82
    {0,               0},         // 0x83 F7
    {0,               0},         // 0x84
    {0,               0},         // 0x85
    {0,               0},         // 0x86
    {0,               0},         // 0x87
    {0,               0},         // 0x88
    {0,               0},         // 0x89
    {0,               0},         // 0x8A
    {0,               0},         // 0x8B
    {0,               0},         // 0x8C
    {0,               0},         // 0x8D
    {0,               0},         // 0x8E
    {0,               0},         // 0x8F
};


size_t kbd_buffer_pos;
byte* kbd_ascii_buffer;
byte* kbd_work_buffer;

read_hdl_reply  kbd_read_handler(lostio_filehandle_t* filehandle, size_t blocksize, size_t blockcount);
void send_cmd(byte cmd);



int main(int argc, char* argv[])
{
    lostio_init();
    lostio_type_ramfile_use();
    lostio_type_directory_use();


    kbd_ascii_buffer = malloc(BUFFER_SIZE);
    kbd_work_buffer = malloc(BUFFER_SIZE);
    kbd_buffer_pos = 0;
  
    //Speicher fuer ein neues typehandle allokieren
    typehandle_t* typehandle = malloc(sizeof(typehandle_t));
    typehandle->id          = 255;
    typehandle->pre_open    = NULL;
    typehandle->post_open   = NULL;
    typehandle->read        = &kbd_read_handler;
    typehandle->write       = NULL;
    typehandle->seek        = NULL;
    typehandle->close       = NULL;
    lostio_register_typehandle(typehandle);

    //vfstree_create_node("/raw", 255, 0, (void*)BUFFER_TYPE_RAW);
    vfstree_create_node("/ascii", 255, 0, (void*)BUFFER_TYPE_ASCII, 0);



    register_intr_handler(0x21, &kbd_irq_handler);

    /*ile ((inb(0x64) & 0x02) != 0) yield();
    outb(0x60, 0xf0);
    while (inb(0x60) != 0xfa) yield();
    puts("bla");
    
    while ((inb(0x64) & 0x02) != 0) yield();
    outb(0x60, 0x2);
    while (inb(0x60) != 0xfa) yield();
    puts("bla");*/
    //send_cmd(0xf0);
    //send_cmd(0x2);

    init_service_register("keyboard");
    while(1) yield();
}


void send_cmd(byte cmd)
{
    while ((inb(0x64) & 0x02) != 0) yield();
    outb(0x60, cmd);
    while (inb(0x60) != 0xfa) yield();
}


void kbd_irq_handler() {
    byte scancode = inb(0x60);
    byte ascii;
    bool breakcode;
    static bool state_caps = FALSE;
    
    //Ascii-Zeichen fuer diese Taste holen
    if(state_caps == TRUE)
    {
        ascii = kbd_scancode[scancode][1];
    }
    else
    {
        ascii = kbd_scancode[scancode][0];
    }

    // 
    if(scancode > 0x80)
    {
        breakcode = TRUE;
    }
    else
    {
        breakcode = FALSE;
    }

    //Eine shift taste
    if(((scancode & (~0x80)) == 0x2a) || ((scancode & (~0x80)) == 0x36))
    {
        state_caps = !breakcode;
    }
    else if(ascii != 0)
    {
        if(kbd_buffer_pos < BUFFER_SIZE)
        {
            kbd_ascii_buffer[kbd_buffer_pos++] = ascii;
        }
        else
        {
            printf("\n[KEYBOADRD] Buffer full\n");
        }
    }
}


read_hdl_reply  kbd_read_handler(lostio_filehandle_t* filehandle, size_t blocksize, size_t blockcount)
{
    read_hdl_reply reply;
    byte type = (byte) filehandle->node->data;
    size_t size = blocksize * blockcount;
    
    if(size > kbd_buffer_pos)
    {
        size = kbd_buffer_pos;
    }

    
    reply.size = size;
    reply.data = kbd_work_buffer;

    memcpy(kbd_ascii_buffer, (void*) ((dword)kbd_ascii_buffer + size), kbd_buffer_pos - size);
    memcpy(kbd_work_buffer, kbd_ascii_buffer, size);
    kbd_buffer_pos -= size;

    
    return reply;
}

