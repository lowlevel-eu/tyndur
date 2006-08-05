#ifndef PORTS_H
#define PORTS_H

/// out ///
extern void outb(unsigned short _port, unsigned char _data);  // byte
extern void outw(unsigned short _port, unsigned short _data); // word
extern void outl(unsigned short _port, unsigned long _data);  // long
extern void outb_wait(unsigned short _port, unsigned char _data);  // outb mit darauf folgender Verzögerung

/// in ///
unsigned short inw(unsigned short _port); // word
unsigned char inb(unsigned short _port);  // byte
unsigned long inl(unsigned short _port);  // long

#endif

