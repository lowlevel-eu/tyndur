unit tcpip;
{$MODE ObjFPC}
//{$H+}

interface
uses sysutils;

type
    EConnect = class(Exception)
    end;
    
    EConnectionClosed = class(Exception)
    end;

    TCPConnection = class
        public
            constructor create(ip: dword; port: word);
            constructor create(ip: String; port: word);

            destructor done;

            function HasInput: boolean;
            function ReadLine: String;
            function Closed: boolean;

            procedure Write(s: String);

        private
            procedure GetInput;

            conn:       longint;
            buffer:     String;
            fIgnoreCR:  boolean;

        public
            property ignoreCR: boolean read fIgnoreCR write fIgnoreCR;

    end;

var
    debug: boolean;

implementation

function long2ip(ip: dword): String;
begin
    Result :=          IntToStr((ip shr 24) and $FF);
    Result := Result + IntToStr((ip shr 16) and $FF);
    Result := Result + IntToStr((ip shr  8) and $FF);
    Result := Result + IntToStr((ip shr  0) and $FF);
end;

constructor TCPConnection.create(ip: dword; port: word);
begin
    create(long2ip(ip), port);
end;

constructor TCPConnection.create(ip: String; port: word);
var
    filename: String;
begin
    filename := 'tcpip:/' + ip + ':' + IntToStr(port);
    if debug then begin
        WriteLn('TCPConnection.create: filename = ' + filename);
    end;

    conn := FileOpen(filename, fmOpenReadWrite);
    if conn = -1 then begin
        raise EConnect.create('Konnte Verbindung nicht herstellen');
    end;

    ignoreCR := true;
end;

destructor TCPConnection.done;
begin
    FileClose(conn);
end;


procedure TCPConnection.GetInput;
var
    bytes:  longint;
    s:      ShortString;
begin
    if FileEof(conn) then begin
        raise EConnectionClosed.create('Verbindung wurde geschlossen');
    end;

    bytes := FileRead(conn, s[1], 255);
    if (bytes > 0) and (bytes < 256) then begin
        s[0] := char(bytes);
        buffer := buffer + s;
    end;
end;

function TCPConnection.ReadLine: String;
var
    index:  longint;
begin
    while (not FileEof(conn)) and (Pos(#10, buffer) <= 0) do begin
        GetInput;
    end;

    index := Pos(#10, buffer);
    if index <= 0 then begin
        Result := buffer;
        buffer := '';
    end else begin
        Result := Copy(buffer, 1, index - 1);

        if (Length(buffer) > index) 
            and ignoreCR 
            and (buffer[index + 1] = #13) 
        then begin
            buffer := Copy(buffer, index + 2, length(buffer));
        end else begin
            buffer := Copy(buffer, index + 1, length(buffer));
        end;
    end;
end;


procedure TCPConnection.Write(s: String);
begin
    if FileEof(conn) then begin
        raise EConnectionClosed.create('Verbindung wurde geschlossen');
    end;

    FileWrite(conn, s[1], length(s));
end;

function TCPConnection.HasInput: boolean;
begin    
    GetInput;
    HasInput := (length(buffer) > 0);
end;
            
function TCPConnection.Closed: boolean;
begin
    Closed := FileEof(conn);
end;

end.
