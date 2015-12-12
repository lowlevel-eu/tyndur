/*
 * Copyright (c) 2011 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Patrick Pokatilo.
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
    &__tms_config_service_depends,
    "servmgr: service '%s' depends on '%s', which is not present in the configuration!\n",

    &__tms_config_dir_open,
    "servmgr: could not open configuration directory: '%s'\n",

    &__tms_main_usage,
    "usage: servmgr <root directory> <service terminal> [startup services ...]\n",

    &__tms_main_not_running,
    "service '%s' is not running!\n",

    &__tms_main_no_root,
    "servmgr: timeout while waiting for root filesystem\n",

    &__tms_service_not_found,
    "servmgr: service not found: '%s'\n",

    &__tms_service_dependency,
    "servmgr: could not resolve dependency on '%s'. Thus, '%s' can not be started.\n",

    &__tms_service_timeout,
    "servmgr: timeout while waiting for service '%s'\n",

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
