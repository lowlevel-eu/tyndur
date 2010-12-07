/*
 * Copyright (c) 2009 The tyndur Project. All rights reserved.
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
    &__tms_cmd_exec_not_found,
    "Command not found",

    &__tms_cmd_start_not_found,
    "Executable not found",

    &__tms_cmd_start_usage,
    "Usage: start <path> [arguments ...]",

    &__tms_cmd_cd_error,
    "Cannot change to directory '%s'\n",

    &__tms_cmd_cd_usage,
    "Usage: cd <directory>",

    &__tms_cmd_source_error,
    "Cannot execute script '%s'\n",

    &__tms_cmd_source_usage,
    "Usage: source <path>",

    &__tms_cmd_set_usage,
    "\nUsage: set [-d] [<variable>] [<value>]\n"
    "  -d: Set a default value. If the variable is already set, its value is\n"
    "      not changed\n",

    &__tms_cmd_help_text,
    "Available commands:\n"
    "  <path>          Start the program at <path> in the forground\n"
    "  start <path>    Start the program at <path> in the background\n"
    "  echo <text>     Display <text>\n"
    "\n"
    "  pwd             Show the name of the current directory\n"
    "  ls              Show directory contents\n"
    "  stat            Show information about a file\n"
    "  cd <path>       Change the current directory\n"
    "  mkdir <path>    Create a directory\n"
    "\n"
    "  cp <s> <d>      Copy file <s> to <d>\n"
    "  ln <s> <d>      Create a link <d> to <s>\n"
    "  symlink <s> <d> Create a symbolic link <d> to <s>\n"
    "  readlink <l>    Read the destination of link <l>\n"
    "  rm <path>       Delete a file\n"
    "\n"
    "  cat <path>      Show contents of a file\n"
    "  bincat <path>   Show binary contents of a file (byte-wise)\n"
    "  pipe <path>     In-/Output to a file (e.g. TCP connection)\n"
    "\n"
    "  date            Show current date and time\n"
    "  free            Show total system memory usage\n"
    "  ps              Show a list of all processes\n"
    "  pstree          Display the process hierarchy as a tree\n"
    "  kill <PID>      Terminate process <PID>\n"
    "  dbg_st <PID>    Show debug stack backtrace of process <PID>\n"
    "  set             Set or display environment variables\n"
    "  read <s>        Read a value for environment variable <s>\n"
    "  source <s>      Run script <s> in the active shell\n"
    "  sleep <n>       Wait <n> seconds\n"
    "  exit            Exit shell\n"
    "\n"
    "This help text won't fit on most computer screens.\n"
    "To see the commands above, press Shift+PgUp. Shift+PgDn scrolls back"
    " down.\n"
    "Any other key returns to the current position.\n"
    "\n",

    &__tms_shell_usage,
    "Usage:\n"
    "  Interactive: sh\n"
    "  Batch mode:  sh <script path>\n"
    "  Run command: sh -c <command>\n",

    &__tms_shell_script_error,
    "An error happened while running the script\n",

    0,
    0,
};

static const struct tms_lang lang = {
    .lang = "en",
    .numbers = 2,
    .get_number = get_number,
    .strings = dict,
};

LANGUAGE(&lang)
