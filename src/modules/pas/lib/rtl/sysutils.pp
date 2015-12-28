unit sysutils;
{$MODE ObjFPC}
{$H+}

{$DEFINE FPC_FEXPAND_VOLUMES}
{$DEFINE FPC_FEXPAND_NO_DEFAULT_PATHS}

interface

(*
const
    fmOpenRead      = 1;
    fmOpenWrite     = 2;
    fmOpenReadWrite = fmOpenRead or fmOpenWrite;

function FileOpen(const filename: String; mode: Integer): LongInt;
procedure FileClose(Handle: LongInt);

function FileRead(Handle: LongInt;var Buffer;Count: LongInt) : LongInt;
function FileWrite(Handle: LongInt;const Buffer;Count: LongInt) : LongInt;
*)
function FileEof(Handle: Longint): boolean;
(*
function FileExists(const FileName: String) : Boolean;
function LastDelimiter(const Delimiters, S: string): Integer;

function IsLeapYear(year: word): boolean;
*)

{$I include/objpas/sysutils/sysutilh.inc}

implementation
uses sysconst, dos;

{$I include/objpas/sysutils/sysutils.inc}

type
    TSize = longint;

const
    IO_OPEN_MODE_READ = 1;
    IO_OPEN_MODE_WRITE = 2;
    IO_OPEN_MODE_APPEND = 4;
    IO_OPEN_MODE_TRUNC = 8;
    IO_OPEN_MODE_DIRECTORY = 16;
    IO_OPEN_MODE_CREATE = 32;
    IO_OPEN_MODE_LINK = 64;
    IO_OPEN_MODE_SYNC = 128;

const
    SEEK_SET = 0;
    SEEK_CUR = 1;
    SEEK_END = 2;

function lio_compat_open(filename: PChar; mode: byte): THandle; cdecl; external name 'lio_compat_open';
function lio_compat_close(io_res: THandle): integer; cdecl; external name 'lio_compat_close';

function lio_compat_read(dest: Pointer; blocksize, blockcount: TSize; io_res: THandle): TSSize; cdecl; external name 'lio_compat_read';
function lio_compat_readahead(dest: Pointer; size: TSize; io_res: THandle): TSSize; cdecl; external name 'lio_compat_readahead';
function lio_compat_write(src: Pointer; blocksize, blockcount: TSize; io_res: THandle): TSSize; cdecl; external name 'lio_compat_write';
function lio_compat_eof(io_res: THandle): integer; cdecl; external name 'lio_compat_eof';
function lio_compat_seek(io_res: THandle; offset: qword; origin: integer): boolean; cdecl; external name 'lio_compat_seek';
function lio_compat_tell(io_res: THandle): int64; cdecl; external name 'lio_compat_tell';

function c_init_execute(cmd: PChar): dword; cdecl; external name 'init_execute';
function c_waitpid(pid: integer; status: Pointer; flags: integer): integer; cdecl; external name 'waitpid';

function c_remove(filename: PChar): longint; cdecl; external name 'remove';

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


function FileOpen(const filename: String; mode: Integer): LongInt;
var
    cmode: byte;
    cfilename: String;
begin
    case mode of
        fmOpenRead:         cmode := IO_OPEN_MODE_READ;
        fmOpenWrite:        cmode := IO_OPEN_MODE_WRITE;
        fmOpenReadWrite:    cmode := IO_OPEN_MODE_READ or IO_OPEN_MODE_WRITE;
    end;

    if (cmode and IO_OPEN_MODE_WRITE) <> 0 then begin
        cmode := cmode or IO_OPEN_MODE_CREATE or IO_OPEN_MODE_TRUNC;
    end;

    cFileName := FileName + #0;
    FileOpen := lio_compat_open(@cFileName[1], cmode);

    if FileOpen = 0 then begin
        FileOpen := -1;
    end;
end;

procedure FileClose(Handle: LongInt);
begin
    (* lio_compat_close darf nicht -1 als Pointer erhalten *)
    if Handle = -1 then begin
        RunError(102);
    end;

    lio_compat_close(handle);
end;

function FileRead(Handle: LongInt;var Buffer;Count: LongInt) : LongInt;
begin
    FileRead := lio_compat_read(@buffer, 1, count, handle);
    if FileRead < 0 then begin
        FileRead := -1;
    end;
end;

function FileWrite(Handle: LongInt;const Buffer;Count: LongInt) : LongInt;
begin
    FileWrite := lio_compat_write(@buffer, 1, count, handle);
    if FileWrite < 0 then begin
        FileWrite := -1;
    end;
end;

function FileEof(Handle: Longint): boolean;
begin
    FileEof := lio_compat_eof(handle) <> 0;
end;

function FileExists(const FileName: String) : Boolean;
var
    handle: longint;
begin
    handle := FileOpen(FileName, fmOpenRead);
    FileExists := handle <> -1;
    if FileExists then begin
        FileClose(handle);
    end else begin
        FileExists := DirectoryExists(FileName);
    end;
end;


(*Function LastDelimiter(const Delimiters, S: string): Integer;
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
    
function IsLeapYear(year: word): boolean;
begin
    IsLeapYear := (year mod 4 = 0) and ((year mod 100 <> 0) or (year mod 400 = 0));
end;*)

function GetEnvironmentVariable(const envvar: AnsiString):AnsiString;
begin
    Result := ''; // FIXME
end;    

function GetEnvironmentVariableCount:LongInt;
begin
    Result := 0; // FIXME
end;    

function GetEnvironmentString(index: LongInt):AnsiString;
begin
    Result := ''; // FIXME
end;    

