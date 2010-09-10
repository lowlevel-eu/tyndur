/*
 * Copyright (c) 2010 tyndur Project. All rights reserved.
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

#ifndef _INTTYPES_H_
#define _INTTYPES_H_

#include <stdint.h>

#define PRId8 "%d"
#define PRIu8 "%u"
#define PRIi8 "%i"
#define PRIo8 "%o"
#define PRIx8 "%x"
#define PRIX8 "%X"

#define PRId16 "%d"
#define PRIu16 "%u"
#define PRIi16 "%i"
#define PRIo16 "%o"
#define PRIx16 "%x"
#define PRIX16 "%X"

#define PRId32 "%d"
#define PRIu32 "%u"
#define PRIi32 "%i"
#define PRIo32 "%o"
#define PRIx32 "%x"
#define PRIX32 "%X"

#define PRId64 "%lld"
#define PRIu64 "%llu"
#define PRIi64 "%lli"
#define PRIo64 "%llo"
#define PRIx64 "%llx"
#define PRIX64 "%llX"


#define PRIdFAST8 "%d"
#define PRIuFAST8 "%u"
#define PRIiFAST8 "%i"
#define PRIoFAST8 "%o"
#define PRIxFAST8 "%x"
#define PRIXFAST8 "%X"

#define PRIdFAST16 "%d"
#define PRIuFAST16 "%u"
#define PRIiFAST16 "%i"
#define PRIoFAST16 "%o"
#define PRIxFAST16 "%x"
#define PRIXFAST16 "%X"

#define PRIdFAST32 "%d"
#define PRIuFAST32 "%u"
#define PRIiFAST32 "%i"
#define PRIoFAST32 "%o"
#define PRIxFAST32 "%x"
#define PRIXFAST32 "%X"

#define PRIdFAST64 "%lld"
#define PRIuFAST64 "%llu"
#define PRIiFAST64 "%lli"
#define PRIoFAST64 "%llo"
#define PRIxFAST64 "%llx"
#define PRIXFAST64 "%llX"

#endif
