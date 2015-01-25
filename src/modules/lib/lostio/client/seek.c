/*
 * Copyright (c) 2008 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Antoine Kaufmann.
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
#include <io.h>
#include <lostio.h>
#include <stdio.h>
#include <rpc.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static bool newlio_flseek(io_resource_t* io_res, uint64_t offset, int origin)
{
    int whence;

    switch (origin) {
        case SEEK_SET: whence = LIO_SEEK_SET; break;
        case SEEK_CUR: whence = LIO_SEEK_CUR; break;
        case SEEK_END: whence = LIO_SEEK_END; break;
        default: return false;
    }

    return lio_seek(io_res->lio2_stream, offset, whence) >= 0;
}

/**
 * Cursorposition im Dateihandle setzen
 *
 * @param io_res Dateihandle
 * @param offset Offset bezogen auf den mit origin festgelegten Ursprung
 * @param origin Ursprung. Moeglichkeiten:
 *                  - SEEK_SET Bezogen auf Dateianfang
 *                  - SEEK_CUR Bezogen auf die aktuelle Position
 *                  - SEEK_END Bezogen auf das Ende der Datei
 *
 * @return true wenn die Position erfolgreich gesetzt wurde, sonst false
 */
bool lio_compat_seek(io_resource_t* io_res, uint64_t offset, int origin)
{
    io_seek_request_t seek_request;


    if (IS_LIO2(io_res)) {
        return newlio_flseek(io_res, offset, origin);
    }

    seek_request.id = io_res->id;
    seek_request.offset = offset;
    seek_request.origin = origin;

    return (rpc_get_dword(io_res->pid, "IO_SEEK ", sizeof(io_seek_request_t),
        (char*) &seek_request) == 0);
}

/**
 * Aktuelle Cursorposition im Dateihandle abfragen
 *
 * @param io_res Dateihandle
 * @return Aktuelle Position in Bytes vom Dateianfang oder -1 bei Fehler
 */
int64_t lio_compat_tell(io_resource_t* io_res)
{
    io_tell_request_t tell_request;
    long result;

    if (IS_LIO2(io_res)) {
        int64_t pos = lio_seek(io_res->lio2_stream, 0, LIO_SEEK_CUR);
        return (pos < 0 ? -1 : pos);
    }

    tell_request.id = io_res->id;

    //Da fuer qwords keine Funktion existiert um sie direkt uber RPC zu
    // empfangen, muss hier ein kleiner Umweg gemacht werden.
    response_t* resp = rpc_get_response(io_res->pid, "IO_TELL ",
        sizeof(io_tell_request_t), (char*) &(tell_request));
    memcpy(&result, resp->data, sizeof(long));

    //Antwort und Daten wieder freigeben
    free(resp->data);
    free(resp);

    return result;
}
