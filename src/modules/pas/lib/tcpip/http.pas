unit http;
{$MODE ObjFPC}
{$H+}

interface
uses sysutils, classes;

const
    HTTP_OK             = 200;
    HTTP_FORBIDDEN      = 403;
    HTTP_NOT_FOUND      = 404;

type
    EConnectionAborted = class(Exception) end;

    HTTPHeader = class(TCollectionItem)
        public
            name: String;
            value: String;
    end;

    HTTPRequest = class;
    HTTPHandler = procedure (connection: HTTPRequest);

    HTTPRequest = class
        
        constructor create(host: String; path: String);        
        constructor create(url: String);
        destructor done;

        procedure setRequestHeader(name: String; value: String);
        procedure setRequestBody(value: String);

        procedure get;
        procedure post;

        private
            requestSent: boolean;

            host: String;
            path: String;

            f_requestHeaders: String;
            f_requestBody: String;

            f_responseHeaders: TCollection;
            f_responseBody: PByte;

            f_statusCode: integer;

            f_onHeadersRead: HTTPHandler;
            f_onReadTick: HTTPHandler;
            
            f_responseLength: integer;
            f_remaining: integer;

            function getResponseHeader(name: String): String;
            function getResponseBody: String;
        
        public
            property statusCode: integer read f_statusCode;
            property responseHeaders: TCollection read f_responseHeaders;
            property response[name: String]: String read getResponseHeader;
            property responseBody: String read getResponseBody;
            property binaryResponseBody: PByte read f_responseBody;

            property onHeadersRead: HTTPHandler read f_onHeadersRead write f_onHeadersRead;
            property onReadTick: HTTPHandler read f_onReadTick write f_onReadTick;

            property responseLength: integer read f_responseLength;
            property responseRemaining: integer read f_remaining;
        
    end;

var
    debug: boolean;

implementation

uses tcpip;
        
const
    BYTES_AT_ONCE       = 4096;

constructor HTTPRequest.create(url: String);
var
    slash: integer;
    thost, tpath: String;
begin
    if Copy(url, 1, 7) = 'http://' then begin
        Delete(url, 1, 7);
    end;

    slash := Pos('/', url);
    if slash > 0 then begin
        thost := Copy(url, 1, slash - 1);
        tpath := Copy(url, slash, length(url));
    end else begin
        thost := url;
        tpath := '/';
    end;

    self.create(thost, tpath);
end;

constructor HTTPRequest.create(host: String; path: String);        
begin
    requestSent := false;

    self.host := host;
    self.path := path;

    f_requestHeaders := '';
    f_requestBody := '';
    
    f_responseHeaders := nil;
    f_responseBody := nil;
            
    f_onHeadersRead := nil;
    f_onReadTick := nil;

    f_responseLength := -1;
    f_remaining := -1;
end;

destructor HTTPRequest.done;
begin
end;


procedure HTTPRequest.setRequestHeader(name: String; value: String);
begin
    // FIXME Überschreiben, wenn der Header schon gesetzt ist
    f_requestHeaders := f_requestHeaders + name + ': ' + value + #13#10;
end;

procedure HTTPRequest.setRequestBody(value: String);
begin
    f_requestBody := value;
end;


procedure HTTPRequest.get;
var
    conn:       TCPConnection;
    bytesRead:  integer;
    curPos:     PByte;
    line:       String;
begin
    conn := TCPConnection.create(host, 80);
    
    if debug then WriteLn('HTTP: Sende Anfrage');

    conn.write('GET '+path+' HTTP/1.0'#13#10);
    conn.write('Host: '+host+#13#10);
    conn.write('Connection: close'#13#10);
    conn.write(f_requestHeaders);
    conn.write(#13#10);

    if debug then begin
        WriteLn('HTTP: Warte auf Antwort');
        Flush(output);
    end;

    // Statuszeile verarbeiten
    conn.ignoreCR := true;
    line := conn.readLine;
    if debug then begin
        WriteLn('[STATUS] ', line);
        Flush(output);
    end;

    Delete(line, 1, Pos(' ', line));
    f_statusCode := StrToInt(Copy(line, 1, Pos(' ', line) - 1));

    // Header einlesen
    if f_responseHeaders <> nil then begin
        f_responseHeaders.free;
    end;
    f_responseHeaders := TCollection.create(HTTPHeader);

    while not conn.closed do begin
        line := conn.readLine;

        if debug then begin
            WriteLn('[HEADER] ', line);
            Flush(output);
        end;
        
        if line = '' then begin
            break;
        end;

        with HTTPHeader(f_responseHeaders.add()) do begin
            name := Copy(line, 1, Pos(': ', line) - 1);
            value := Copy(line, Pos(': ', line) + 2, length(line));
        end;
    end;

    if f_onHeadersRead <> nil then begin
        f_onHeadersRead(self);
    end;

    // Body einlesen
    conn.ignoreCR := false; 
    try
        f_remaining := StrToInt(response['Content-Length']);
    except 
        on EConvertError do begin
            if debug then begin
                WriteLn('Content-Length ist keine Zahl: "', response['Content-Length'], '"');
            end;
            f_remaining := -1;
        end;
    end;
    
    f_responseLength := f_remaining;
    if f_responseLength >= 0 then begin
        WriteLn('Erwarte ', f_responseLength, ' bytes');
        f_responseBody := GetMem(f_responseLength);
    end else begin
        WriteLn('Erwarte ', BYTES_AT_ONCE, ' bytes');
        f_responseBody := GetMem(BYTES_AT_ONCE);
    end;

    curPos := f_responseBody;
    while (not conn.closed) and (f_remaining <> 0) do begin
        repeat
            if (f_remaining > BYTES_AT_ONCE) or (f_remaining = -1) then begin
                bytesRead := conn.read(curPos^, BYTES_AT_ONCE);
            end else begin
                bytesRead := conn.read(curPos^, f_remaining);
            end;
        until bytesRead <> 0;
        inc(curPos, bytesRead);
   
        if f_remaining = -1 then begin
            Inc(f_responseLength, bytesRead);
            if debug then begin
                WriteLn('Erwarte ', f_responseLength + BYTES_AT_ONCE,'bytes');
                Flush(output);
            end;
            ReallocMem(f_responseBody, f_responseLength + BYTES_AT_ONCE);
        end else begin
            Dec(f_remaining, bytesRead);
        end;
        
        if debug then begin
            WriteLn(bytesRead, ' Bytes gelesen, verbleibend: ', f_remaining);
            Flush(output);
        end;

        if f_onReadTick <> nil then begin
            f_onReadTick(self);
        end;

        if debug then begin
            WriteLn('[ BODY ] ', line);
            Flush(output);
        end;
        
        //f_responseBody := f_responseBody + line + #10;
    end;

    conn.done;

    if f_remaining > 0 then begin
        raise EConnectionAborted.create(IntToStr(f_remaining) + ' Bytes fehlen');
    end;
end;

procedure HTTPRequest.post;
begin
    raise Exception.create('Nicht implementiert');
end;
            
function HTTPRequest.getResponseHeader(name: String): String;
var
    i:      integer;
    header: HTTPHeader;
begin
    for i := 0 to responseHeaders.count - 1 do begin
        header := HTTPHeader(responseHeaders.items[i]);
        if header.name = name then begin
            exit(header.value);
        end;
    end;

    exit('');
end;

function HTTPRequest.getResponseBody: String;
begin
    if f_responseBody = nil then begin
        Result := '';
    end else begin
        SetLength(Result, f_responseLength);
        Move(f_responseBody, Result[1], f_responseLength);
    end;
end;

end.
