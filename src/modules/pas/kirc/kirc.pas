program kirc;
{$MODE ObjFPC}
uses sysutils, crt, ircwindow;

var
    theIRCWindow: TIRCWindow;
    conn: longint;
    nick: string;
    channel: string;

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
end;

procedure OpenConnection(var conn: longint);
var
    server: String;
    port: String;
begin
    Write('Server [217.26.49.12]: ');
    ReadLn(server);
    if server = '' then begin
        server := '217.26.49.12';
    end;

    Write('Port [6667]: ');
    ReadLn(port);
    if port = '' then begin
        port := '6667';
    end;
    
    Write('Nickname: ');
    ReadLn(nick);

    conn := FileOpen('tcpip:/'+server+':'+port, fmOpenReadWrite);

    WriteConn(conn, 'USER kirc 0 0 :kirc/LOST'#10);
    WriteConn(conn, 'NICK '+nick+#10);
end;

function GetNickFromPrefix(prefix: String): String;
var
    bang: integer;
begin
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
begin
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
        WriteConn(conn, 'PONG '+parameter+#10);
    end 
    else if command = 'JOIN' then begin
        theIRCWindow.Incoming(prefix + ' hat den Kanal betreten', 2);
    end 
    else if command = 'PART' then begin
        theIRCWindow.Incoming(GetNickFromPrefix(prefix) 
            + ' hat den Kanal verlassen (' 
            + ShiftParameter(parameter) +')', 2);
    end 
    else if command = 'QUIT' then begin
        theIRCWindow.Incoming(GetNickFromPrefix(prefix) 
            + ' hat den Server verlassen ('
            + ShiftParameter(parameter) +')', 2);
    end 
    else if command = 'NICK' then begin
        theIRCWindow.Incoming(GetNickFromPrefix(prefix) 
            + ' nennt sich jetzt ' 
            + ShiftParameter(parameter), 2);
    end 
    else if command = 'KICK' then begin
        ShiftParameter(parameter);
        msg := ShiftParameter(parameter);
        theIRCWindow.Incoming(prefix + ' hat ' + msg
            + ' aus dem Kanal geworfen (' + ShiftParameter(parameter) + ')', 2);
        
        if msg = nick then begin
            channel := '';
            theIRCWindow.title := nick;
        end;
    end 
    else if command = 'MODE' then begin
        ShiftParameter(parameter);
        theIRCWindow.Incoming(prefix + ' setzt den Modus: ' 
            + parameter, 9);
    end 
    else if command = 'TOPIC' then begin
        ShiftParameter(parameter);
        theIRCWindow.Incoming(GetNickFromPrefix(prefix) 
            + ' setzt das Kanalthema auf: ' + ShiftParameter(parameter), 9);
    end 
    else if command = 'PRIVMSG' then begin
        ShiftParameter(parameter);
        msg := ShiftParameter(parameter);

        if Copy(msg, 1, 8) = #1'ACTION ' then begin
            ShiftParameter(msg);
            theIRCWindow.Incoming(GetNickFromPrefix(prefix) + ' '
                + Copy(msg, 1, length(msg) - 1) + #10, 14);
        end else begin
            theIRCWindow.Incoming('<' + GetNickFromPrefix(prefix) + '> ' 
                + msg, 7);
        end;
    end 
    else if command = 'ERROR' then begin
        theIRCWindow.Incoming(line, 12);
    end 
    else begin
        theIRCWindow.Incoming(line, 8);
    end;
end;

procedure EvaluateIRCCommand(const line: String);
begin
    if channel = '' then begin
        if Copy(line, 1, 6) = '/join ' then begin
            channel := Copy(line, 7, length(line));
            theIRCWindow.title := nick + ' in ' + channel;
            WriteConn(conn, 'JOIN '+ channel + #10);
        end else begin
            theIRCWindow.Incoming('Ungueltiger Befehl: ' + line, 12);
        end;
    end else begin
        if line[1] = '/' then begin
            WriteConn(conn, Copy(line, 2, length(line)) + #10);
        end else begin
            theIRCWindow.Incoming('<' + nick + '> ' + line+#10, 7);
            WriteConn(conn, 'PRIVMSG '+channel+' :'+line+#10);
        end;
    end;
end;

var 
    inBuffer, outBuffer, s: String;
    lineEnd: integer;
    c: char;
begin
    OpenConnection(conn);

    ClrScr;
    theIRCWindow := TIRCWindow.init(nick);
    theIRCWindow.commandHandler := @EvaluateIRCCommand;
    theIRCWindow.Draw(false);

    channel := '';

    inBuffer := '';
    outBuffer := '';
    while not FileEof(conn) do begin
        ReadConn(conn, s);
        inBuffer := inBuffer + s;
       
        lineEnd := Pos(#10, inBuffer);
        if lineEnd > 0 then begin
            EvaluateIRCInput(conn, Copy(inBuffer, 1, lineEnd-1));
            inBuffer := Copy(inBuffer, lineEnd+1, Length(s));
        end;

        theIRCWindow.CheckInput;

    end;
    FileClose(conn);

    TextBackground(0);
    TextColor(7);
    WriteLn;
end.
