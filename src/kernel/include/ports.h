#ifndef PORTS_H
#define PORTS_H

extern inline unsigned short inw(unsigned short _port)
{
	unsigned short result;
	__asm__ ("inw %1, %0" : "=a" (result) : "Nd" (_port));
	return result;
}

/// in in byte ///
extern inline unsigned char inb(unsigned short _port)
{
	unsigned char result;
	__asm__ ("inb %1, %0" : "=a" (result) : "Nd" (_port));
	return result;
}

/// in in long(32 bit) ///
extern inline unsigned long inl(unsigned short _port)
{
	unsigned long result;
	__asm__ ("inl %1, %0" : "=a" (result) : "Nd" (_port));
	return result;
}



/// out in dword ///
extern inline void outw(unsigned short _port, unsigned short _data)
{
	__asm__ ("outw %0, %1" : : "a" (_data), "Nd" (_port));
}

/// out in byte ///
extern inline void outb(unsigned short _port, unsigned char _data)
{
	__asm__ ("outb %0, %1" : : "a" (_data), "Nd" (_port));
}

/// out in long(32 bit) ///
extern inline void outl(unsigned short _port, unsigned long _data)
{
	__asm__ ("outl %0, %1" : : "a"(_data), "Nd" (_port));
}

/* Ein Byte an einen IO Port senden und für langsame Ports kurz verzögern */
extern inline void outb_wait(unsigned short _port, unsigned char _data)
{
	__asm__ ("outb %0, %1\njmp 1f\n1: jmp 1f\n1:" : : "a" (_data), "Nd" (_port));
}

#endif

