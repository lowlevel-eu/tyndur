
#include "types.h"
#include "syscall.h"
#include "stdlib.h"
#include "stdio.h"
#include "unistd.h"
#include "string.h"
#include "rpc.h"
#include "io.h"
#include "dir.h"
#include "lostio.h"
#include "config.h"

#define _USE_START_
#include "init.h"

int main(int argc, char* argv[])
{
    qword timeout = get_tick_count() + 20000000;
    while ((init_service_get("kbc") == 0) || (init_service_get("console") == 0))
    {
        yield();
        if(get_tick_count() > timeout)
        {
            if (init_service_get("kbc") == 0)
            {
                puts("[ MOUSETEST ] Konnte den Maustreiber nicht finden!");
            }
            
            if(init_service_get("console") == 0)
            {
              puts("[ MOUSETEST ] Konnte den Konsolentreiber nicht finden!");
            }

            puts("[ MOUSETEST ] Mousetest beendet!");

            return -1;
        }
    }
    
    FILE *mouse;
    mouse = fopen("kbc:/mouse/data", "r");
    stdout = fopen("console:/stdout", "r");
    setvbuf(stdout, malloc(128), _IOLBF, 128);
    char mousedata[12];
    float x = 320;
    float y = 240;
    printf("\n \033[s");
    while (TRUE)
    {
    	if (fread(mousedata, 12, 1, mouse))
    	{
    		x += ((int*)mousedata)[0];
    		y += ((int*)mousedata)[1];
    		if (x > 640) x = 640;
    		if (x < 0) x = 0;
    		if (y > 480) y = 480;
    		if (y < 0) y = 0;
    		printf("\033[u");
    		printf("%3d/%3d, ", (int)x, (int)y);
    		int i;
    		byte buttons = mousedata[8];
    		for (i = 0; i < 8; i++)
    		{
    			printf("%c", (buttons & 0x01)?('1'):('0'));
    			buttons = buttons >> 1;
    		}
    	}
    }
}
