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
        handle:     Pointer;
        pattern:    String;

        time:   longint;
        size:   longint;
        attr:   byte;
        name:   String;
    end;

    DateTime = record
        year:   word;
        month:  word;
        day:    word;
        hour:   word;
        min:    word;
        sec:    word;
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


Function GetEnv (EnvVar: String) : String;
function EnvCount: longint;
function EnvStr(index: longint): String;

procedure Exec(const path: PathStr; const args: ComStr);

Procedure GetDate (var year, month, mday, wday: word);
Procedure GetTime (var hour, minute, second, sec100: word);
procedure UnPackTime (p: longint; var T: datetime);
Procedure PackTime (var T: datetime; var P: longint);

Procedure SwapVectors;

implementation

type
    DirEntry = record
        name:   array [1..32] of char;
        ftype:  byte;
        size:   qword;

        ctime, 
        mtime, 
        atime:  qword;
    end;
    PDirEntry = ^DirEntry;


function c_directory_open(dirname: PChar): Pointer; cdecl; external name 'directory_open';
function c_directory_close(handle: Pointer): longint; cdecl; external name 'directory_close';
function c_directory_read(handle: Pointer): PDirEntry; cdecl; external name 'directory_read';

function c_init_execute(cmd: PChar): dword; cdecl; external name 'init_execute';
function c_waitpid(pid: integer; status: Pointer; flags: integer): integer; cdecl; external name 'waitpid';

function c_getenv(const name: PChar): PChar; cdecl; external name 'getenv';

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


Function FilenameMatches(const filename: String; pattern: String): boolean;
var
    i_pattern: integer;
    i_file: integer;
    
    subname: String;
    j: integer;
begin
    i_pattern := 0;
    i_file := 1;

    // Zeichenweise das Pattern durchgehen und pruefen, ob das Zeichen
    // uebereinstimmt
    while i_pattern < length(pattern) do begin
        Inc(i_pattern);

        // Falls wir mit dem Dateinamen schon am Ende sind, duerfen im
        // Pattern ab jetzt nur noch beliebig viele * kommen
        if i_file > length(filename) then begin
            while (i_pattern <= length(pattern)) 
                and (pattern[i_pattern] = '*') do 
            begin
                Inc(i_pattern);
            end;
            exit(i_pattern = length(pattern) + 1);
        end;

        case pattern[i_pattern] of
            '?':
                // Ein ? ist leicht zu parsen: Passt, naechstes Zeichen
                begin                
                    Inc(i_file);
                end;

            '*':
                // Ein * dagegen ist etwas schwieriger zu pruefen. Der Trick
                // besteht darin, auszuprobieren, ob der Rest passen wuerde,
                // wenn der * das naechste, die naechsten zwei, usw. Zeichen
                // des Dateinamens abdeckt. Die Pruefung, ob der Rest passt,
                // wird durch rekursiven Aufruf von FilenameMatches erledigt.
                begin
                    // Wenn mehrere * aufeinander folgen, koennen bis auf
                    // einen alle ignoriert werden
                    while (i_pattern <= length(pattern)) 
                        and (pattern[i_pattern] = '*') do 
                    begin
                        Inc(i_pattern);
                    end;
                    
                    // Jede moegliche Ersetzung fuer das * durchprobieren, 
                    // angefangen beim leeren String bis zur Ersetzung des
                    // kompletten restlichen Dateinamens
                    pattern := Copy(pattern, i_pattern, length(pattern));
                    for j := i_file to length(filename) + 1 do begin                        
                        subname := Copy(filename, j, length(filename));
                        if FilenameMatches(subname, pattern) then begin
                            exit(true);
                        end;
                    end;

                    exit(false);
                end;

            else
                // Ansonsten sind wir im trivialen Fall: Das Zeichen in 
                // Dateiname und Pattern muss uebereinstimmen
                begin
                    if pattern[i_pattern] <> filename[i_file] then begin
                        exit(false);
                    end;
                    Inc(i_file);
                end;
        end;
    end;
    
    // Wenn wir hier landen, ist das Pattern abgearbeitet. An dieser Stelle
    // muss dann auch der Dateiname zu Ende sein.
    exit(i_file = length(filename) + 1);
end;


Procedure FindFirst (const Path: pathstr; Attr: word; var F: SearchRec);
var
    dir, name, ext: String;
begin
    FSplit(path, dir, name, ext);

    if length(dir) = 0 then begin
        dir := #0;
    end else if dir[length(dir) - 1] = ':' then begin
        dir := dir + #0;
    end else begin
        // FIXME LostIO sollte mit / am Ende eines Dateinamens zurechtkommen
        dir[length(dir)] := #0;
    end;

    f.handle := c_directory_open(@dir[1]);
    f.pattern := name + ext;
    FindNext(f);
end;

Procedure FindNext (var f: searchRec);
var
    entry: PDirEntry;
begin
    if f.handle = nil then begin
        DosError := 1;
        exit;
    end;

    // TODO Filtern
    // TODO Andere Attribute als Name fuellen
    DosError := 0;
    repeat
        entry := c_directory_read(f.handle);
        if entry <> nil then begin
            // Der Dateiname muss aus einem C-char[] erst einmal in einen
            // ordentlichen String umgewandelt werden
            f.name := entry^.name;
            if (Pos(#0, f.name) > 0) then begin
                SetLength(f.name, Pos(#0, f.name) - 1);
            end;

            // Anschliessend kann geprueft werden, ob der Datei ueberhaupt
            // zum Suchmuster passt
            if FilenameMatches(f.name, f.pattern) then begin
                f.time := 0; // TODO Dateidatum
                break;
            end;
        end else begin
            DosError := 1;
            break;
        end;
    until false;
end;

Procedure FindClose (Var F: SearchRec);
begin
    c_directory_close(f.handle);
    f.handle := nil;
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
        dir := Copy(path, 1, index);
        path := Copy(path, index + 1, Length(path));
    end else begin
        dir := '';
    end;
    
    index := LastDelimiter('.', path);
    if index > 0 then begin
        name:= Copy(path, 1, index - 1);
        ext:= Copy(path, index, Length(path));
    end else begin
        name := path;
        ext := '';
    end;
end;

Function GetEnv (EnvVar: String) : String;
var
    res: PChar;
begin
    EnvVar := EnvVar + #0;
    res := c_getenv(@EnvVar[1]);
    if res <> nil then begin
        GetEnv := StrPas(res);
    end else begin
        GetEnv := '';
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

procedure Exec(const path: PathStr; const args: ComStr);
var
    cmdline: String;
    ret, pid: integer;
    status: integer;
begin
    cmdline := path + ' ' + args + #0;

    pid := c_init_execute(@cmdline[1]);
    ret := c_waitpid(pid, @status, 0);
    while (ret <> pid) and (ret <> -1) do begin
        yield;
        ret := c_waitpid(pid, @status, 0);
    end;
end;

Procedure GetDate (var year, month, mday, wday: word);
begin
    year := 1337; // FIXME
    month := 0;    
end;

Procedure GetTime (var hour, minute, second, sec100: word);
begin
    // FIXME    
    hour := 25;
    minute := 0;
    second := 0;
    sec100 := 0;
end;

procedure UnPackTime (p: longint; var T: datetime);
begin
    // FIXME UnPackTime
    with t do begin
        year := 2020;
        month := 1;
        day := 23;
        hour := 4;
        min := 45;
        sec := 67;
    end;
end;

Procedure PackTime (var T: datetime; var P: longint);
begin
    // FIXME PackTime
end;

Procedure SwapVectors;
begin
    // Bleibt für LOST unimplementiert
end;

end.
