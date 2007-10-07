program pashello;
{$MODE ObjFPC}

uses dos, sysutils, classes, tcpip, http, packages;

const
    configRoot: String = 'file:/config/lpt/';
    arch: String = 'i386';

procedure DrawProgress(http: HTTPRequest);
begin
    if http.responseRemaining >= 0 then begin
        Write(#13, (http.responseLength - http.responseRemaining), '/', http.responseLength, ' Bytes empfangen');
    end else begin
        Write(#13, http.responseLength, ' Bytes empfangen');
    end;
end;

procedure Download(url, filename: String);
var
    f: File;
    httpc: HTTPRequest;
begin
    try
        httpc := HTTPRequest.create(url);
        WriteLn('Herunterladen von ', url);
        Write('Verbinde...');
        httpc.onReadTick := @DrawProgress;
        httpc.get;
        WriteLn;

        Assign(f, filename);
        Rewrite(f, httpc.responseLength);
        BlockWrite(f, httpc.binaryResponseBody^, 1);
        Close(f);

        httpc.done;
    except 
        on e: Exception do WriteLn('EXCEPTION: ', e.message);
    end;
    
end;

procedure PrintUsage;
begin
    WriteLn('Aufruf: lpt [scan|get <Paket>]');
end;

procedure Scan;
var
    f: Text;
    name: String;
    url: String;
    space: integer;
begin
    Assign(f, configRoot + 'pkgsrc');
    Reset(f);
    while not eof(f) do begin
        ReadLn(f, url);
        if url = '' then begin
            continue;
        end;

        space := Pos(' ', url);
        if  space > 0 then begin
            name := Copy(url, 1, space - 1);
            url := Copy(url, space + 1, length(url));
        end else begin
            WriteLn('Ungueltige Paketquelle: ' + url);
            continue;
        end;

        if url[length(url)] <> '/' then begin
            url := url + '/packages.' + arch;
        end else begin
            url := url + 'packages.' + arch;
        end;

        WriteLn('Lade Paketliste ' + name + ' von ' + url);
        Download(url, configRoot + 'pkglist.' + name);
    end;
    Close(f);
end;

procedure Get(pkgname: String);
var
    srec: SearchRec;
    pkgset: TPackageSet;
begin
    pkgset := TPackageSet.create;

    dos.FindFirst(configRoot + 'pkglist.*', 0, srec);
    while DosError = 0 do begin
        WriteLn(srec.name);
        pkgset.ScanFile(configRoot + srec.name);
        dos.FindNext(srec);
    end;
end;

begin
    if system.ParamCount < 1 then begin
        PrintUsage;
        halt(1);
    end;

    if system.ParamStr(1) = 'scan' then begin
        Scan;
    end else if system.ParamStr(1) = 'get' then begin
        if system.ParamCount = 2 then begin
            Get(system.ParamStr(2));
        end else begin
            WriteLn('Falsche Parameterzahl');
            PrintUsage;
        end; 
    end else begin
        WriteLn('Ungueltige Aktion: ', system.ParamStr(1));
        PrintUsage;
        halt(1);
    end;
end.
