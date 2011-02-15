/*
 * Copyright (c) 2011 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
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

#include <tms.h>
#include "resstr.h"

static int get_number(int n)
{
    return (n == 1 ? 0 : 1);
}

static const struct tms_strings dict[] = {
    {&__tms_ftp_host_not_reachable,
    "\033[1;37mError: Host not reachable\033[0m"},

    {&__tms_ftp_connecting,
    "\033[1;37mConnecting...\033[0m"},

    {&__tms_ftp_connected,
    "\033[1;37mConnected with %s, waiting for reply...\033[0m\n"},

    {&__tms_ftp_prompt_user,
    "\033[1;37mUser (%s): \033[0m"},

    {&__tms_ftp_prompt_password,
    "\033[1;37mPassword: \033[0m"},

    {&__tms_ftp_disconnected,
    "\033[1;37mDisconnected...\033[0m"},

    {&__tms_ftp_data_connect,
    "\033[1;37mOpening data connection...\033[0m"},

    {&__tms_ftp_data_disconnect,
    "\033[1;37mData connection terminated\033[0m"},

    {&__tms_ftp_no_connection,
    "\033[1;37mError: Not connected...\033[0m"},

    {&__tms_ftp_usage,
    "FTP-Client command list:\n\n"
    "ascii   - switch to ascii mode\n"
    "binary  - switch to binary mode\n"
    "bye     - close any open server connection and quit\n"
    "cd      - change active remote directory\n"
    "cdup    - change to remote root directory\n"
    "clear   - clear the terminal\n"
    "close   - close server connection\n"
    "get     - [path+file name] get a file from the server\n"
    "help    - show this help text\n"
    "mkdir   - [path+dir name] create a directory on the server\n"
    "open    - [ftp.name.net] open a connection to a server\n"
    "put     - [path+file name] save a file on the server\n"
    "pwd     - show current working directory\n"
    "rm      - [path+file name] delete a file on the server\n"
    "rmdir   - [path+dir name] delete a directory on the server\n"
    "system  - show the name of the remote operating system\n"
    "user    - log in with user and password\n"},

    {&__tms_ftp_type_help,
    "\033[1;37mType `help` to get a list of available commands!\033[0m"},

    {0,
    0},
};

static const struct tms_lang lang = {
    .lang = "en",
    .numbers = 2,
    .get_number = get_number,
    .strings = dict,
};

LANGUAGE(&lang)

