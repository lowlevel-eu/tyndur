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

program KEdit;
{$H+}

uses
    dos, crt, sysutils, tyndur, multilang,
    kedit_tui, kedit_buf, kedit_input, kedit_main;

type
    textfile  = text;

var
    state:  kedit_state;

(***********************************************
 * Dummyfunktionen für tyndur                  *
 ***********************************************)

procedure DisableFlushing(var f: Textfile); external name 'disable_flushing';

function Unspace(stext: string):string;
begin
    if Length(stext) = 0 then exit(stext);

    repeat
        if stext[1] = ' ' then begin
            Delete(stext, 1, 1);
        end;
    until  (Length(stext) = 0) or (stext[1] <> ' ');

    repeat
        if stext[Length(stext)] = ' ' then begin
            Delete(stext, Length(stext), 1);
        end;
    until (Length(stext) = 0) or (stext[Length(stext)] <> ' ');

    Unspace := stext;
end;

function HasLeadingSpaces(s: String; num: integer): boolean;
var
    i: integer;
begin
    if s = '' then begin
        exit(num = 0);
    end;

    for i := 1 to num do begin
        if s[i] <> ' ' then begin
            exit(false);
        end;
    end;
    exit(true);
end;

(***********************************************
 * Das Hauptprogrammm                          *
 ***********************************************)
procedure Init;
var
    i: longint;
begin
    with state do begin
        filename    := '(unbenannt)';
        extension   := '';

        highlight   := false;
        readonly    := false;
        replace     := false;
        changed     := false;

        scrollx     := 0;
        scrolly     := 0;
        cursX       := 1;
        cursY       := 1;

        for i := 1 to MaxLines do begin
            text^[i] := '';
        end;
    end;

    TextColor(7);
    TextBackground(0);
    ClrScr;
end;


procedure Edit;

    var
        autoindent: boolean;
        old_autoindent: boolean;

    procedure DoInput;
    var
        key: UnicodeChar;
    begin
        // old_autoindent ist nur gesetzt, wenn schon der letzte Tastendruck
        // ein Enter war.
        old_autoindent := autoindent;
        autoindent := false;

        with state do begin
            key := ReadUnicodeChar;
            case key of
                #27: { VT100-Escapesequenz }
                    begin
                        key := ReadEscapeSequence();
                        if (key >= SK_MIN) and (key <= SK_MAX) and (special_key_handler[key] <> nil) then begin
                            special_key_handler[key](@state);
                        end else begin
                            Beep;
                        end;
                    end;

                #8: { Backspace }
                    if readonly then begin
                        Beep;
                    end else begin
                        if cursX > 1 then begin
                            if text^[cursY] = '' then begin
                                DeleteLine(@state, redrawEditor);
                            end else if HasLeadingSpaces(text^[cursY], cursX - 1) then begin
                                Delete(text^[cursY], 1 + ((cursX - 2) and not 7), 1 + ((cursX - 2) and 7));
                                Dec(cursX, 1 + ((cursX - 2) and 7));
                                changed:=true;
                            end else begin
                                Delete(text^[cursY], cursX - 1, 1);
                                Dec(cursX);
                                changed:=true;
                            end
                        end else if cursY>1 then begin
                            cursX := Length(text^[cursY-1])+1;
                            text^[cursY-1] := text^[cursY-1]+text^[cursY];
                            DeleteLine(@state, redrawEditor);
                            Dec(cursY);
                        end else begin
                            Beep;
                        end;
                    end;

                #9: { Tab - auf durch 8 teilbare X-Position aufrunden }
                    if readonly then begin
                        Beep;
                    end else begin
                        if cursX <= MaxColumns - 8 then begin
                            Insert(Space(8 - ((cursX - 1) mod 8)), text^[cursY], cursX);
                            cursX := 1 + ((cursX + 8) and not 7);
                        end;
                    end;

                #10, #13: { Enter }
                    if readonly then begin
                        Beep;
                    end else if cursY = MaxLines then begin
                        Beep;
                    end else begin
                        InsertLine(@state, redrawEditor);
                        Inc(cursY);
                        cursX := 1;

                        if length(text^[cursY-1]) > 0 then begin
                            autoindent := true;
                            while (cursX <= MaxColumns) and (text^[cursY-1, cursX] = ' ') do begin
                                text^[cursY] := ' ' + text^[cursY];
                                inc(cursX);
                            end;

                            if old_autoindent then begin
                                if text^[cursY-1] = Space(length(text^[cursY-1])) then begin
                                    text^[cursY-1] := '';
                                end;
                            end;

                        end;

                        changed:=true;
                    end;

                else { Alles andere - Buchstaben und so }
                    if readonly then begin
                        Beep;
                    end else if ord(key) < 32 then begin
                        Beep;
                    end else if replace then begin
                        if Length(text^[cursY]) < cursX then begin
                            text^[cursY] := text^[cursY] +
                                Space(cursX - Length(text^[cursY]) - 1);
                            SetLength(text^[cursY], cursX);
                        end;
                        text^[cursY][cursX] := key;


                        if cursX < MaxColumns then begin
                            Inc(cursX);
                        end;

                        changed:=true;
                    end else if (cursX > MaxColumns) then begin
                        Beep;
                    end else begin
                        if Length(text^[cursY])<cursX then begin
                            text^[cursY] := text^[cursY] +
                                Space(cursX - Length(text^[cursY]) - 1);
                        end;

                        Insert(key,text^[cursY],cursX);
                        Inc(cursX);
                        changed:=true;
                    end;
            end {case};
        end {with};
    end {procedure};

begin
    exitEditor := false;
    redrawEditor := true;
    autoindent := false;
    repeat
        if redrawEditor then begin
            DrawEditor(@state);
            Flush(output);
            redrawEditor := false;
        end else begin
            RedrawLine(@state, state.cursY);
        end;

        DoInput;
        DoScrolling(@state, redrawEditor);
    until exitEditor;
end;

procedure SetLang;
var
    name: String;
    lang: TLanguage;
begin
    name := GetEnv('LANG');
    if name <> '' then begin
        lang := GetLanguage(name);
        if lang <> nil then begin
            SetLanguage(lang);
        end;
    end;
end;

var
    i: integer;
begin
    DisableFlushing(output);
    SetLang;

    New(state.text);
    for i:=1 to ParamCount do begin
        //WriteLn('Parameter ', i, ': ', ParamStr(i));
        if ParamStr(i) = '' then begin
            continue;
        end;
        Flush(output);
        Init;
        LoadFile(@state, ParamStr(i), true);
        Edit;
    end;

    if ParamCount=0 then begin
        //WriteLn('Keine Parameter');
        Flush(output);
        Init;
        Edit;
    end;
    Dispose(state.text);
end.
