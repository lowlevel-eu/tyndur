(*
 * Copyright (c) 2008-2011 The tyndur Project. All rights reserved.
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
 *)

unit kedit_buf;

interface

uses kedit_main;

procedure DoScrolling(state: pkedit_state; var redrawEditor: boolean);
procedure DeleteLine(state: pkedit_state; var redrawEditor: boolean);
procedure InsertLine(state: pkedit_state; var redrawEditor: boolean);


implementation

uses sysutils, kedit_tui;

procedure DoScrolling(state: pkedit_state; var redrawEditor: boolean);
begin
    with state^ do begin
        if cursX <= scrollX then begin
            scrollX := cursX - 1;
            redrawEditor := true;
        end;

        if cursX > scrollX + TextCols then begin
            scrollX := cursX - TextCols;
            redrawEditor := true;
        end;

        if cursY <= scrollY then begin
            scrollY := cursY - 1;
            redrawEditor := true;
        end;

        if cursY > scrollY + TextLines then begin
            scrollY := cursY - TextLines;
            redrawEditor := true;
        end;
    end;
end;

procedure DeleteLine(state: pkedit_state; var redrawEditor: boolean);
var
    i: longint;
begin
    with state^ do begin
        for i := cursY to MaxLines-1 do begin
            text^[i] := text^[i+1];
        end;
        text^[MaxLines] := '';

        redrawEditor := true;
    end;
end;

procedure InsertLine(state: pkedit_state; var redrawEditor: boolean);
var
    i: longint;
begin
    with state^ do begin
        if cursy = MaxLines then begin
            Beep;
            exit;
        end;

        for i := MaxLines downto cursy+1 do begin
            text^[i] := text^[i-1];
        end;

        text^[cursY + 1] := Copy(text^[cursy], cursx, 255);
        text^[cursY]     := Copy(text^[cursy], 1, cursx-1);

        redrawEditor := true;
    end;
end;


end.
