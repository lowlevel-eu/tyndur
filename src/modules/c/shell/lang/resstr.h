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
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL;
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO;
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY;
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef RESSTR_H
#define RESSTR_H

extern void* __tms_cmd_exec_not_found;
extern void* __tms_cmd_start_not_found;
extern void* __tms_cmd_start_usage;
extern void* __tms_cmd_cd_error;
extern void* __tms_cmd_cd_usage;
extern void* __tms_cmd_source_error;
extern void* __tms_cmd_source_usage;
extern void* __tms_cmd_set_usage;
extern void* __tms_cmd_help_text;

extern void* __tms_shell_usage;
extern void* __tms_shell_script_error;
extern void* __tms_shell_fail_dup;
extern void* __tms_shell_parse_error;
extern void* __tms_shell_err_end_squot;
extern void* __tms_shell_err_end_dquot;

extern void* __tms_bench_usage;
extern void* __tms_bench_opening_error;
extern void* __tms_bench_reading_error;
extern void* __tms_bench_message_no1;
extern void* __tms_bench_message_no2;

extern void* __tms_bincat_usage;
extern void* __tms_bincat_invalid_start;
extern void* __tms_bincat_invalid_length;
extern void* __tms_bincat_invalid_end;
extern void* __tms_bincat_length_end;
extern void* __tms_bincat_files_error;
extern void* __tms_bincat_opening_error;

extern void* __tms_cat_usage;
extern void* __tms_cat_opening_error;
extern void* __tms_cat_reading_error;

extern void* __tms_cp_usage;
extern void* __tms_cp_error_dst_no_dir;
extern void* __tms_cp_skip;
extern void* __tms_cp_src_opening_error;
extern void* __tms_cp_dst_opening_error;
extern void* __tms_cp_error_dst_path;
extern void* __tms_cp_error_create_dst;
extern void* __tms_cp_error_src_read;

extern void* __tms_date_usage;
extern void* __tms_date_opening_error;

extern void* __tms_dbg_st_usage;

extern void* __tms_echo_usage;

extern void* __tms_free_usage;
extern void* __tms_free_used;
extern void* __tms_free_free;
extern void* __tms_free_all;

extern void* __tms_kill_usage;
extern void* __tms_kill_not_found;
extern void* __tms_kill_error;

extern void* __tms_killall_usage;
extern void* __tms_killall_not_found;
extern void* __tms_killall_cant_kill;
extern void* __tms_killall_killed;

extern void* __tms_ln_usage;
extern void* __tms_ln_error;

extern void* __tms_ls_usage;
extern void* __tms_ls_opening_error;

extern void* __tms_mkdir_usage;
extern void* __tms_mkdir_error;

extern void* __tms_pipe_usage;
extern void* __tms_pipe_opening_error;
extern void* __tms_pipe_close;

extern void* __tms_ps_usage;
extern void* __tms_ps_with_eip;
extern void* __tms_ps_without_eip;

extern void* __tms_pstree_usage;

extern void* __tms_pwd_usage;

extern void* __tms_read_usage;

extern void* __tms_readlink_usage;
extern void* __tms_readlink_error;

extern void* __tms_rm_usage;
extern void* __tms_rm_invalid_param;
extern void* __tms_rm_help;
extern void* __tms_rm_isdir;
extern void* __tms_rm_doesnt_exist;
extern void* __tms_rm_question_del;
extern void* __tms_rm_message_skip;
extern void* __tms_rm_message_del;
extern void* __tms_rm_error;

extern void* __tms_sleep_usage;

extern void* __tms_stat_usage;
extern void* __tms_stat_error;
extern void* __tms_stat_message_no1;
extern void* __tms_stat_message_no2;
extern void* __tms_stat_message_no3;
extern void* __tms_stat_message_no4;
extern void* __tms_stat_message_no5;

extern void* __tms_symlink_usage;
extern void* __tms_symlink_error;

#endif
