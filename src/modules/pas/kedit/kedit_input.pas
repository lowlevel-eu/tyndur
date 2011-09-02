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

unit kedit_input;

interface

uses tyndur, kedit_main, kedit_tui;


type
    TSKHandler = procedure(state: pkedit_state);

var
    special_key_handler: array [SK_MIN..SK_MAX] of TSKHandler;



implementation

uses sysutils, crt, kedit_buf;

resourcestring
    rsFileNotSaved = 'Die Datei ist noch nicht gespeichert. Speichern?';


procedure uih_save_file(state: pkedit_state);
begin
    if state^.readonly then begin
        RemoveRO(state);
        DrawEditor(state);
    end else begin
        SaveFile(state);
        DrawEditor(state);
    end;
end;

procedure uih_load_file(state: pkedit_state);
begin
    if changed then case YesNoCancel(rsFileNotSaved, true) of
        OPT_YES:    SaveFile(state);
        OPT_NO:     changed := false;
        OPT_CANCEL: {do nothing};
    end;

    if not changed then begin
        LoadFile(state);
        DrawEditor(state);
    end;
end;

procedure uih_toggle_syntax(state: pkedit_state);
begin
    with state^ do begin
        highlight := not highlight;
        redrawEditor := true;
    end;
end;

procedure uih_quit(state: pkedit_state);
begin
    if changed then case YesNoCancel(rsFileNotSaved, true) of
        OPT_YES:    SaveFile(state);
        OPT_NO:     changed := false;
        OPT_CANCEL: {do nothing};
    end;

    if not changed then begin
        TextBackground(0);
        ClrScr;
        exitEditor := true;
    end;
end;

procedure uih_toggle_replace(state: pkedit_state);
begin
    with state^ do begin
        replace := not replace;
        redrawEditor := true;
    end;
end;

procedure uih_del_char(state: pkedit_state);
begin
    with state^ do begin
        if readonly then begin
            Beep;
        end else begin
            if length(text^[cursY]) > 0 then begin
                if cursX <= Length(text^[cursY]) then begin
                    Delete(text^[cursY],cursX,1);
                    changed:=true;
                end else if cursY<MaxLines then begin
                    Inc(cursy);
                    text^[cursy-1]:=text^[cursy-1]+text^[cursy];
                    DeleteLine(state, redrawEditor);
                    Dec(cursy);
                    redrawEditor := true;
                end else begin
                    Beep;
                end;
            end else begin
                DeleteLine(state, redrawEditor);
            end;
        end;
    end;
end;

procedure uih_del_line(state: pkedit_state);
begin
    with state^ do begin
        if readonly then begin
            Beep;
        end else begin
            DeleteLine(state, redrawEditor);
            cursX := 1;
            changed := true;
        end;
    end;
end;

procedure uih_cursor_left(state: pkedit_state);
begin
    with state^ do begin
        if cursX > 1 then begin
            Dec(cursX);
        end;
    end;
end;

procedure uih_cursor_right(state: pkedit_state);
begin
    with state^ do begin
        if cursX < 255 then begin
            Inc(cursX);
        end;
    end;
end;

procedure uih_cursor_up(state: pkedit_state);
begin
    with state^ do begin
        if cursY > 1 then begin
            Dec(cursY);
        end;
    end;
end;

procedure uih_cursor_down(state: pkedit_state);
begin
    with state^ do begin
        if cursY<MaxLines then begin
            Inc(cursY);
        end;
    end;
end;

procedure uih_cursor_page_up(state: pkedit_state);
begin
    with state^ do begin
        if cursY > 10 then begin
            Dec(cursY,10);
        end else begin
            cursy := 1;
        end;
    end;
end;

procedure uih_cursor_page_down(state: pkedit_state);
begin
    with state^ do begin
        if cursY < MaxLines - 10 then begin
            Inc(cursY,10);
        end else begin
            cursy := MaxLines;
        end;
    end;
end;

procedure uih_cursor_to_start_of_line(state: pkedit_state);
begin
    with state^ do begin
        if cursX = 1 then begin
            // Wenn der Cursor schon auf der ersten Spalte ist, zum ersten
            // Nichtleerzeichen auf der Zeile springen
            while (length(text^[cursY]) > cursX)
                and (text^[cursY][cursX] = ' ')
            do begin
                Inc(cursX)
            end;
        end else begin
            // Ansonsten Cursor in die erste Spalte setzen
            cursX := 1;
        end;
    end;
end;

procedure uih_cursor_to_end_of_line(state: pkedit_state);
var
    text_end: integer;
begin
    with state^ do begin
        text_end := Length(RTrim(text^[cursY]));
        if cursX = text_end + 1 then begin
            // Wenn der Cursor schon hinter dem letzten Zeichen des Texts (ohne
            // abschliessende Leerzeichen) steht, ganz ans Ende der Zeile
            // springen
            cursX := Length(text^[cursY]);
        end else begin
            // Ansonsten Cursor ans Ende des Texts setzen
            cursX := text_end;
        end;
        if cursX < 255 then Inc(cursX);
    end;
end;

procedure uih_cursor_large_page_up(state: pkedit_state);
begin
    with state^ do begin
        if cursY > 50 then begin
            Dec(cursY,50);
        end else begin
            cursy := 1;
        end;
    end;
end;

procedure uih_cursor_large_page_down(state: pkedit_state);
begin
    with state^ do begin
        if cursY < MaxLines - 50 then begin
            Inc(cursY,50);
        end else begin
            cursy := MaxLines;
        end;
    end;
end;

var
    c: char;
begin
    for c := SK_MIN to SK_MAX do begin
        special_key_handler[c] := nil;
    end;

    special_key_handler[SK_F2]          := @uih_save_file;
    special_key_handler[SK_F3]          := @uih_load_file;
    special_key_handler[SK_F8]          := @uih_toggle_syntax;
    special_key_handler[SK_F10]         := @uih_quit;
    special_key_handler[SK_F12]         := @uih_del_line;
    special_key_handler[SK_INS]         := @uih_toggle_replace;
    special_key_handler[SK_DEL]         := @uih_del_char;
    special_key_handler[SK_LEFT]        := @uih_cursor_left;
    special_key_handler[SK_RIGHT]       := @uih_cursor_right;
    special_key_handler[SK_UP]          := @uih_cursor_up;
    special_key_handler[SK_DOWN]        := @uih_cursor_down;
    special_key_handler[SK_PGUP]        := @uih_cursor_page_up;
    special_key_handler[SK_PGDN]        := @uih_cursor_page_down;
    special_key_handler[SK_HOME]        := @uih_cursor_to_start_of_line;
    special_key_handler[SK_END]         := @uih_cursor_to_end_of_line;
    special_key_handler[SK_CTRL_PGUP]   := @uih_cursor_large_page_up;
    special_key_handler[SK_CTRL_PGDN]   := @uih_cursor_large_page_down;

    // TODO
    //special_key_handler[SK_CTRL_HOME]   := @uih_cursor_to_start_of_document;
    //special_key_handler[SK_CTRL_END]    := @uih_cursor_to_end_of_document;
end.
