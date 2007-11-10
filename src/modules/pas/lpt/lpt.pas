program pashello;
{$MODE ObjFPC}

uses dos, sysutils, classes, tar, tcpip, http, packages;

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

procedure mkpath(path: string);
var
    i: integer;
    prefix: String;
begin
    WriteLn('mkpath ' + path);
    i := 1;
    while i <= length(path) do begin
        if path[i] = '/' then begin
            Write('> ');
            prefix := Copy(path, 1, i);
            if not FileExists(prefix) then begin
                mkdir(prefix);
                Write('mkdir ');
            end;
            WriteLn(prefix);
        end;
        Inc(i);
    end;
end;

procedure Untar(tarfile: String);
var
    tar: TTarArchive;
    path: String;
    dir, filename, ext: String;

    f: file;
begin
    tar := TTarArchive.create(tarfile);
    while tar.hasNext do begin
        path := tar.NextFilename;
        dos.FSplit(path, dir, filename, ext);

        mkpath('file:/' + dir);
        
        Assign(f, 'file:/' + path);
        Rewrite(f, 1);
        tar.ExtractFile(f);
        Close(f);
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
    mkpath(configRoot);

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
    section: String;

    version: TPackageVersion;
    filename: String;
    url: String;
begin
    pkgset := TPackageSet.create;

    dos.FindFirst(configRoot + 'pkglist.*', 0, srec);
    while DosError = 0 do begin
        WriteLn('Lese Paketliste ein: ' + srec.name);
        // TODO Aus dem Namen die URL bestimmen
        pkgset.ScanFile(configRoot + srec.name, 'http://192.168.1.3:8080/lpt/');
        dos.FindNext(srec);
    end;

    section := 'bin';
    version := pkgset.GetCurrentVersion(pkgname, section);
    
    if version = nil then begin
        WriteLn('Konnte keine Version von ' + pkgname + ' finden');
        exit;
    end;

    filename := pkgname + '-' + section + '-' + version.version 
        + '-' + arch + '.tar';

    url := version.repository + filename;
    Download(url, configRoot + 'pkg_' + filename);

    WriteLn('Entpacken von ' + pkgname + '/' + section);
    Untar(configRoot + 'pkg_' + filename);
end;

procedure List;
var
    srec: SearchRec;
    pkgset: TPackageSet;
    pkg: TPackage;
    sect: TPackageSection;
    ver: TPackageVersion;


    i, j, k: integer;
begin
    pkgset := TPackageSet.create;

    dos.FindFirst(configRoot + 'pkglist.*', 0, srec);
    while DosError = 0 do begin
        WriteLn('Lese Paketliste ein: ' + srec.name);
        // TODO Aus dem Namen die URL bestimmen
        pkgset.ScanFile(configRoot + srec.name, 'http://192.168.1.3:8080/lpt/');
        dos.FindNext(srec);
    end;

    for i := 0 to pkgset.packages.count - 1 do begin
        pkg := TPackage(pkgset.packages.items[i]);
        WriteLn('+ ' + pkg.name);
        for j := 0 to pkg.sections.count - 1 do begin
            sect := TPackageSection(pkg.sections.items[i]);
            WriteLn('| + ' + sect.section);
            for k := 0 to sect.versions.count - 1 do begin
                ver := TPackageVersion(sect.versions.items[i]);
                WriteLn('| | + ' + ver.version);
            end;
        end;
    end;
end;

begin
    if system.ParamCount < 1 then begin
        PrintUsage;
        halt(1);
    end;

    if system.ParamStr(1) = 'scan' then begin
        Scan;
    end else if system.ParamStr(1) = 'list' then begin
        List;
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
