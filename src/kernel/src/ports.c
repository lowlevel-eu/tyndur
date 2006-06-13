 void out(unsigned short _port, unsigned char _data)
{
	__asm__ ("out %%al, %%dx" : : "a" (_data), "d" (_port));
}

