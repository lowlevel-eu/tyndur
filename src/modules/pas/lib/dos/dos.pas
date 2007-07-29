unit dos;

interface

const
    readonly    = $01;
    hidden      = $02;
    sysfile     = $04;
    volumeid    = $08;
    directory   = $10;
    archive     = $20;
    anyfile     = $3F;

    DosVersion  = 0;

type
    ComStr  = ShortString;
    PathStr = ShortString;
    DirStr  = ShortString;
    NameStr = ShortString;
    ExtStr  = ShortString;

    SearchRec = packed record
        handle: dword;
        time:   longint;
        size:   longint;
        attr:   byte;
        name:   String;
    end;

var
    DosError: integer;
    DosExitCode: integer;

Function FSearch (Path: pathstr; DirList: string) : pathstr;
Function FExpand (const path: pathstr) : pathstr;
procedure FSplit(path: PathStr; var dir: DirStr; var name: NameStr; var ext: ExtStr);

Procedure FindFirst (const Path: pathstr; Attr: word; var F: SearchRec);
Procedure FindNext (var f: searchRec);
Procedure FindClose (Var F: SearchRec);

procedure GetFAttr (var F; var Attr: word);
procedure SetFAttr (var F; Attr: word);

procedure GetFTime(var F; var time: longint);
procedure SetFTime(var F; time: longint);


function EnvCount: longint;
function EnvStr(index: longint): String;

implementation

type
    DirEntry = record
        name:   String[32];
        ftype:  byte;
        size:   qword;

        ctime, 
        mtime, 
        atime:  qword;
    end;
    PDirEntry = ^DirEntry;


function c_directory_open(dirname: PChar): dword; cdecl; external name 'directory_open';
function c_directory_close(handle: dword): longint; cdecl; external name 'directory_close';
function c_directory_read(handle: dword): PDirEntry; cdecl; external name 'directory_close';

function LastDelimiter(const Delimiters, S: string): Integer;
var
    i, j: longint;
begin
    for i := length(s) downto 1 do begin
        for j := length(Delimiters) downto 1 do begin
            if s[i] = Delimiters[j] then begin
                exit(i);
            end;
        end;
    end;

    exit(0);
end;

procedure GetFAttr (var F; var Attr: word);
begin
    // TODO Wie prüft man das mit LostIO? Verzeichnis wäre wichtig.
    Attr := 0;
end;

procedure SetFAttr (var F; Attr: word);
begin
    // TODO SetFAttr halbwegs umsetzen
end;

procedure GetFTime(var F; var time: longint);
begin
    // FIXME GetFTime
    time := 0;
end;

procedure SetFTime(var F; time: longint);
begin
    // FIXME SetFTime
end;

Function FExpand (const path: pathstr) : pathstr;
begin
    // FIXME FExpand
    FExpand := path;
end;

Procedure FindFirst (const Path: pathstr; Attr: word; var F: SearchRec);
var
    dir, name, ext: String;
begin
    FSplit(path, dir, name, ext);

    dir := dir + #0; 
    f.handle := c_directory_open(@dir[1]);
    FindNext(f);
end;

Procedure FindNext (var f: searchRec);
var
    entry: PDirEntry;
begin
    // TODO Filtern
    // TODO Andere Attribute als Name fuellen
    DosError := 0;
    entry := c_directory_read(f.handle);
    if entry <> nil then begin
        f.name := entry^.name;
    end else begin
        DosError := 1;
    end;
end;

Procedure FindClose (Var F: SearchRec);
begin
    c_directory_close(f.handle);
    f.handle := 0;
end;

Function FSearch (Path: pathstr; DirList: string) : pathstr;
var
    f: SearchRec;
begin
    // FIXME DirList ist eine Liste
    // FIXME Path ist ein Muster, nicht nur ein Dateiname
    FindFirst(dirlist, 0, f);

    while DosError = 0 do begin
        if f.name = path then begin
            FSearch := FExpand(dirlist + '/' + f.name);
            exit;
        end;
        FindNext(f);
    end;

    FSearch := '';
end;



procedure FSplit(path: PathStr; var dir: DirStr; var name: NameStr; var ext: ExtStr);
var
    index: integer;
begin
    index := LastDelimiter('/', path);
    if index > 0 then begin
        dir := Copy(path, 1, index - 1);
        path := Copy(path, index + 1, Length(path));
    end else begin
        dir := '';
    end;
    
    index := LastDelimiter('.', path);
    if index > 0 then begin
        name:= Copy(path, 1, index - 1);
        ext:= Copy(path, index + 1, Length(path));
    end else begin
        name := path;
        ext := '';
    end;
end;

function EnvCount: longint;
begin
    // FIXME EnvCount
    EnvCount := 0;
end;

function EnvStr(index: longint): String;
begin
    // FIXME EnvStr
    EnvStr := '';
end;

end.
