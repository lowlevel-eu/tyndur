unit sysutils;

interface

const
    fmOpenRead      = 1;
    fmOpenWrite     = 2;
    fmOpenReadWrite = fmOpenRead or fmOpenWrite;

function FileOpen(const filename: String; mode: Integer): LongInt;
procedure FileClose(Handle: LongInt);

function FileRead(Handle: LongInt;var Buffer;Count: LongInt) : LongInt;
function FileWrite(Handle: LongInt;const Buffer;Count: LongInt) : LongInt;
function FileEof(Handle: Longint): boolean;

function FileExists(const FileName: String) : Boolean;
function LastDelimiter(const Delimiters, S: string): Integer;

function IsLeapYear(year: word): boolean;

implementation

type
    TSize = longint;

const
    SEEK_SET = 0;
    SEEK_CUR = 1;
    SEEK_END = 2;

function c_fopen(filename, mode: PChar): longint; cdecl; external name 'fopen';
function c_fclose(f: longint): integer; cdecl; external name 'fclose';

function c_fwrite(src: Pointer; blocksize, blockcount: TSize; f: longint): TSize; cdecl; external name 'fwrite';
function c_fread(dest: Pointer; blocksize, blockcount: TSize; f: longint): TSize; cdecl; external name 'fread';
function c_fseek(f: longint; offset, whence: longint): longint; cdecl; external name 'fseek';

function c_feof(f: longint): boolean; cdecl; external name 'feof';


function FileOpen(const filename: String; mode: Integer): LongInt;
var
    cmode: String;
    cfilename: String;
begin
    case mode of
        fmOpenRead:         cmode := 'r'#0;
        fmOpenWrite:        cmode := 'w'#0;
        fmOpenReadWrite:    cmode := 'rw'#0;
    end;

    cFileName := FileName + #0;
    FileOpen := c_fopen(@cFileName[1], @cmode[1]);

    if FileOpen = 0 then begin
        FileOpen := -1;
    end;
end;

procedure FileClose(Handle: LongInt);
begin
    c_fclose(handle);
end;

function FileRead(Handle: LongInt;var Buffer;Count: LongInt) : LongInt;
begin
    FileRead := c_fread(@buffer, 1, count, handle);
end;

function FileWrite(Handle: LongInt;const Buffer;Count: LongInt) : LongInt;
begin
    FileWrite := c_fwrite(@buffer, 1, count, handle);
end;

function FileEof(Handle: Longint): boolean;
begin
    FileEof := c_feof(handle);
end;

function FileExists(const FileName: String) : Boolean;
var
    handle: longint;
begin
    handle := FileOpen(FileName, fmOpenRead);
    FileExists := handle <> -1;
    if FileExists then begin
        FileClose(handle);
    end;
end;


Function LastDelimiter(const Delimiters, S: string): Integer;
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
end;

end.