function ExecuteProcess(const path: AnsiString;const comline: AnsiString):LongInt;
var
    cmdline: String;
    ret, pid: integer;
    status: integer;
begin
    if comline <> '' then begin
        cmdline := path + ' ' + comline + #0;
    end else begin
        cmdline := path + #0;
    end;

    pid := c_init_execute(@cmdline[1]);
    ret := c_waitpid(pid, @status, 0);

    while (ret <> pid) and (ret <> -1) do begin
        yield;
        ret := c_waitpid(pid, @status, 0);
    end;

    if ret = -1 then begin
        raise EOSError.create('Konnte Programm ' + path + 'nicht ausführen');
    end;

    ExecuteProcess := status;
end;

function ExecuteProcess(const path: AnsiString;const comline: Array Of AnsiString):LongInt;
var
    args: String;
    i: integer;
begin
    if High(comline) >= Low(comline) then begin
        args := comline[Low(comline)];
        for i := Low(comline) + 1 to High(comline) do begin
            args := args + ' ' + comline[i];
        end;
    end else begin
        args := '';
    end;

    ExecuteProcess := ExecuteProcess(path, args);
end;

procedure GetLocalTime(var systemtime: TSystemTime);
begin
    // FIXME
end;

procedure Beep;
begin
    // FIXME
end;

function SysErrorMessage(errorcode: LongInt):AnsiString;
begin
    Result := ''; // FIXME
end;    

function FileCreate(const filename: AnsiString):LongInt;
begin
    Result := 0; // FIXME
end;    

function FileCreate(const filename: AnsiString; mode: LongInt):LongInt;
begin
    Result := 0; // FIXME
end;    

function FileSeek(handle: LongInt; foffset: Int64; origin: LongInt): Int64;
begin
    Result := 0; // FIXME
end;    

function FileSeek(handle: LongInt; foffset: LongInt; origin: LongInt):LongInt;
begin
    Result := 0; // FIXME
end;    

function FileSeek(handle: LongInt; foffset: Int64; origin: Int64):Int64;
begin
    Result := 0; // FIXME
end;    

function FileTruncate(handle: LongInt; size: Int64):Boolean;
begin
    Result := true; // FIXME
end;

function FileAge(const filename: AnsiString):LongInt;
begin
    if FileExists(filename) then begin
        // FIXME
        Result := DateTimeToFileDate(EncodeDate(1980, 1, 1));
    end else begin
        Result := -1;
    end;
end;

function DirectoryExists(const directory: AnsiString):Boolean;
var
    handle: Pointer;
begin
    handle := c_directory_open(PChar(directory));
    DirectoryExists := (handle <> nil);
    if DirectoryExists then begin
        c_directory_close(handle);
    end;
end;

function FindFirst(const path: AnsiString; attr: LongInt;out rslt: TSearchRec):LongInt;
var
    f: ^SearchRec;
begin
    New(f);
    rslt.FindHandle := THandle(f);
    dos.FindFirst(path, attr, f^);

    rslt.name := f^.name;
    rslt.size := f^.size;
    rslt.attr := f^.attr;
    rslt.time := f^.time;

    exit(DosError);
end;

function FindNext(var rslt: TSearchRec):LongInt;
var
    f: ^SearchRec;
begin
    f := Pointer(rslt.FindHandle);
    dos.FindNext(f^);

    rslt.name := f^.name;
    rslt.size := f^.size;
    rslt.attr := f^.attr;
    rslt.time := f^.time;

    exit(DosError);
end;

procedure FindClose(var f: TSearchRec);
var
    srec: ^SearchRec;
begin
    srec := Pointer(f.FindHandle);
    dos.FindClose(srec^);
    Dispose(srec);
end;

function FileGetDate(handle: LongInt):LongInt;
begin
    Result := 0; // FIXME
end;    

function FileSetDate(handle: LongInt; age: LongInt):LongInt;
begin
    Result := 0; // FIXME
end;    

function FileGetAttr(const filename: AnsiString):LongInt;
begin
    Result := 0; // FIXME
end;    

function FileSetAttr(const filename: AnsiString; attr: LongInt):LongInt;
begin
    Result := 0; // FIXME
end;    

function DeleteFile(const filename: AnsiString):Boolean;
var
    c_filename: String;
begin
    c_filename := filename + #0;
    Result := (c_remove(@c_filename[1]) = 0);
end;

function RenameFile(const oldname: AnsiString;const newname: AnsiString):Boolean;
begin
    Result := true; // FIXME
end;

function DiskFree(drive: Byte):Int64;
begin
    Result := 0; // FIXME
end;    

function DiskSize(drive: Byte):Int64;
begin
    Result := 0; // FIXME
end;    

function GetCurrentDir:AnsiString;
begin
    Result := ''; // FIXME
end;    

function SetCurrentDir(const newdir: AnsiString):Boolean;
begin
    Result := true; // FIXME
end;

function CreateDir(const newdir: AnsiString):Boolean;
begin
    Result := true; // FIXME
end;

function RemoveDir(const dir: AnsiString):Boolean;
begin
    Result := true; // FIXME
end;

procedure InitCaseTables;
var
    c: char;
begin
    for c := #0 to #255 do begin
        UpperCaseTable[Ord(c)] := c;
        LowerCaseTable[Ord(c)] := c;
    end;

    for c := 'A' to 'Z' do begin
        LowerCaseTable[Ord(c)] := Chr(Ord(c) - Ord('A') + Ord('a'));
    end;
    for c := 'a' to 'z' do begin
        LowerCaseTable[Ord(c)] := Chr(Ord(c) - Ord('a') + Ord('A'));
    end;
end;

begin
    InitCaseTables;
    InitExceptions;
end.
