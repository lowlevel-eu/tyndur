#ifndef PORTS_H
#define PORTS_H

extern inline word inw(word _port)
{
	word result;
	__asm__ ("inw %1, %0" : "=a" (result) : "Nd" (_port));
	return result;
}

/// in in byte ///
extern inline byte inb(word _port)
{
	byte result;
	__asm__ ("inb %1, %0" : "=a" (result) : "Nd" (_port));
	return result;
}

/// in in long(32 bit) ///
extern inline dword inl(word _port)
{
	dword result;
	__asm__ ("inl %1, %0" : "=a" (result) : "Nd" (_port));
	return result;
}



/// out in dword ///
extern inline void outw(word _port, word _data)
{
	__asm__ ("outw %0, %1" : : "a" (_data), "Nd" (_port));
}

/// out in byte ///
extern inline void outb(word _port, byte _data)
{
	__asm__ ("outb %0, %1" : : "a" (_data), "Nd" (_port));
}

/// out in long(32 bit) ///
extern inline void outl(word _port, dword _data)
{
	__asm__ ("outl %0, %1" : : "a"(_data), "Nd" (_port));
}

/* Ein Byte an einen IO Port senden und für langsame Ports kurz verzögern */
extern inline void outb_wait(word _port, byte _data)
{
	__asm__ ("outb %0, %1\njmp 1f\n1: jmp 1f\n1:" : : "a" (_data), "Nd" (_port));
}

#endif

