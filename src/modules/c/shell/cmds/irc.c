#include "types.h"
#include "stdlib.h"
#include "stdio.h"
#include "unistd.h"
#include "config.h"

void irc_display_usage();

void irc_set_nick(char* nick);
void irc_set_user(char* user, char* fullname);
void irc_join(char* channel);

FILE* connection;

#ifdef CONFIG_SHELL_BUILTIN_ONLY
    int shell_command_irc(int argc, char* argv[], const char* args)
#else
    #define _USE_START_
    #include "init.h"
    int main(int argc, char* argv[])
#endif
{
    if (argc != 4) {
        irc_display_usage();
        return -1;
    }

    char* path = argv[1];
    char* nick = argv[2];
    char* channel = argv[3];

    FILE* f = fopen(path, "r+");
    connection = f;
    if (f != NULL) {
        //char* usernick = "nick FreakyPenguin|lost\nuser LOST 0 0 :LOST-Benutzer\n";
        //fwrite(usernick, strlen(usernick), 1, f);
        irc_set_nick(nick);
        irc_set_user("LOST", "LOST-Benutzer");
        //irc_join(channel);

        char outbuf[1024];
        int outbuf_pos = 0;
        
        while (!feof(f)) { 
            char buf[1024];
            size_t length = fread(buf, 1023, 1, f);
            
            // Nur wenn Daten gelesen wurden, muessen auch welche verarbeitet
            // werden.
            if (length > 0) {
                buf[length] = '\0';
                char* line = strtok(buf, "\n");

                while (line != NULL) {
                    if (strncmp(line, "PING", 4) == 0) {
                        line[1] = 'O';
                        fwrite(line, length, 1, f);
                    } else {
                        printf("%s\n", line);
                    }

                    line = strtok(NULL, "\n");
                }
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

void irc_display_usage()
{
    puts("\nAufruf: irc <Dateiname> <Nickname> <Kanal>");
}

void irc_set_nick(char* nick)
{
    size_t cmd_size = strlen(nick) + 7;
    char cmd[cmd_size];
    char* ptr = cmd;

    memcpy(ptr, "NICK ", 5);
    ptr += 5;

    memcpy(ptr, nick, strlen(nick));
    ptr += strlen(nick);

    memcpy(ptr, "\n", 2);

    fwrite(cmd, cmd_size, 1, connection);
}

void irc_set_user(char* user, char* fullname)
{
    size_t cmd_size = strlen(user) + strlen(fullname) + 13;
    char cmd[cmd_size];
    char* ptr = cmd;

    memcpy(ptr, "USER ", 5);
    ptr += 5;

    memcpy(ptr, user, strlen(user));
    ptr += strlen(user);

    memcpy(ptr , " 0 0 :", 6);
    ptr += 6;

    memcpy(ptr, fullname, strlen(fullname));
    ptr += strlen(fullname);

    memcpy(ptr, "\n", 2);
    
    fwrite(cmd, cmd_size - 1, 1, connection);
}

void irc_join(char* channel)
{
    size_t cmd_size = strlen(channel) + 7;
    char cmd[cmd_size];
    char* ptr = cmd;

    memcpy(ptr, "JOIN ", 5);
    ptr += 5;

    memcpy(ptr, channel, strlen(channel));
    ptr += strlen(channel);

    memcpy(ptr, "\n", 2);

    fwrite(cmd, cmd_size - 1, 1, connection);
}


