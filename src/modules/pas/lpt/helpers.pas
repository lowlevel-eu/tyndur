unit helpers;
{$MODE ObjFPC}

interface
uses crt, sysutils, dos, classes, tar, packages, tcpip, http, multilang;

procedure ScanFile(pkgset: TPackageSet; filename, repository: String);
procedure DrawProgress(http: HTTPRequest);
function StartDownload(url: String): HTTPRequest;
function DownloadToFile(url, filename: String): boolean;
function DownloadToTarArchive(url: String): TTarArchive;
function getConfigRoot: String;
procedure Untar(tarfile: String; targetpath: String = 'file:/');
procedure Untar(tar: TTarArchive; targetpath: String = 'file:/');
function getArch: String;
procedure mkpath(path: string);

implementation

resourcestring
    rsBytesReceived = '%d Bytes empfangen';
    rsBytesReceivedPartial = '%d/%d Bytes empfangen';

    rsDownloading = 'Herunterladen von %s';
    rsDownloadSummary = '%d %[0:Byte:Bytes] in %ds heruntergeladen';
    rsConnecting = 'Verbinde...';
    rsHTTPError = 'Fehler: HTTP-Statuscode';

procedure mkpath(path: string);
var
    i: integer;
    prefix: String;
begin
    i := 1;
    while i <= length(path) do begin
        if path[i] = '/' then begin
            prefix := Copy(path, 1, i);
            if not FileExists(prefix) then begin
                mkdir(prefix);
            end;
        end;
        Inc(i);
    end;
end;

function getConfigRoot: String;
begin
    exit('file:/config/lpt/');
end;

function getArch: String;
begin
    exit('i386');
end;

procedure DrawProgress(http: HTTPRequest);
const
    i: Integer = 0;
begin
    if http.responseRemaining > 0 then begin
        Inc(i);
        if i mod 16 <> 0 then begin
            exit;
        end;

        Write(#13, FormatNum(rsBytesReceivedPartial,
            [http.responseLength - http.responseRemaining,
            http.responseLength]));
    end else begin
        Write(#13, FormatNum(rsBytesReceived, [http.responseLength]));
    end;
end;

function StartDownload(url: String): HTTPRequest;
var
    httpc: HTTPRequest;
begin
    httpc := HTTPRequest.create(url);
    httpc.setRequestHeader('Pragma', 'no-cache');
    httpc.setRequestHeader('Cache-Control', 'no-cache');
    httpc.setRequestHeader('User-Agent', 'lpt');
    WriteLn(Format(rsDownloading, [url]));
    Write(rsConnecting);
    httpc.onReadTick := @DrawProgress;
    httpc.get;
    WriteLn;

    if httpc.statusCode = 200 then begin
        exit(httpc);
    end else begin
        WriteLn(rsHTTPError, ' ', httpc.statusCode);
        exit(nil)
    end;
end;

function DownloadToFile(url, filename: String): boolean;
var
    f: File;
    httpc: HTTPRequest;
    tar: TTarArchive;
    i: integer;
begin
    DownloadToFile := false;
    httpc := StartDownload(url);
    if httpc <> nil then begin
        Assign(f, filename);
        Rewrite(f, httpc.responseLength);
        BlockWrite(f, httpc.binaryResponseBody^, 1);
        Close(f);
        DownloadToFile := true;
        httpc.done;
    end;
end;

function GetTickCount: Int64; cdecl; external name 'get_tick_count';

function DownloadToTarArchive(url: String): TTarArchive;
var
    f: File;
    httpc: HTTPRequest;
    tar: TTarArchive;
    i: integer;

    start_time, end_time: Int64;
begin
    DownloadToTarArchive := nil;

    start_time := GetTickCount;
    httpc := StartDownload(url);
    end_time := GetTickCount;

    if httpc <> nil then begin
        WriteLn(FormatNum(rsDownloadSummary, [httpc.responseLength,
            ((end_time - start_time) div 1000000)]));
        tar := TTarArchive.create(httpc.binaryResponseBody, httpc.responseLength);
        DownloadToTarArchive := tar;
    end;
end;

procedure ScanFile(pkgset: TPackageSet; filename, repository: String);
var
    f: Text;
    line: String;
    status: integer;

    pkg: TPackage;
    section: TPackageSection;
    version: TPackageVersion;
begin
    Assign(f, filename);
    Reset(f);
    status := 0;
    pkg := nil;
    try
        while not eof(f) do begin
            ReadLn(f, line);

            if line = '' then begin
                continue;
            end;

            case line[1] of
                'P':
                    begin
                        status := 1;
                        pkg := pkgset.add(Copy(line, 3, length(line)));
                    end;
                'D':
                    if status = 1 then begin
                        pkg.desc := Copy(line, 3, length(line));
                    end else begin
                        raise EPackageFileFormat.create(
                            'D in status ' + IntToStr(status));
                    end;
                'S':
                    if status >= 1 then begin
                        status := 2;
                        section := pkg.add(Copy(line, 3, length(line)));
                    end else begin
                        raise EPackageFileFormat.create(
                            'S in status ' + IntToStr(status));
                    end;
                'V':
                    if status >= 2 then begin
                        status := 3;
                        version := section.add(Copy(line, 3, length(line)));
                        version.pkg := pkg;
                        version.section := section;
                        version.repository := repository;
                    end else begin
                        raise EPackageFileFormat.create(
                            'V in status ' + IntToStr(status));
                    end;
                's':
                    if status >= 3 then begin
                        version.size := StrToInt(Copy(line, 3, length(line)));
                    end else begin
                        raise EPackageFileFormat.create(
                            's in status ' + IntToStr(status));
                    end;
                'd':
                    if status >= 3 then begin
                        if length(line) < 2 then begin
                            raise EPackageFileFormat.create(
                                'Einsames d');
                        end;

                        version.addDependency(line[2], Copy(line, 4, length(line)));

                    end else begin
                        raise EPackageFileFormat.create(
                            'd in status ' + IntToStr(status));
                    end;
                else
                    begin
                        raise EPackageFileFormat.create(
                            'Unbekannter Schluessel: ' + line[1]);
                    end;
            end;
        end;
    finally
        Close(f);
    end;
end;

procedure Untar(tar: TTarArchive; targetpath: String = 'file:/');
var
    path: String;
    dir, filename, ext: String;

    f: file;
    filetype: char;
begin
    while tar.hasNext do begin
        path := tar.NextFilename;
        dos.FSplit(path, dir, filename, ext);

        mkpath(targetpath + '/' + dir);

        filetype := tar.NextFiletype;
        if filetype = TAR_TYPE_FILE then begin
            Assign(f, targetpath + '/' + path);
            Rewrite(f, 1);
            tar.ExtractFile(f);
            Close(f);
        end else if filetype in [TAR_TYPE_HARDLINK, TAR_TYPE_SYMLINK] then begin
            tar.ExtractLink(targetpath);
        end else begin
            tar.SkipFile;
        end;
    end;
end;

procedure Untar(tarfile: String; targetpath: String = 'file:/');
var
    tar: TTarArchive;
begin
    tar := TTarArchive.create(tarfile);
    Untar(tar, targetpath);
end;

begin
end.
