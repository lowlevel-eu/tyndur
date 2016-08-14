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
    "\nUsage: start <path> [arguments ...]\n",

    &__tms_cmd_cd_error,
    "Cannot change to directory '%s'\n",

    &__tms_cmd_cd_usage,
    "\nUsage: cd <directory>\n",

    &__tms_cmd_source_error,
    "Cannot execute script '%s'\n",

    &__tms_cmd_source_usage,
    "\nUsage: source <path>\n",

    &__tms_cmd_set_usage,
    "\nUsage: set [-d] [<variable>] [<value>]\n\n"
    "Options:\n"
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
    "  sync            Write all changes from the cache to disk\n"
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
    "Any other key returns to the current position.\n",



    &__tms_shell_usage,
    "\nUsage:\n"
    "  Interactive: sh\n"
    "  Batch mode:  sh <script path>\n"
    "  Run command: sh -c <command>\n",

    &__tms_shell_script_error,
    "An error happened while running the script\n",

    &__tms_shell_fail_dup,
    "lio_dup() failed: %s\n",

    &__tms_shell_parse_error,
    "Parse error in '%s'\n",

    &__tms_shell_err_end_dquot,
    "Could not find closing '\"'\n",

    &__tms_shell_err_end_squot,
    "Could not find closing '\''\n",



    &__tms_bench_usage,
    "\nUsage: bench [-s] <filename> <kilobytes>\n",

    &__tms_bench_opening_error,
    "Couldn't open '%s' for reading!\n",

    &__tms_bench_reading_error,
    "An error occurred while reading!\n",

    &__tms_bench_message_no1,
    "\rBlocksize: %6d; %5d/%5d kB left; Rate: -- bytes/s [%lld s] ",

    &__tms_bench_message_no2,
    "\rBlocksize: %6d; %5d/%5d kB left; Rate: %10lld bytes/s [%lld s] ",



    &__tms_bincat_usage,
    "\nUsage: bincat [options] <filename>\n\n"
    "Options:\n"
    "  -s, --start   start position\n"
    "  -l, --length  output length\n"
    "  -e, --end     end position\n\n"
    "If --length and --end are both specificed, --length takes precedence.\n\n"
    "Position and length can be also specified in hexadecimal\n"
    "notation when prefixed by '0x'. They have to be multiples of\n"
    "the blocksize (%d bytes).\n",

    &__tms_bincat_invalid_start,
    "Invalid 'start' value!\n",

    &__tms_bincat_invalid_length,
    "Invalid 'length' value!\n",

    &__tms_bincat_invalid_end,
    "Invalid 'end' value!\n",

    &__tms_bincat_length_end,
    "--length and --end cannot be used at the same time. Using --length.\n",

    &__tms_bincat_files_error,
    "Error: bincat can only open one file at a time!\n",

    &__tms_bincat_opening_error,
    "Couldn't open '%s' for reading!\n",



    &__tms_cat_usage,
    "\nUsage: cat <filename>\n",

    &__tms_cat_opening_error,
    "Couldn't open '%s' for reading!\n",

    &__tms_cat_reading_error,
    "An error occurred while reading!\n",



    &__tms_cp_usage,
    "\nUsage: cp [options] <source> <destination>\n"
    "  or: cp [options] <source> ... <directory>\n"
    "  or: cp [options] -t <directory> <destination>\n\n"
    "Options:\n"
    "  -v, --verbose    verbose mode\n"
    "  -r, --recursive  copy directories recursively\n"
    "  -h, --help       show this help\n",

    &__tms_cp_error_dst_no_dir,
    "Error: Got mulitple sources, but destination is not a directory!\n",

    &__tms_cp_skip,
    "%s skipped (Did you mean 'cp -r'?)\n",

    &__tms_cp_src_opening_error,
    "Couldn't open source file!\nPath: '%s'\n",

    &__tms_cp_dst_opening_error,
    "Couldn't open destination file!\nPath: '%s'\n",

    &__tms_cp_error_dst_path,
    "An error occurred while setting targetpath!\n",

    &__tms_cp_error_create_dst,
    "Error: Path '%s' couldn't be created!\n"
    "       skipped '%s'.\n",

    &__tms_cp_error_src_read,
    "An error has occurred while reading the source file!\n",



    &__tms_date_usage,
    "\nUsage: date\n",

    &__tms_date_opening_error,
    "Couldn't open '%s' for reading!\n",



    &__tms_dbg_st_usage,
    "\nUsage: dbg_st <PID>\n",



    &__tms_echo_usage,
    "\nUsage: echo <arguments>\n",



    &__tms_free_usage,
    "\nUsage: free\n",

    &__tms_free_used,
    "\n%10d bytes used\n",

    &__tms_free_free,
    "%10d bytes free\n",

    &__tms_free_all,
    "%10d bytes total\n",



    &__tms_kill_usage,
    "\nUsage: kill <PID>\n",

    &__tms_kill_not_found,
    "Process %d not found!\n",

    &__tms_kill_error,
    "Couldn't kill process %d: %d (%s)\n",



    &__tms_killall_usage,
    "usage: killall <name>\n",

    &__tms_killall_not_found,
    "no matching process found\n",

    &__tms_killall_killed,
    "pid %d killed\n",

    &__tms_killall_cant_kill,
    "couldn't kill pid %d - %s: %s (%s)\n",



    &__tms_ln_usage,
    "\nUsage: ln <target> <link-path>\n\n"
    "Create hardlink to file/directory <target> at <link-path>.\n",

    &__tms_ln_error,
    "An error has occurred while creating the link!\n",



    &__tms_ls_usage,
    "\nUsage: ls [options] [directories]\n\n"
    "Options:\n"
    "  -a, --all             show hidden files\n"
    "  -C, --color           colorize output\n"
    "  -h, --human-readable  show filesize human-readable, (e.g. 10M, 1G)\n"
    "      --help            show this help\n",

    &__tms_ls_opening_error,
    "Couldn't open '%s' for reading!\n",



    &__tms_mkdir_usage,
    "\nUsage: mkdir <directory name>\n",

    &__tms_mkdir_error,
    "Couldn't create directory '%s'!",



    &__tms_pipe_usage,
    "\nUsage: pipe [-c|-a] <filename>\n",

    &__tms_pipe_opening_error,
    "Couldn't open file!\n",

    &__tms_pipe_close,
    "\nConnection closed.\n",



    &__tms_ps_usage,
    "\nUsage: ps [--with-eip]\n",

    &__tms_ps_with_eip,
    " PID  PPID Status  EIP        Memory  CMD\n",

    &__tms_ps_without_eip,
    " PID  PPID Status   Memory  CMD\n",



    &__tms_pstree_usage,
    "\nUsage: pstree\n",



    &__tms_pwd_usage,
    "\nUsage: pwd\n",



    &__tms_read_usage,
    "\nUsage: read <env>\n\n"
    "Read a value for environment variable <env>.\n",


    &__tms_readlink_usage,
    "\nUsage: readlink <link-path>\n",

    &__tms_readlink_error,
    "Error: Symlink couldn't be read!\n",



    &__tms_rm_usage,
    "\nUsage: rm [options] <path>\n\n",

    &__tms_rm_invalid_param,
    "Invalid parameter: '%c'\n",

    &__tms_rm_help,
    "Try 'rm --help' for more information.\n",

    &__tms_rm_isdir,
    "'%s/' is a directory: skipped\n",

    &__tms_rm_doesnt_exist,
    "'%s': File doesn't exist!\n",

    &__tms_rm_question_del,
    "Delete '%s'?\n",

    &__tms_rm_message_skip,
    "Skipped '%s'.\n'",

    &__tms_rm_message_del,
    "Removing '%s'.\n",

    &__tms_rm_error,
    "Couldn't remove '%s'!\n",



    &__tms_sleep_usage,
    "\nUsage: sleep <seconds>\n",



    &__tms_stat_usage,
    "\nUsage: stat <path>\n",

    &__tms_stat_error,
    "Error: File not found!\n",

    &__tms_stat_message_no1,
    "  File: '%s'\n",

    &__tms_stat_message_no2,
    "  Size: %8d  Blocks: %8d  Block size: %8d\n",

    &__tms_stat_message_no3,
    "  Type: ",

    &__tms_stat_message_no4,
    "Directory",

    &__tms_stat_message_no5,
    "File",



    &__tms_symlink_usage,
    "\nUsage: symlink <link-path> <path>\n",

    &__tms_symlink_error,
    "Error: Couldn't create symlink!\n",

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
