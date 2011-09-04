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

unit kedit_tui;
{$MODE ObjFPC}
{$H+}

interface
uses kedit_main;

const
    Version = '0.5.0';

    TermLines = 25;
    TextLines = 23;
    TextCols  = 80;

    OPT_YES     = 1;
    OPT_NO      = 2;
    OPT_CANCEL  = 3;

procedure ShowCursor(visible: boolean);
procedure FillRectangle(x1, y1, x2, y2: integer);
function Space(n: integer): string;
procedure DrawTextBox(x,y,w: integer; text: string; active: boolean);
procedure DrawDblFrame(x,y,xx,yy: integer);

function YesNoCancel(question: string; cancel: boolean): integer;

procedure RedrawLine(state: pkedit_state; y: longint);
procedure DrawTitleBar(state: pkedit_state);
procedure DrawEditor(state: pkedit_state);
procedure SetSyntaxHighlighting(state: pkedit_state);

implementation

uses
    crt, strutils, sysutils,
    syntax, syntax_c, syntax_pas, syntax_intel, syntax_atandt, syntax_perl, syntax_python;

resourcestring
    rsMenuBar = 'F2: Speichern, F3: Laden, F8: Syntax, F10: Beenden, EINFG: Einfügen/Ersetzen';
    rsMenuBarReadOnly = 'F2: R/W laden, F3: Laden, F8: Syntax, F10: Beenden, EINFG: Einfügen/Ersetzen';

var
    highlighter: TSyntax;

procedure ShowCursor(visible: boolean);
begin
    if visible then CursorOn else CursorOff;
end;

(* Rein grafisches Zeug ***********************************************)

procedure FillRectangle(x1, y1, x2, y2: integer);
var
    i, j: integer;
begin
    for i := x1 to x2 do begin
        for j := y1 to y2 do begin
            GotoXY(i,j);
            Write(' ');
        end;
    end;
end;

function Space(n: integer): string;
begin
    Space := StringOfChar(' ', n);
end;

procedure DrawTextBox(x,y,w: integer; text: string; active: boolean);
begin
    TextBackground(6);
    if active
        then TextColor(15)
        else TextColor(7);

    GotoXY(x,y);
    Write(text + Space(w-Length(text)) : w);
end;

procedure DrawDblFrame(x,y,xx,yy: integer);
var
    i: integer;
begin
    GotoXY(x,y);
    Write('+'); for i:=x+1 to xx-1 do Write('-');  Write('+');

    GotoXY(x,yy);
    Write('+'); for i:=x+1 to xx-1 do Write('-');  Write('+');

    for i:=y+1 to yy-1 do begin GotoXY(x,i); Write('|'); end;
    for i:=y+1 to yy-1 do begin GotoXY(xx,i); Write('|'); end;
end;


(* Grafisches Eingabezeug *********************************************)

function YesNoCancel(question: string; cancel: boolean): integer;
begin
    TextBackground(1);
    GotoXY(1,1); Write(space(79));
    GotoXY(1,1); Write(question);

    if cancel then begin
        Write(' [j/n/esc]');
    end else begin
        Write(' [j/n]');
    end;

    repeat
        case readkey of
            'j', 'J':
                begin
                    YesNoCancel := OPT_YES;
                    break;
                end;

            'n', 'N':
                begin
                    YesNoCancel := OPT_NO;
                    break;
                end;

            #27:
                if cancel then begin
                    YesNoCancel := OPT_CANCEL;
                    break;
                end;
        end;
    until false;
end;


(* Grafisches Zeug mit Bezug auf den Editor ***************************)

