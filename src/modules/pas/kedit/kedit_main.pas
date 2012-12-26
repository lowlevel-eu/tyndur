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

unit kedit_main;

interface

const
    MaxLines = 32768;
    MaxColumns = 255;

type
    type_text = Array [1..MaxLines] of UnicodeString;

    // Beschreibt einen offenen Puffer
    kedit_state = record
        filename:   String;
        extension:  String;

        cursX:      longint;
        cursY:      longint;
        scrollX:    longint;
        scrollY:    longint;

        replace:    boolean;
        readonly:   boolean;

        text:       ^type_text;
    end;
    pkedit_state = ^kedit_state;

var
    changed:            boolean; // FIXME Gehoert in kedit_state
    highlight:          boolean;
    exitEditor:         boolean;
    redrawEditor:       boolean;

procedure SaveFile(state: pkedit_state);
procedure LoadFile(state: pkedit_state);
procedure LoadFile(state: pkedit_state; loadname: string; create: boolean);
procedure RemoveRO(state: pkedit_state);

function RTrim(stext: string):string;


implementation

uses crt, dos, sysutils, strutils, kedit_tui;


resourcestring
    rsFilename = 'Dateiname';
    rsFileIsReadOnly = 'Die Datei %s existiert und ist schreibgeschützt. [ENTER]';
    rsOverwriteExistingFile = 'Die Datei %s gibt''s schon. Überschreiben?';
    rsFileNotFound = 'Datei %s nicht gefunden.';


function RTrim(stext: string):string;
begin
    repeat
        if stext[Length(stext)] = ' ' then begin
            Delete(stext, Length(stext), 1);
        end;
    until (Length(stext) = 0) or (stext[Length(stext)] <> ' ');
    RTrim := stext;
end;



procedure GuessSyntaxHighlighting(state: pkedit_state; filename: string);
begin
    {
        Dieses wundervolle Code-Konstrukt ist das Ergebnis
        stundenlanger Codearbeit von Patrick Pokatilo a.k.a
        the incredible SHyx0rmz und von Alexander H. Kluth a.k.a.
        'Pagefault-it!'-DerHartmut.
    }
    if RightStr(filename, 4) = '.pas' then begin
        state^.extension := 'pas';
    end else if RightStr(filename, 4) = '.asm' then begin
        state^.extension := 'asm';
    end else if RightStr(filename, 2) = '.c' then begin
        state^.extension := 'c';
    end else if RightStr(filename, 2) = '.h' then begin
        state^.extension := 'c';
    end else if RightStr(filename, 2) = '.S' then begin
        state^.extension := 's';
    end else if RightStr(filename, 2) = '.s' then begin
        state^.extension := 's';
    end else if RightStr(filename, 3) = '.pl' then begin
        state^.extension := 'pl';
    end else if RightStr(filename, 3) = '.pm' then begin
        state^.extension := 'pm';
    end else if RightStr(filename, 3) = '.py' then begin
        state^.extension := 'py';
    end;

    SetSyntaxHighlighting(state);
end;

procedure SaveFile(state: pkedit_state);
var
    savename: string;
    i, j: longint;
    sfile: textfile;
    emptylines: longint;
    attr: word;
