/// in in dword ///
unsigned short inw(unsigned short _port)
{
	unsigned short result;
	__asm__ ("in %%dx, %%al" : "=a" (result) : "d" (_port));
	return result;
}

/// in in byte ///
unsigned char inb(unsigned short _port)
{
	unsigned char result;
	__asm__ ("in %%dx, %%al" : "=a" (result) : "d" (_port));
	return result;
}

/// in in long(32 bit) ///
unsigned long inl(unsigned short _port)
{
	unsigned long result;
	__asm__ ("in %%dx, %%al" : "=a" (result) : "d" (_port));
	return result;
}

/// out in dword ///
void outw(unsigned short _port, unsigned short _data)
{
	__asm__ ("out %%al, %%dx" : : "a" (_data), "d" (_port));
}

/// out in byte ///
void outb(unsigned short _port, unsigned char _data)
{
	__asm__ ("out %%al, %%dx" : : "a" (_data), "d" (_port));
}

/// out in long(32 bit) ///
void outl(unsigned short _port, unsigned long _data)
{
	__asm__ ("out %%al, %%dx" : : "a"(_data), "d" (_port));
}

