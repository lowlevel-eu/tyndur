unit tcpip;
{$MODE ObjFPC}
{$H+}

interface
uses sysutils, crt;

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
            function Read(var outbuffer; bytes: integer): integer;

        private
            procedure GetInput;

        private
            conn:       longint;
            buffer:     String;
            fIgnoreCR:  boolean;

        public
            property ignoreCR: boolean read fIgnoreCR write fIgnoreCR;

    end;

var
    debug: boolean;

implementation

const BLOCKSIZE = 65536;

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
    if conn <> -1 then begin
        FileClose(conn);
    end;
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
        Delete(buffer, 1, index);
    end;
        
    if ignoreCR and (index > 1) and (Result[index - 1] = #13) then begin
        SetLength(Result, Length(Result) - 1);
    end;
end;
            
function TCPConnection.Read(var outbuffer; bytes: integer): integer;
var
    curPos: PByte;
    size: integer;
    bytesRead: integer;
begin
    curPos := @outbuffer;

    if bytes < length(buffer) then begin
        Move(buffer[1], outbuffer, bytes);
        Delete(buffer, 1, bytes);
        exit(bytes);
    end else begin
        size := length(buffer);
        Inc(curPos, size);
        Dec(bytes, size);
        bytesRead := size;
        Move(buffer[1], outbuffer, length(buffer));
        buffer := '';
    end;
    
    while (not FileEof(conn)) and (bytes > 0) do begin
        if bytes > BLOCKSIZE then begin
            size := FileRead(conn, curPos^, BLOCKSIZE);
        end else begin
            size := FileRead(conn, curPos^, bytes);
        end;
        if size < 0 then begin
            break;
        end;

        Dec(bytes, size);
        Inc(bytesRead, size);
        Inc(curPos, size);
    end;

    Result := bytesRead;
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
