#include "syscall.h"
#include "syscallx.h"

void yield()
{
    int result;

    SYSCALL0(SYSCALL_PM_SLEEP, result);
}
