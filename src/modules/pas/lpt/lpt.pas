program lpt;
{$MODE ObjFPC}

uses
    crt, dos, sysutils, classes, tar, multilang,
    helpers, packages, repositories, repository_base;

resourcestring
    rsUsage =
        'Aufruf: lpt [scan|list|get <Paket>]'#10 +
        '  scan: Lädt die Paketlisten von den Servern'#10 +
        '  list: Zeigt alle verfügbaren Pakete an'#10 +
        '  get:  Installiert das angegebene Paket'#10 +
        '  install: Installiert das angegebene Paket aus lokaler Datei'#10 +
        '           Achtung, Datei als letzten Parameter angeben!';

    rsDownloadAndExtract = 'Herunterladen und Entpacken von %s';
    rsSearching = 'Suche %s';
    rsConfiguring = 'Richte %s ein';

    rsListInstallDependency = 'Installation hängt ab von: ';
    rsListRunDependency = 'Hängt ab von: ';

    rsError = 'Fehler:';
    rsCantLoadRepoList = 'Kann Repository-Liste nicht laden:';
    rsFailDownloadRepoList = 'Download der Paketliste für %s fehlgeschlagen:';
    rsAlreadyInstalled = 'Paket ist bereits installiert:';
    rsCannotReadPkglists = 'Kann die Paketlisten nicht einlesen:';
    rsCannotFindPackage = 'Konnte keine Version von %s finden';
    rsCannotResolveDep = 'Konnte die Abhängigkeiten des Pakets nicht auflösen:';
    rsWrongParamCount = 'Falsche Parameterzahl';
    rsInvalidAction = 'Ungütige Aktion:';


procedure PrintUsage;
begin
    WriteLn(rsUsage);
end;

procedure PrintSignifant;
begin
    writeln(' ________________________      ');
    writeln('|.----------------------.|     ');
    writeln('||         /  \~~~/  \  ||     ');
    writeln('||   ,----(     oo    ) ||     ');
    writeln('||  /      \__     __/  ||     ');
    writeln('|| /|         (\  |(    ||     ');
    writeln('||^ \   /___\  /\ |     ||     ');
    writeln('||   |__|   |__| ""     ||     ');
    writeln('|''----------------------''|   ');
    writeln(' ------------------------      ');
    writeln(' [ |-----|   tyndur   O ]      ');
    writeln(' [ |-----|            O ]      ');
    writeln('');
    writeln('Haben Sie heute schon signifantiert?');
    writeln('');
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
            WriteLn(rsCantLoadRepoList);
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
                WriteLn(Format(rsFailDownloadRepoList, [repo.name]));
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
        except
            on e: Exception do begin
                TextColor(12);
                WriteLn(rsError, ' ', e.message);
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
            WriteLn(rsAlreadyInstalled, ' ', version.pkg.name, '/',
                version.section.section);
            TextColor(7);
            Install := true;
            exit;
        end;

        repos := GetRepository(version.repository);

        // Ansonsten herunterladen und installieren
        TextColor(15);
        WriteLn(Format(rsDownloadAndExtract, [version.pkg.name + '/' +
            version.section.section]));
        TextColor(7);
        installarchive := repos.Download(version);
        if installarchive <> nil then begin
            Untar(installarchive);
        end;

        filename := 'file:/packages/' + version.pkg.name + '/' +
            version.version + '/postinstall-' + version.section.section;

        WriteLn(Format(rsSearching, [filename]));
        if FileExists(filename) then begin
            WriteLn(Format(rsConfiguring, [version.pkg.name + '/' +
                version.section.section]));
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
            WriteLn(rsCannotReadPkglists);
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
        WriteLn(format(rsCannotFindPackage, [pkgname]));
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
            WriteLn(rsCannotResolveDep);
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
                        WriteLn(rsError, ' ', e.message);
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
        for j := 0 to pkg.sections.count - 1 do begin
            sect := TPackageSection(pkg.sections.items[j]);
            for k := 0 to sect.versions.count - 1 do begin
                ver := TPackageVersion(sect.versions.items[k]);

                Write(pkg.name, StringOfChar(' ', 12 - Length(pkg.name)));
                Write(' | ', sect.section, StringOfChar(' ', 5 - Length(sect.section)));
                Write(' | ', ver.version : 10);
                if Length(pkg.desc) > 43 then begin
                    WriteLn(' | ' + Copy(pkg.desc, 1, 40), '...');
                end else begin
                    WriteLn(' | ' + pkg.desc);
                end;

                for l := 0 to ver.preinstDependencies.count - 1 do begin
                    dep := TDependency(ver.preinstDependencies.items[l]);
                    WriteLn('' : 36, rsListInstallDependency, dep.pkgname + ' (' + dep.section + ')');
                end;
                
                for l := 0 to ver.runDependencies.count - 1 do begin
                    dep := TDependency(ver.runDependencies.items[l]);
                    WriteLn('' : 36, rsListRunDependency, dep.pkgname + ' (' + dep.section + ')');
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

procedure SetLang;
var
    name: String;
    lang: TLanguage;
begin
    name := GetEnv('LANG');
    if name <> '' then begin
        lang := GetLanguage(name);
        if lang <> nil then begin
            SetLanguage(lang);
        end;
    end;
end;

begin
    SetLang;

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
            WriteLn(rsWrongParamCount);
            PrintUsage;
        end;
    end else if system.ParamStr(1) = 'reget' then begin
        if system.ParamCount = 2 then begin
            Get(system.ParamStr(2), true);
        end else begin
            WriteLn(rsWrongParamCount);
            PrintUsage;
        end;
    end else if system.ParamStr(1) = 'install' then begin
        if system.ParamCount = 3 then begin
            Install(system.ParamStr(3), system.ParamStr(2), false);
        end else begin
            WriteLn(rsWrongParamCount);
            PrintUsage;
        end;
    end else if system.ParamStr(1) = 'reinstall' then begin
        if system.ParamCount = 3 then begin
            Install(system.ParamStr(3), system.ParamStr(2), true);
        end else begin
            WriteLn(rsWrongParamCount);
            PrintUsage;
        end;
    end else if system.ParamStr(1) = 'cfg-addbin' then begin
        if system.ParamCount = 3 then begin
            CfgAddbin(system.ParamStr(2), system.ParamStr(3));
        end else begin
            WriteLn(rsWrongParamCount);
            PrintUsage;
        end;
    end else if system.ParamStr(1) = 'cfg-addlib' then begin
        if system.ParamCount = 3 then begin
            CfgAddlib(system.ParamStr(2), system.ParamStr(3));
        end else begin
            WriteLn(rsWrongParamCount);
            PrintUsage;
        end;
    end else if system.ParamStr(1) = 'cfg-addinc' then begin
        if system.ParamCount = 3 then begin
            CfgAddinc(system.ParamStr(2), system.ParamStr(3));
        end else begin
            WriteLn(rsWrongParamCount);
            PrintUsage;
        end;
    end else if system.ParamStr(1) = 'cfg-adddoc' then begin
        if system.ParamCount = 3 then begin
            CfgAdddoc(system.ParamStr(2), system.ParamStr(3));
        end else begin
            WriteLn(rsWrongParamCount);
            PrintUsage;
        end;
    end else if system.ParamStr(1) = 'moo' then begin
        PrintSignifant();
    end else begin
        WriteLn(rsInvalidAction, ' ', system.ParamStr(1));
        PrintUsage;
        halt(1);
    end;
end.
