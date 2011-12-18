(*
 * Copyright (c) 2007-2011 The tyndur Project. All rights reserved.
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

unit ircwindow;
{$MODE ObjFPC}
{$STATIC on}
{$H+}

interface

const
    BUFFER_LINES = 30;
    MAX_WINDOWS = 9;

type
    (* Gibt zurück, ob das Fenster noch aktiv ist *)
    TIRCCommandHandler = function(const s: String): boolean;

    TIRCWindow = class
        private
            fTitle: String;
            fChannel: String;
            fCommandHandler: TIRCCommandHandler;
            fJoined: boolean;

            outBuffer: UnicodeString;
            posy: byte;

            inBuffer: array [0..BUFFER_LINES - 1] of record
                s: String;
                color: byte;
            end;

            inBufferPos: integer;

            procedure ScrollLines(num: integer);
            procedure SetTitle(title: String);

            function AutoCompleteOutBuffer(): UnicodeString;

        public
            current: TIRCWindow; static;
            num: integer; static;

            constructor Init(title: String);
            procedure Draw(content: boolean);
            procedure Incoming(s: String; color: byte);
            function CheckInput: boolean;

            class function get(chan: String): TIRCWindow;
            class procedure ChannelIncoming(chan: String; s: String; color: byte);
            class procedure GlobalIncoming(s: String; color: byte);

            property title: String read fTitle write SetTitle;
            property channel: String read fChannel write fChannel;
            property joined: boolean read fJoined write fJoined;
            property commandHandler: TIRCCommandHandler
                read fCommandHandler write fCommandHandler;
    end;

implementation

uses
    sysutils, crt, tyndur;

var
    theIRCWindow: array[1..MAX_WINDOWS] of TIRCWindow;

constructor TIRCWindow.Init(title: String);
var
    i: integer;
begin
    posy := 0;
    fTitle := title;
    fCommandHandler := nil;
    joined := false;

    inBufferPos := 0;
    for i := 0 to BUFFER_LINES - 1 do begin
        inBuffer[i].s := '';
        inBuffer[i].color := 7;
    end;

    if TIRCWindow.num < MAX_WINDOWS then begin
        Inc(TIRCWindow.num);
        theIRCWindow[TIRCWindow.num] := self;
    end;
end;

class function TIRCWindow.get(chan: String): TIRCWindow;
var
    i: integer;
begin
    for i := 1 to TIRCWindow.num do begin
        if theIRCWindow[i].channel = chan then begin
            exit(theIRCWindow[i]);
        end;
    end;

    exit(nil);
end;

class procedure TIRCWindow.ChannelIncoming(chan: String; s: String; color: byte);
var
    ircwindow: TIRCWindow;
begin
    ircwindow := TIRCWindow.get(chan);
    if ircwindow = nil then begin
        ircwindow := theIRCWindow[1];
    end;

    ircwindow.Incoming(s, color);
end;

class procedure TIRCWindow.GlobalIncoming(s: String; color: byte);
var
    i: integer;
begin
    for i := 1 to TIRCWindow.num do begin
        theIRCWindow[i].Incoming(s, color);
    end;
end;

procedure TIRCWindow.Draw(content: boolean);
var
    firstLine, lastLine: integer;
    i, curY: integer;
begin
    if content then begin
        TextBackground(0);
        ClrScr;
    end;

    GotoXY(1,1);
    TextColor(14);
    TextBackground(1);
    Write('kIRC: ' + fTitle : 80);

    if content then begin

        if inBufferPos = 0 then begin
            firstLine := BUFFER_LINES - 1;
        end else begin
            firstLine := inBufferPos - 1;
        end;

        lastLine := inBufferPos;
        curY := 0;
        while curY < 22 do begin
            Inc(curY, (length(inBuffer[firstLine].s) + 79) div 80);
            Dec(firstLine);

            if (firstLine < 0) then begin
                firstLine := BUFFER_LINES - 1;
            end;

            if inBuffer[firstLine].s = '' then begin
                break;
            end;
        end;
        firstLine := (firstLine + 1) mod BUFFER_LINES;

        GotoXY(1,1);
        Write(firstLine, ' - ', lastLine);

        GotoXY(1,2);
        TextBackground(0);

        TextColor(inBuffer[firstLine].color);
        Write(Copy(inBuffer[firstLine].s, 80 * (curY - 22), length(inBuffer[firstLine].s)));
        curY := 2 + (length(inBuffer[firstLine].s) + 79) div 80;

        i := (firstLine + 1) mod BUFFER_LINES;
        while i <> lastLine do begin
            GotoXY(1, curY);
            //if inBuffer[i].s <> '' then begin
                TextColor(inBuffer[i].color);
                Write(inBuffer[i].s);
                Inc(curY, (length(inBuffer[i].s) + 79) div 80);
            //end;
            i := (i + 1) mod BUFFER_LINES;
        end;

    end;

    TextBackground(1);
    GotoXY(1, 24);
    Write(StringOfChar(' ' , 80));
    GotoXY(1, 24);
    TextColor(7);
    Write(Utf8Encode(outBuffer));
    GotoXY(1, 24);
end;

procedure TIRCWindow.setTitle(title: String);
begin
    fTitle := title;
    Draw(false);
end;

procedure TIRCWindow.ScrollLines(num: integer);
var
    i: integer;
begin
    TextBackground(0);
    GotoXY(1, 24);
    for i := 1 to num do begin
        Write(StringOfChar(' ' , 80));
    end;
    WriteLn;
    Draw(false);
end;

procedure TIRCWindow.Incoming(s: String; color: byte);
var
    lines: integer;
begin
    lines := (length(s) + 79) div 80;
    if lines = 0 then begin
        lines := 1;
    end;

    inBuffer[inBufferPos].s := s;
    inBuffer[inBufferPos].color := color;

    inBufferPos := (inBufferPos + 1) mod BUFFER_LINES;
    inBuffer[inBufferPos].s := '';

    if self <> TIRCWindow.current then begin
        if posy + lines > 22 then begin
            Dec(posy, posy + lines - 22);
        end;
        Inc(posy, lines);
        exit;
    end;

    (*
    Draw(true);
    *)

    GotoXY(1, 2 + posy);
    if posy + lines > 22 then begin
        ScrollLines(posy + lines - 22);
        Dec(posy, posy + lines - 22);
        GotoXY(1, 2 + posy);
    end;

    TextBackground(0);
    TextColor(color);
    Write(s);
    Inc(posy, lines);
    GotoXY(length(outBuffer) + 1, 24);
end;

procedure HandleEscapeSequence;
var
    i: integer;
begin
    i := 0;

    case ReadEscapeSequence() of
        SK_F1:  i := 1;
        SK_F2:  i := 2;
        SK_F3:  i := 3;
        SK_F4:  i := 4;
        SK_F5:  i := 5;
        SK_F6:  i := 6;
        SK_F7:  i := 7;
        SK_F8:  i := 8;
        SK_F9:  i := 9;
    end;

    if (i < 1) or (i > TIRCWindow.num) then begin
        exit;
    end;

    TIRCWindow.current := theIRCWindow[i];
end;

function TIRCWindow.CheckInput: boolean;
var
    c: UnicodeChar;
    autoCompletion, curToken: UnicodeString;
    cursorpos : integer;
begin
    CheckInput := true;

    if not keypressed then begin
        exit;
    end;

    TextColor(7);
    TextBackground(1);

    c := ReadUnicodeChar;
    case c of
        #8:     if Length(outBuffer) > 0 then begin
                    if length(outBuffer) < 80 then begin
                        GotoXY(length(outBuffer), 24);
                        Write(' ');
                        SetLength(outBuffer, Length(outBuffer) - 1);
                        GotoXY(length(outBuffer) + 1, 24);
                    end else begin
                        SetLength(outBuffer, Length(outBuffer) - 1);
                        GotoXY(1, 24);
                        Write(Utf8Encode(Copy(outBuffer, length(outBuffer) - 78, 79)));
                    end;
                end;
        #9:     begin
                    autoCompletion := AutoCompleteOutBuffer();
                    if autoCompletion <> '' then begin
                        outBuffer := outBuffer + autoCompletion  + ' ';
                        GotoXY(1, 24);
                        Write(Utf8Encode(Copy(outBuffer, length(outBuffer) - 78, 79)));
                    end;
                end;
        #10:    ;
        #27:    begin
                    HandleEscapeSequence();
                    CheckInput := false;
                end;
        else    begin
                    cursorPos := length(outBuffer) + 1;
                    outBuffer := outBuffer + c;
                    while not (nextkey() in [#0 .. #31]) do begin
                        outBuffer := outBuffer + readkey;
                    end;

                    if length(outBuffer) < 80 then begin
                        GotoXY(cursorPos, 24);
                        Write(Utf8Encode(Copy(outBuffer, cursorPos, length(outBuffer) - cursorPos + 1)));
                    end else begin
                        GotoXY(1, 24);
                        Write(Utf8Encode(Copy(outBuffer, length(outBuffer) - 78, 79)));
                    end;
                end;
    end;

    if c = #10 then begin
        if fCommandHandler <> nil then begin
            if not fCommandHandler(Utf8Encode(outBuffer)) then begin
                CheckInput := false;
            end;

            TextColor(7);
            TextBackground(1);
            GotoXY(1, 24);
            Write(StringOfChar(' ', 80));
            GotoXY(1, 24);
            outBuffer := '';
        end;
    end;

    GotoXY(1, 25);
    TextBackground(0);
    TextColor(8);

    curToken := Copy(outBuffer, LastDelimiter(' ', outBuffer) + 1, length(outBuffer));
    if length(curToken) > 2 then begin
        autoCompletion := AutoCompleteOutBuffer;
        if autoCompletion <> '' then begin
            autoCompletion := curToken + autoCompletion;
        end else begin
            autoCompletion := '';
        end;
    end else begin
         autoCompletion := '';
    end;
    Write(Utf8Encode(Copy(autoCompletion, 1, 79)));
    Write(StringOfChar(' ', 79 - length(autoCompletion)));

    TextBackground(1);
    TextColor(7);
    GotoXY(length(outBuffer) + 1, 24);
end;


function TIRCWindow.AutoCompleteOutBuffer(): UnicodeString;
var
    completionToken, curToken: UnicodeString;
    spacePos: integer;
    curInBufferLine: integer;
    s: UnicodeString;

    i: integer;
begin
    spacePos := LastDelimiter(' ', outBuffer);
    completionToken := Copy(outBuffer, spacePos + 1, length(outBuffer));

    curInBufferLine := inBufferPos;
    AutoCompleteOutBuffer := '';
    repeat
        curToken := '';
        for i := 1 to length(inBuffer[curInBufferLine].s) do begin
            s := Utf8Decode(inBuffer[curInBufferLine].s);
            if s = '' then begin
                continue;
            end;
            if s[i] in ['A' .. 'Z', 'a' .. 'z', '0' .. '9'] then begin
                curToken := curToken + s[i];
            end else if Ord(s[i]) > 128 then begin
                // Nicht-ASCII-Zeichen sind wahrscheinlich auch Buchstaben
                curToken := curToken + s[i];
            end else begin
                // TODO CompareText benutzen
                if Copy(curToken, 1, length(completionToken)) = completionToken then begin
                    // TODO: Bei mehreren Treffern gemeinsamen Anfang suchen
                    AutoCompleteOutBuffer := Copy(curToken, length(completionToken) + 1, length(curToken));
                end;
                curToken := '';
            end;
        end;

        // TODO CompareText benutzen
        if Copy(curToken, 1, length(completionToken)) = completionToken then begin
            // TODO: Bei mehreren Treffern gemeinsamen Anfang suchen
            AutoCompleteOutBuffer := Copy(curToken, length(completionToken) + 1, length(curToken));
        end;

        curInBufferLine := (curInBufferLine + 1) mod BUFFER_LINES;
    until curInBufferLine = inBufferPos;
end;

begin
    TIRCWindow.num := 0;
    TIRCWindow.current := nil;
end.
