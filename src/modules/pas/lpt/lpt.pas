program lpt;
{$MODE ObjFPC}

uses crt, dos, sysutils, classes, tar, helpers, packages, repositories, repository_base;

procedure PrintUsage;
begin
    WriteLn('Aufruf: lpt [scan|list|get <Paket>]');
    WriteLn('  scan: Lädt die Paketlisten von den Servern');
    WriteLn('  list: Zeigt alle installierten Pakete an');
    WriteLn('  get:  Installiert das angegebene Paket');
    WriteLn('  install: Installiert das angegebene Paket aus lokaler Datei');
    WriteLn('           Achtung, Datei als letzten Parameter angeben!');
end;

procedure Scan;
var
    i: Integer;
    repos: TCollection;
    repo: TRepository;
begin
    mkpath(getConfigRoot);

    try
        ReadPkgsrc(getConfigRoot + 'pkgsrc');
    except
        on e: Exception do begin
            TextColor(12);
            WriteLn('Kann Repository-Liste nicht laden:');
            WriteLn(e.message);
            TextColor(7);
        end;
    end;
    repos := GetAllRepositories();

    for i := 0 to repos.count - 1 do begin
        repo := TRepository(repos.items[i]);
        try
            repo.PrepareLists();
        except
            on e: EDownload do begin
                TextColor(12);
                WriteLn('Download der Paketliste für ' + repo.name + ' fehlgeschlagen:');
                WriteLn(e.message);
                TextColor(7);
            end;
        end;
    end;
end;

procedure ReadPkglists(pkgset: TPackageSet);
var
    repo: TRepository;
    repos: TCollection;
    i: integer;
begin
    repos := GetAllRepositories();
    for i := 0 to repos.count - 1 do begin
        repo := TRepository(repos.items[i]);
        try
            repo.FetchLists(pkgset);
            WriteLn('Laden der Pakete von ' + repo.name + 'erfolgreich.');
        except
            on e: Exception do begin
                TextColor(12);
                WriteLn('Fehler: ' + e.message);
                TextColor(7);
            end;
        end;
    end;
end;

procedure Get(pkgname: String; reinstall: boolean);


    function Install(version: TPackageVersion; reinstall: boolean): boolean;
    var
        filename: String;
        repos: TRepository;
        installarchive: TTarArchive;
    begin
        Install := false;

        // Wenn der Pfad bereits existiert, gehen wir davon aus, dass das
        // Paket bereits installiert ist
        filename := 'file:/packages/' + version.pkg.name + '/' +
            version.version + '/' + version.section.section + '/';
        if FileExists(filename) and not reinstall then begin
            TextColor(15);
            WriteLn('Bereits installiert: ' + version.pkg.name + '/' +
                version.section.section);
            TextColor(7);
            Install := true;
            exit;
        end;

        repos := GetRepository(version.repository);

        // Ansonsten herunterladen und installieren
        TextColor(15);
        WriteLn('Herunterladen und Entpacken von ' + version.pkg.name + '/' + version.section.section);
        TextColor(7);
        installarchive := repos.Download(version);
        if installarchive <> nil then begin
            Untar(installarchive);
        end;

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
    pkgset: TPackageSet;
    section: String;

    instset: TPackageSet;
    pkg: TPackage;
    sect: TPackageSection;
    version: TPackageVersion;
    i, j, k: integer;
begin
    try
        ReadPkgsrc(getConfigRoot + 'pkgsrc');
        pkgset := TPackageSet.create;

        ReadPkglists(pkgset);
    except
        on e: EPackageList do begin
            TextColor(12);
            WriteLn('Kann die Paketlisten nicht einlesen: ');
            WriteLn(e.message);
            TextColor(7);
        end;
    end;

    // Parameter hat die Form Paketname/Section. Wenn keine Section angegeben
    // ist, wird bin als Default benutzt.
    i := Pos('/', pkgname);
    if i > 0 then begin
        section := Copy(pkgname, i + 1, Length(pkgname));
        pkgname := Copy(pkgname, 1, i - 1);
    end else begin
        section := 'bin';
    end;

    version := pkgset.GetCurrentVersion(pkgname, section);
    
    if version = nil then begin
        TextColor(12);
        WriteLn('Konnte keine Version von ' + pkgname + ' finden');
        TextColor(7);
        exit;
    end;
    
    instset := TPackageSet.create();
    instset.AddVersion(version);
    try
        instset.AddDependencies(pkgset);
    except
        on e: EDependency do begin
            TextColor(12);
            WriteLn('Konnte die Abhaengigkeiten des Pakets nicht aufloesen:');
            WriteLn(e.message);
            TextColor(7);
            exit;
        end;
    end;

    for i := 0 to instset.packages.count - 1 do begin
        pkg := TPackage(instset.packages.items[i]);
        for j := 0 to pkg.sections.count - 1 do begin
            sect := TPackageSection(pkg.sections.items[j]);
            for k := 0 to sect.versions.count - 1 do begin
                version := TPackageVersion(sect.versions.items[k]);
                try
                    Install(version, reinstall);
                except
                    on e: Exception do begin
                        TextColor(12);
                        WriteLn('Fehler: ' + e.message);
                        TextColor(7);
                    end;
                end;
            end;
        end;
    end;

end;

procedure Install(pkgfile: String; pkgname: String; reinstall: boolean);
var
    repo: TRepository;
begin
    AddRepository(pkgname, pkgfile, singlefile);
    Get(pkgname, reinstall);
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
    ReadPkgsrc(getConfigRoot + 'pkgsrc');
    pkgset := TPackageSet.create;
    ReadPkglists(pkgset);

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

procedure CfgAdddoc(path, pubname: String);
begin
    path := path + #0;
    pubname := 'file:/system/lpt-doc/' + pubname + #0;

    mkpath('file:/system/lpt-doc/');
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
            Get(system.ParamStr(2), false);
        end else begin
            WriteLn('Falsche Parameterzahl');
            PrintUsage;
        end;
    end else if system.ParamStr(1) = 'reget' then begin
        if system.ParamCount = 2 then begin
            Get(system.ParamStr(2), true);
        end else begin
            WriteLn('Falsche Parameterzahl');
            PrintUsage;
        end;
    end else if system.ParamStr(1) = 'install' then begin
        if system.ParamCount = 3 then begin
            Install(system.ParamStr(3), system.ParamStr(2), false);
        end else begin
            WriteLn('Falsche Parameterzahl');
            PrintUsage;
        end;
    end else if system.ParamStr(1) = 'reinstall' then begin
        if system.ParamCount = 3 then begin
            Install(system.ParamStr(3), system.ParamStr(2), true);
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
    end else if system.ParamStr(1) = 'cfg-adddoc' then begin
        if system.ParamCount = 3 then begin
            CfgAdddoc(system.ParamStr(2), system.ParamStr(3));
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
