/*
 * Copyright (c) 2007 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
 * by Kevin Wolf.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *     This product includes software developed by the LOST Project
 *     and its contributors.
 * 4. Neither the name of the LOST Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "types.h"
#include "stdlib.h"
#include "stdio.h"
#include "unistd.h"
#include <lost/config.h>
#include "sleep.h"

void irc_display_usage(void);

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
    //char* channel = argv[3];

    FILE* f = fopen(path, "r+");
    connection = f;
    if (f != NULL) {
        //char* usernick = "nick FreakyPenguin|lost\nuser LOST 0 0 :LOST-Benutzer\n";
        //fwrite(usernick, strlen(usernick), 1, f);
        irc_set_nick(nick);
        msleep(100);
        irc_set_user("LOST", "LOST-Benutzer");
        msleep(100);
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
                    if (0 && strncmp(line, "PING", 4) == 0) {
                        line[1] = 'O';
                        printf("PONG!\n");
                        fwrite(line, 1, length, f);
                        msleep(100);
                        fwrite("\n", 1, 1, f);
                        //fprintf(f, "%s\n", line);
                        fprintf(stdout, "%s\n", line);
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


