#include "types.h"
#include "stdlib.h"
#include "stdio.h"
#include "unistd.h"
#include "config.h"

void pipe_display_usage();

#ifdef CONFIG_SHELL_BUILTIN_ONLY
    int shell_command_pipe(int argc, char* argv[], const char* args)
#else
    #define _USE_START_
    #include "init.h"
    int main(int argc, char* argv[])
#endif
{
    if (argc != 2) {
        pipe_display_usage();
        return -1;
    }
    
    char* path = argv[1];
    FILE* f = fopen(path, "r+");
    if (f != NULL) {
        char outbuf[1024];
        int outbuf_pos = 0;
        
        while (!feof(f)) { 
            char buf[1024];
            size_t length = fread(buf, 1023, 1, f);
            if (length > 0) {
                buf[length] = '\0';
                printf("%s", buf);
                fflush(stdout);
            }
            
            char c;
            if (fread(&c, 1, 1, stdin) && (c != 0)) {
                if (c == '\b') 
                {
                    if (outbuf_pos) {
                        outbuf_pos--;
                        printf("\033[1D \033[1D");
                        fflush(stdout);
                    }
                } else {
                    printf("%c", c);
                    fflush(stdout);
                    
                    outbuf[outbuf_pos++] = c;
                }
                
                if ((c == '\n') || (outbuf_pos == 1024)) {
                    if (!strncmp(outbuf, "EOF.\n", outbuf_pos)) {
                        break;
                    } else {
                        fwrite(outbuf, outbuf_pos, 1, f);
                        outbuf_pos = 0;
                    }
                }
            }
        }
        fclose(f);
        printf("\nVerbindung beendet.\n");
    } else {
        puts("Konnte Datei nicht oeffnen");
    }

    return EXIT_SUCCESS;
}

void pipe_display_usage()
{
    puts("\nAufruf: pipe <Dateiname>");
}