begin
    with state^ do begin
        TextBackground(1);

        // Dateinamen abfragen
        GotoXY(1,1);
        Write(space(79));
        if filename='(unbenannt)' then begin
            GotoXY(1,1);
            Write(rsFilename, ': ');
            ReadLn(savename);
        end else begin
            GotoXY(1,1);
            Write(rsFilename, ' [',filename,']: ');
            ReadLn(savename);
            if savename='' then begin
                savename := filename;
            end;
        end;

        Textbackground(0);

        // Ueberpruefen, ob die Datei schon existiert und ggf.
        // schreibgeschuetzt ist
        if FileExists(savename) then begin
            GetFAttr(sfile, attr);

            Textbackground(1);
            GotoXY(1,1);
            Write(space(79));
            GotoXY(1,1);

            if attr and dos.ReadOnly = dos.Readonly then begin
                Write(Format(rsFileIsReadOnly, [savename]));
                Textbackground(0);
                readkey;
                exit;
            end else begin
                case YesNoCancel(Format(rsOverwriteExistingFile, [savename]), false) of
                    OPT_YES:    {Do nothing};
                    OPT_NO:     exit;
                end;
            end;
        end;

        // Datei schreiben
        filename := savename;
        Assign(sfile, savename);
        Rewrite(sfile);
        emptylines := 0;

        // FIXME Leerzeilen am Dateiende
        for i := 1 to MaxLines do begin
            if text^[i]='' then begin
                Inc(emptylines)
            end else begin
                for j := 1 to emptylines do begin
                    WriteLn(sfile);
                end;
                WriteLn(sfile, Utf8Encode(text^[i]));
                emptylines:=0;
            end;
        end;
        Close(sfile);
    end;

    // Neue Dateiendung, neues Highlighting
    GuessSyntaxHighlighting(state, savename);

    changed := false;
end;

procedure LoadFile(state: pkedit_state; loadname: string; create: boolean);
var lfile: textfile;
    cnt: longint;
    i: integer;
    attr: word;
    s: String;
    ext: String;
    reversed: String;
begin
    if loadname = '' then begin
        Textbackground(1);
        gotoxy(1,1); Write(space(79));
        gotoxy(1,1); Write(rsFilename, ': '); ReadLn(loadname);
        Textbackground(0);
    end;

    if loadname = '' then begin
        exit;
    end;

    loadname := Trim(loadname);
    GuessSyntaxHighlighting(state, loadname);

    if not FileExists(loadname) then begin
        if create then begin
            state^.filename := loadname;
            exit;
        end else begin
            gotoxy(1,1); Write(space(79));

            gotoxy(1,1);
            Textbackground(4);
            Write(Format(rsFileNotFound, [loadname]));
            Textbackground(0);
            readkey;

            gotoxy(1,1); Write(space(79));
            exit;
        end;
    end;

    with state^ do begin
        Assign(lfile,loadname);
        GetFAttr(lfile, attr);
        if attr and dos.ReadOnly = dos.ReadOnly then readonly := true;
        Reset(lfile);

        for i := 1 to MaxLines do begin
            text^[i] := '';
        end;

        cnt := 0;
        while not(eof(lfile)) do begin
            Inc(cnt);
            ReadLn(lfile, s);

            // FIXME Sobald kedit Tabs kann, duefern die hier nicht mehr zu
            // Leerzeichen werden
            text^[cnt] := Utf8Decode(AnsiReplaceStr(s, #9, StringOfChar(' ', 8)));
        end;

        Close(lfile);
        filename := loadname;
    end;

    gotoxy(1,1);
    Write(space(79));
end;

procedure LoadFile(state: pkedit_state);
begin
    LoadFile(state, '', false);
end;

procedure RemoveRO(state: pkedit_state);
begin
    ShowCursor(false);
    FillRectangle(19,9,61,15);
    DrawDblFrame(19,9,61,15);
    GotoXY(21,10); TextColor(15); Write('Schreibschutz entfernen'); TextColor(7);
    GotoXY(21,11); Write('Wollen Sie den Schreibschutz wirklich');
    GotoXY(21,12); Write('aufheben? Die Datei muss dann unter');
    GotoXY(21,13); Write('einem anderen Namen gespeichert werden.');
    GotoXY(38,14); Write('[J/N]');

    repeat
        case readkey of
            'n','N':
                break;

            'j','J':
                begin
                    state^.filename := '(unbenannt)';
                    state^.readonly := false;
                    break;
                end;
        end;
    until false;

    ShowCursor(true);
    redrawEditor := true;
end;

end.
