#define _USE_START_
#include "init.h"

struct cpu {
	int MHZ;
};

/// TODO: Prüfen ob cpuid überhaupt zur verfügung steht.



void cpuid_is()
{
	int result;
	asm (
		"pushfl;"
		"popl %%eax;"
		"movl %%ebx, %%eax;"
		"xorl $0x200000, %%eax;"
		"pushl %%eax;"
		"popfl;"
		"pushfl;"
		"pop %%eax;"
		"xorl %%eax, %%ebx;"
		: "=a"(result)
	);
	return result;
}

void cpuid(dword* eax, dword* ebx, dword* ecx, dword* edx)
{
	asm volatile("cpuid"
				: "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
				: "a"(eax), "b"(ebx), "c"(ecx), "d"(edx));
}

int main(int argc, char* argv[])
{
	puts("|CPUID| Module loaded.\n");
}
