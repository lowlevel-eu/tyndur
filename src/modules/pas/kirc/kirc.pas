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

program kirc;
{$MODE ObjFPC}
{$H+}

uses sysutils, crt, ircwindow;

var
    conn: longint;
    nick: string;
    away: boolean;

procedure WriteConn(conn: longint; s: ShortString);
begin
    FileWrite(conn, s[1], length(s));

    // TODO Entfernen, sobald tpcip auch ohne auskommt
    Delay(100);
end;

procedure ReadConn(var conn: longint; var s: ShortString);
var
    bytes: longint;
begin
    bytes := FileRead(conn, s[1], 255);
    if (bytes > 0) and (bytes < 256) then begin
        s[0] := char(bytes);
    end else begin
        s := '';
    end;

    (*if bytes > 0 then begin
        TextColor(11);
        WriteLn(s);
        TextColor(7);
    end;*)
end;

function NickIsValid(nick: string): boolean;
var
    i: integer;
begin
    if nick = '' then begin
        exit(false);
    end;

    for i := 1 to Length(nick) do begin
        if nick[i] in [' '] then begin
            exit(false);
        end;
    end;
    NickIsValid := true;
end;

procedure OpenConnection(var conn: longint);
var
    server: String;
    port: String;
begin
    Write('Server [irc.euirc.net]: ');
    ReadLn(server);
    if server = '' then begin
        server := 'irc.euirc.net';
    end;

    Write('Port [6667]: ');
    ReadLn(port);
    if port = '' then begin
        port := '6667';
    end;

    repeat
        Write('Nickname: ');
        ReadLn(nick);
    until NickIsValid(nick);

    conn := FileOpen('tcpip:/'+server+':'+port, fmOpenReadWrite);

    if conn = -1 then begin
        WriteLn('Konnte TCP/IP-Verbindung nicht herstellen.');
        halt;
    end;

    WriteConn(conn, 'USER kirc 0 0 :kirc/tyndur'#10);
    WriteConn(conn, 'NICK '+nick+#10);
    away := false;
end;

function GetNickFromPrefix(prefix: String): String;
var
    bang: integer;
begin
    if prefix = '' then begin
        exit('');
    end;

    if prefix[1] = ':' then begin
        prefix := Copy(prefix, 2, length(prefix));
    end;

    bang := Pos('!', prefix);
    if bang > 0 then begin
        exit(Copy(prefix, 1, bang - 1));
    end else begin
        exit(prefix);
    end;
end;

function ShiftParameter(var list: String): String;
var
    space: integer;
begin
    if list = '' then begin
        exit('');
    end;

    space := Pos(' ', list);
    if list[1] = ':' then begin
        ShiftParameter := Copy(list, 2, length(list));
        list := '';
    end else if space > 0 then begin
        ShiftParameter := Copy(list, 1, space - 1);
        list := Copy(list, space + 1, length(list));
    end else begin
        ShiftParameter := list;
        list := '';
    end;
end;

procedure EvaluateIRCInput(conn: longint; line: String);
var
    prefix: string;
    command: string;
    parameter: string;
    space: integer;
    msg: string;
    chan: string;
    ircwindow: TIRCWindow;
begin
    if line = '' then begin
        exit;
    end;

    if line[1] = ':' then begin
        space := Pos(' ', line);
        prefix := Copy(line, 1,  space - 1);
        line := Copy(line, space + 1, length(line));
    end;

    space := Pos(' ', line);
    if space > 0 then begin
        command := Copy(line, 1, space - 1);
        if line[space + 1] = ':' then begin
            parameter := Copy(line, space + 2, length(line));
        end else begin
            parameter := Copy(line, space + 1, length(line));
        end;
    end else begin
        command := line;
    end;

    if command = 'PING' then begin
        (*
        TIRCWindow.current.Incoming(line, 8);
        TIRCWindow.current.Incoming('PONG :'+parameter+#10, 8);
        *)
        WriteConn(conn, 'PONG :'+parameter+#10);
    end
    else if command = 'JOIN' then begin
        chan := ShiftParameter(parameter);
        TIRCWindow.ChannelIncoming(chan, prefix + ' hat den Kanal ' + chan + ' betreten', 2);
    end
    else if command = 'PART' then begin
        chan := ShiftParameter(parameter);
        TIRCWindow.ChannelIncoming(chan, GetNickFromPrefix(prefix)
            + ' hat den Kanal verlassen ('
            + parameter +')', 2);
    end
    else if command = 'QUIT' then begin
        // TODO Auf Channels einschränken, wo der Nick auch wirklich ist
        TIRCWindow.GlobalIncoming(GetNickFromPrefix(prefix)
            + ' hat den Server verlassen ('
            + parameter +')', 2);
    end
    else if command = 'NICK' then begin
        // TODO Auf Channels einschränken, wo der Nick auch wirklich ist
        TIRCWindow.GlobalIncoming(GetNickFromPrefix(prefix)
            + ' nennt sich jetzt '
            + ShiftParameter(parameter), 2);
    end
    else if command = 'KICK' then begin
        chan := ShiftParameter(parameter);
        msg := ShiftParameter(parameter);
        TIRCWindow.ChannelIncoming(chan, prefix + ' hat ' + msg
            + ' aus dem Kanal geworfen (' + ShiftParameter(parameter) + ')', 2);

        if msg = nick then begin
            ircwindow := TIRCWindow.get(chan);
            ircwindow.title := nick;
            ircwindow.joined := false;
        end;
    end
    else if command = 'MODE' then begin
        chan := ShiftParameter(parameter);
        TIRCWindow.ChannelIncoming(chan, prefix + ' setzt den Modus: '
            + parameter, 9);
    end
    else if command = 'TOPIC' then begin
        chan := ShiftParameter(parameter);
        TIRCWindow.ChannelIncoming(chan, GetNickFromPrefix(prefix)
            + ' setzt das Kanalthema auf: ' + ShiftParameter(parameter), 9);
    end
    else if command = 'PRIVMSG' then begin
        chan := ShiftParameter(parameter);
        msg := ShiftParameter(parameter);

        ircwindow := TIRCWindow.get(chan);
        if ircwindow = nil then begin
            ircWindow := TIRCWindow.current;
        end;

        if Copy(msg, 1, 8) = #1'ACTION ' then begin
            ShiftParameter(msg);
            ircwindow.Incoming(GetNickFromPrefix(prefix) + ' '
                + Copy(msg, 1, length(msg) - 1) + #10, 14);
        end else begin
            ircwindow.Incoming('<' + GetNickFromPrefix(prefix) + '> ' + msg, 7);
        end;
    end
    else if command = 'ERROR' then begin
        TIRCWindow.GlobalIncoming(line, 12);
    end
    else begin
        TIRCWindow.GlobalIncoming(line, 8);
    end;
end;


function EvaluateIRCCommand(const line: String): boolean; forward;
function get_fnptr: TIRCCommandHandler;
begin
    get_fnptr := @EvaluateIRCCommand;
end;

function EvaluateIRCCommand(const line: String): boolean;
var
    rststr: String;
    tmpnick: String;
    tmpmsg: String;
    command: String;
    i: integer;
    ircwindow: TIRCWindow;
    channel: string;
begin
    EvaluateIRCCommand := true;

    // Bei Leerzeilen machen wir gleich mal gar nichts
    // Vor allem keinen Zugriff auf line[1], sonst ist kirc weg
    if line = '' then begin
        exit;
    end;

    channel := TIRCWindow.current.channel;

    // Ein Slash bedeutet meist der Beginn eines Kommandos, also parsen wir das ganze mal
    if line[1] = '/' then begin
        // Eigentliches Kommando extrahieren
        // Wichtig: Darauf achten, ob ein Leerzeichen da ist oder nicht
        if Pos(' ', line) = 0 then begin
            command := Copy(line, 1, length(line));
        end else begin
            command := Copy(line, 1, Pos(' ', line)-1);
        end;

        // Nun den eigentlichen Befehl parsen
        if command = '/join' then begin
            channel := Copy(line, 7, length(line));

            ircwindow := TIRCWindow.get(channel);

            if ircwindow = nil then begin
                ircwindow := TIRCWindow.init(nick);
                ircwindow.commandHandler := get_fnptr();
            end;

            if not ircwindow.joined then begin
                ircwindow.title := nick + ' in ' + channel;
                ircwindow.channel := channel;

                WriteConn(conn, 'JOIN ' + channel + #10);
            end;

            TIRCWindow.current := ircwindow;
            TIRCWindow.current.joined := true;
            EvaluateIRCCommand := false;

        end else if command = '/connect' then begin
            //TODO
        end else if command = '/cs' then begin
            WriteConn(Conn, 'PRIVMSG ChanServ :' + Copy(line, 5, length(line)) + #10);
        end else if command = '/ns' then begin
            WriteConn(conn, 'PRIVMSG NickServ :' + Copy(line, 5, length(line)) + #10);
        end else if command = '/kick' then begin
            WriteConn(conn, 'KICK ' + channel + ' ' + Copy(line, 7, length(line)) + #10);
        end else if command = '/quit' then begin
            if length(line) > 6 then begin // User hat eine quit-message angegeben
                WriteConn(conn, 'QUIT :' + Copy(line, 7, length(line))+#10);
            end else begin
                WriteConn(conn, 'QUIT :Verlassend'+#10);
            end;
        end else if command = '/exit' then begin
            //TODO
        end else if command = '/away' then begin
            if away = true then begin
                WriteConn(conn, ':' + nick + ' AWAY' + #10);
                away := false;
            end else begin
                if length(line) > 6 then begin
                    WriteConn(conn, 'AWAY :' + Copy(line, 7, length(line)) + #10);
                end else begin
                    WriteConn(conn, 'AWAY :Abwesend' + #10);
                end;
                away := true;
            end;
        end else if command = '/back' then begin
            if away = true then begin
                WriteConn(conn, ':' + nick + ' AWAY' + #10);
            end;
        end else if command = '/invite' then begin
            WriteConn(conn, 'INVITE ' + Copy(line, 9, length(line)) + #10);
        end else if command = '/part' then begin
            if length(line) <= 6 then begin
                WriteConn(conn, 'PART ' + channel + #10);
            end else begin
                WriteConn(conn, 'PART ' + Copy(line, 7, length(line)) + #10);
            end;
            TIRCWindow.current.Incoming('Sie haben ' + channel + ' verlassen.', 12);
            TIRCWindow.current.joined := false;
        end else if command = '/mode' then begin
            if Copy(line, 7, 7) = '-' then begin
                WriteConn(conn, 'MODE ' + channel + ' ' + Copy(line, 7, length(line)) + #10);
            end else if Copy(line, 7, 7) = '+' then begin
                WriteConn(conn, 'MODE ' + channel + ' ' + Copy(line, 7, length(line)) + #10);
            end else begin
                WriteConn(conn, 'MODE ' + Copy(line, 7, length(line)) + #10);
            end;
        end else if command = '/nick' then begin
            WriteConn(conn, ':' + nick + ' NICK ' + Copy(line, 7, length(line)) + #10);
            nick := Copy(line, 7, length(line));
        end else if command = '/msg' then begin
            rststr := Copy(line, 6, length(line));
            tmpnick := Copy(rststr, 1, Pos(' ', rststr));
            tmpmsg := Copy(rststr, length(tmpnick)+1, length(rststr));
            WriteConn(conn, 'PRIVMSG ' + tmpnick + ': ' + tmpmsg + #10);
        end else if line[2] = '/' then begin // Doppelter Slash = Kein Kommando, einfach weitergeben
            TIRCWindow.current.Incoming('<' + nick + '> ' + line+#10, 7);
            WriteConn(conn, 'PRIVMSG ' + channel + ' :' + Copy(line, 2, length(line))+#10);
        end else begin
            TIRCWindow.current.Incoming('Ungueltiger Befehl: ' + line, 12);
        end;
    end else begin
        TIRCWindow.current.Incoming('<' + nick + '> ' + line+#10, 7);
        WriteConn(conn, 'PRIVMSG ' + channel + ' :' + line+#10);
    end;
end;

var
    inBuffer, outBuffer: String;
    s: ShortString;
    lineEnd: integer;
    c: char;
    i: integer;
begin
    OpenConnection(conn);

    ClrScr;
    TIRCWindow.current := TIRCWindow.init(nick);
    TIRCWindow.current.commandHandler := @EvaluateIRCCommand;
    TIRCWindow.current.Draw(false);

    inBuffer := '';
    outBuffer := '';

    while not FileEof(conn) do begin
        ReadConn(conn, s);
        inBuffer := inBuffer + s;

        lineEnd := Pos(#10, inBuffer);
        if lineEnd > 0 then begin
            EvaluateIRCInput(conn, Copy(inBuffer, 1, lineEnd-1));
            inBuffer := Copy(inBuffer, lineEnd+1, Length(inBuffer));
        end;

        if not TIRCWindow.current.CheckInput then begin
            TIRCWindow.current.Draw(true);
        end;
    end;
    FileClose(conn);

    TextBackground(0);
    TextColor(7);
    WriteLn;
end.