procedure DrawTitleBar(state: pkedit_state);
begin
    GotoXY(1,1);
    Textbackground(1);

    TextColor(14);
    Write('Editor: ');

    TextColor(7);
    Write(state^.filename + #27'[K');

    GotoXY(55,1);
    Write(state^.cursX, ':', state^.cursY);

    GotoXY(68,1);
    if state^.readonly then begin
        TextColor(12);
        Write('- READONLY! -');
    end else if state^.replace then begin
        Write('-- REPLACE --')
    end else begin
        Write('-- INSERT -- ');
    end;

    TextBackground(0);
    TextColor(7);
end;

procedure RedrawLine(state: pkedit_state; y: longint);
var
    screenY: byte;
begin
    if highlight then begin
        DrawEditor(state);
        exit;
    end;

    // Titelzeile aktualisieren (Zeile/Spalte)
    DrawTitleBar(state);

    with state^ do begin
        // Die erste Titelzeile gehört dem Editor, nicht dem Text
        screenY := 1 + y - scrolly;

        GotoXY(1, screenY);
        Write(Utf8Encode(Copy(text^[y], 1+scrollx, 79)));
        Write(Space(79 - Length(Copy(text^[y], 1+scrollx, 79))));
        GotoXY(CursX - ScrollX, 1 + CursY - ScrollY);
    end;
end;

procedure DrawEditor(state: pkedit_state);
var
    i, j: longint;
    firstVisible: integer;
    lastVisible: integer;
    firstHighlighted: integer;
    syntax_state: longint;
    syntax_chg: TSyntaxChange;
    fromCol, toCol: integer;
    s: UnicodeString;
begin
    // Titelleiste
    DrawTitleBar(state);

    // Der eigentliche Text
    GotoXY(1,2);
    firstVisible := 1 + state^.scrolly;
    lastVisible := TextLines + state^.scrolly;
    if (not highlight) or (highlighter = nil) then begin
        for i := firstVisible to lastVisible do begin
            s := Copy(state^.text^[i], 1 + state^.scrollx, 80);
            Write(Utf8Encode(s));
            Write(Space(80-Length(Copy(state^.text^[i],1+state^.scrollx,80))));
        end;
    end else begin
        // 20 Zeilen Kontext werden benutzt, um am Anfang wahrscheinlich den
        // richtigen Zustand zu treffen
        firstHighlighted := firstVisible - 20;
        if firstHighlighted < 1 then begin
            firstHighlighted := 1;
        end;

        syntax_chg.color := syn_other;
        syntax_state := 0;
        for i := firstHighlighted to firstVisible - 1 do begin
            highlighter.StartLine(state^.text^[i], syntax_state);
            repeat
                syntax_chg := highlighter.Next;
            until syntax_chg.posY = 0;
            syntax_state := highlighter.GetState;
        end;

        // Und jetzt den Text selbst bunt ausgeben
        for i := firstVisible to lastVisible do begin
            highlighter.StartLine(state^.text^[i], syntax_state);
            fromCol := state^.scrollx + 1;
            repeat
                syntax_chg := highlighter.Next;
                TextColor(SyntaxColor(syntax_chg.color));
                TextBackground(SyntaxBgColor(syntax_chg.color));

                if syntax_chg.posY = 0 then begin
                    toCol := length(state^.text^[i]);
                end else begin
                    toCol := syntax_chg.posY - 1;
                end;
                if toCol > state^.scrollx + 80 then begin
                    toCol := state^.scrollx + 80;
                end;

                if fromCol <= toCol then begin
                    s := Copy(state^.text^[i], fromCol, toCol - fromCol + 1);
                    Write(Utf8Encode(s));
                end;

                if syntax_chg.posY > fromCol then begin
                    fromCol := syntax_chg.posY;
                end;
            until syntax_chg.posY = 0;
            syntax_state := highlighter.GetState;

            TextBackground(0);
            if toCol < state^.scrollx + 80 then begin
                if toCol < state^.scrollx then begin
                    WriteLn(#27'[K');
                end else begin
                    Write(Space(state^.scrollx + 80 - toCol));
                end;
            end;
        end;
    end;

    // Untere Leiste
    GotoXY(1,25);
    TextBackground(1);
    TextColor(7);
    if state^.readonly then begin
        Write(rsMenuBarReadOnly, #27'[K');
    end else begin
        Write(rsMenuBar, #27'[K');
    end;

    // Cursor richtig in den Text setzen
    GotoXY(state^.CursX - state^.ScrollX, 1 + state^.CursY - state^.ScrollY);
    TextBackground(0);
    TextColor(7);
    flush(output);
end;

procedure SetSyntaxHighlighting(state: pkedit_state);
begin
    if state^.extension = 'c' then begin
        highlighter := TSyntax_C.create;
    end else if state^.extension = 'pas' then begin
        highlighter := TSyntax_Pas.create;
    end else if state^.extension = 'asm' then begin
        highlighter := TSyntax_INTEL.create;
    end else if state^.extension = 's' then begin
        highlighter := TSyntax_ATANDT.create;
    end else if state^.extension = 'pl' then begin
        highlighter := TSyntax_Perl.create;
    end else if state^.extension = 'pm' then begin
        highlighter := TSyntax_Perl.create;
    end else if state^.extension = 'py' then begin
        highlighter := TSyntax_Python.create;
    end;
end;

begin
    highlighter := nil;
end.
