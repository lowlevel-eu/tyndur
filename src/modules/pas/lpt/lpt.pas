program pashello;
{$MODE ObjFPC}

uses dos, sysutils, classes, tar, tcpip, http, packages, repositories;

const
    configRoot: String = 'file:/config/lpt/';
    arch: String = 'i386';


procedure mkpath(path: string);
var
    i: integer;
    prefix: String;
begin
    //WriteLn('mkpath ' + path);
    i := 1;
    while i <= length(path) do begin
        if path[i] = '/' then begin
            //Write('> ');
            prefix := Copy(path, 1, i);
            if not FileExists(prefix) then begin
                //WriteLn('mkdir ''', prefix, '''');
                mkdir(prefix);
            end else begin;
                //WriteLn(prefix);
            end;
        end;
        Inc(i);
    end;
end;

procedure Untar(tar: TTarArchive);
var
    path: String;
    dir, filename, ext: String;

    f: file;
begin
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

procedure Untar(tarfile: String);
var
    tar: TTarArchive;
begin
    tar := TTarArchive.create(tarfile);
    Untar(tar);
end;



procedure DrawProgress(http: HTTPRequest);
const
    i: Integer = 0;
begin
    Inc(i);
    if i mod 16 = 0 then begin
        exit;
    end;

    if http.responseRemaining >= 0 then begin
        Write(#13, (http.responseLength - http.responseRemaining), '/', http.responseLength, ' Bytes empfangen');
    end else begin
        Write(#13, http.responseLength, ' Bytes empfangen');
    end;
end;

function Download(url, filename: String; do_untar: boolean = false): boolean;
var
    f: File;
    httpc: HTTPRequest;
    tar: TTarArchive;
begin
    Download := false;
    try
        httpc := HTTPRequest.create(url);
        httpc.setRequestHeader('Pragma', 'no-cache');
        httpc.setRequestHeader('Cache-Control', 'no-cache');
        WriteLn('Herunterladen von ', url);
        Write('Verbinde...');
        httpc.onReadTick := @DrawProgress;
        httpc.get;
        WriteLn;

        if httpc.statusCode <> 200 then begin
            WriteLn('Fehler: HTTP-Statuscode ', httpc.statusCode);
        end else if do_untar then begin
            tar := TTarArchive.create(httpc.binaryResponseBody, httpc.responseLength);
            Untar(tar);
            Download := true;
        end else begin
            Assign(f, filename);
            Rewrite(f, httpc.responseLength);
            BlockWrite(f, httpc.binaryResponseBody^, 1);
            Close(f);
            Download := true;
        end;

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


    function Install(version: TPackageVersion): boolean;
    var
        filename: String;
        url: String;
    begin
        Install := false;

        // Wenn der Pfad bereits existiert, gehen wir davon aus, dass das
        // Paket bereits installiert ist
        filename := 'file:/packages/' + version.pkg.name + '/' +
            version.version + '/' + version.section.section + '/';
        if FileExists(filename) then begin
            WriteLn('Bereits installiert: ' + version.pkg.name + '/' +
                version.section.section);
            Install := true;
            exit;
        end;

        // Ansonsten herunterladen und installieren
        WriteLn('Herunterladen und Entpacken von ' + version.pkg.name + '/' + version.section.section);
        filename := version.pkg.name + '-' + version.section.section + '-' + version.version
            + '-' + arch + '.tar';
        url := version.repository + filename;
        if not Download(url, '', true) then begin
            exit;
        end;
        WriteLn;

        filename := 'file:/packages/' + version.pkg.name + '/' +
            version.version + '/postinstall-' + version.section.section;

        WriteLn('Suche ' + filename);
        if FileExists(filename) then begin
            WriteLn('Richte ' + version.pkg.name + '/' + version.section.section + ' ein');
            exec(filename, '');
        end;

        Install := true;
    end;

var
    srec: SearchRec;
    pkgset: TPackageSet;
    section: String;
    repo: TRepository;

    instset: TPackageSet;
    pkg: TPackage;
    sect: TPackageSection;
    version: TPackageVersion;
    i, j, k: integer;
begin
    ReadPkgsrc(configRoot + 'pkgsrc');
    pkgset := TPackageSet.create;

    dos.FindFirst(configRoot + 'pkglist.*', 0, srec);
    while DosError = 0 do begin
        WriteLn('Lese Paketliste ein: ' + srec.name);
        
        repo := GetRepository(Copy(srec.name, length('pkglist.*'), length(srec.name)));
        if repo = nil then begin
            WriteLn('Fehler: Nicht in pkgsrc gefunden. Ueberspringe Quelle.');
            continue;
        end;

        pkgset.ScanFile(configRoot + srec.name, repo.url);
        dos.FindNext(srec);
    end;


    section := 'bin';
    version := pkgset.GetCurrentVersion(pkgname, section);
    
    if version = nil then begin
        WriteLn('Konnte keine Version von ' + pkgname + ' finden');
        exit;
    end;
    
    instset := TPackageSet.create();
    instset.AddVersion(version);
    instset.AddDependencies(pkgset);

    for i := 0 to instset.packages.count - 1 do begin
        pkg := TPackage(instset.packages.items[i]);
        for j := 0 to pkg.sections.count - 1 do begin
            sect := TPackageSection(pkg.sections.items[j]);
            for k := 0 to sect.versions.count - 1 do begin
                version := TPackageVersion(sect.versions.items[k]);
                if not Install(version) then begin
                    WriteLn('Breche Paketinstallationen ab.');
                    exit;
                end;
            end;
        end;
    end;

end;

procedure List;
var
    srec: SearchRec;
    pkgset: TPackageSet;
    pkg: TPackage;
    sect: TPackageSection;
    ver: TPackageVersion;
    dep: TDependency;

    repo: TRepository;

    i, j, k, l: integer;
begin
    ReadPkgsrc(configRoot + 'pkgsrc');
    pkgset := TPackageSet.create;

    dos.FindFirst(configRoot + 'pkglist.*', 0, srec);
    while DosError = 0 do begin
        WriteLn('Lese Paketliste ein: ' + srec.name);

        repo := GetRepository(Copy(srec.name, length('pkglist.*'), length(srec.name)));
        if repo = nil then begin
            WriteLn('Fehler: Nicht in pkgsrc gefunden. Ueberspringe Quelle.');
            continue;
        end;

        pkgset.ScanFile(configRoot + srec.name, repo.url);
        dos.FindNext(srec);
    end;

    for i := 0 to pkgset.packages.count - 1 do begin
        pkg := TPackage(pkgset.packages.items[i]);
        WriteLn('+ ' + pkg.name);
        for j := 0 to pkg.sections.count - 1 do begin
            sect := TPackageSection(pkg.sections.items[j]);
            WriteLn('| + ' + sect.section);
            for k := 0 to sect.versions.count - 1 do begin
                ver := TPackageVersion(sect.versions.items[k]);
                WriteLn('| | + ' + ver.version);

                for l := 0 to ver.preinstDependencies.count - 1 do begin
                    dep := TDependency(ver.preinstDependencies.items[l]);
                    WriteLn('| | |  [i] ' + dep.pkgname + ' (' + dep.section + ')');
                end;
                
                for l := 0 to ver.runDependencies.count - 1 do begin
                    dep := TDependency(ver.runDependencies.items[l]);
                    WriteLn('| | |  [r] ' + dep.pkgname + ' (' + dep.section + ')');
                end;

            end;
        end;
    end;
end;

function c_lostio_create_link(target, link: PChar; hard: boolean): integer; cdecl; external name 'io_create_link';

procedure CfgAddbin(path, pubname: String);
begin
    path := path + #0;
    pubname := 'file:/system/lpt-bin/' + pubname + #0;

    mkpath('file:/system/lpt-bin/');
    c_lostio_create_link(@path[1], @pubname[1], False);
end;

procedure CfgAddlib(path, pubname: String);
begin
    path := path + #0;
    pubname := 'file:/system/lib/' + pubname + #0;

    mkpath('file:/system/lib/');
    c_lostio_create_link(@path[1], @pubname[1], False);
end;

procedure CfgAddinc(path, pubname: String);
begin
    path := path + #0;
    pubname := 'file:/system/include/' + pubname + #0;

    mkpath('file:/system/include/');
    c_lostio_create_link(@path[1], @pubname[1], False);
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
    end else if system.ParamStr(1) = 'cfg-addbin' then begin
        if system.ParamCount = 3 then begin
            CfgAddbin(system.ParamStr(2), system.ParamStr(3));
        end else begin
            WriteLn('Falsche Parameterzahl');
            PrintUsage;
        end;
    end else if system.ParamStr(1) = 'cfg-addlib' then begin
        if system.ParamCount = 3 then begin
            CfgAddlib(system.ParamStr(2), system.ParamStr(3));
        end else begin
            WriteLn('Falsche Parameterzahl');
            PrintUsage;
        end;
    end else if system.ParamStr(1) = 'cfg-addinc' then begin
        if system.ParamCount = 3 then begin
            CfgAddinc(system.ParamStr(2), system.ParamStr(3));
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
