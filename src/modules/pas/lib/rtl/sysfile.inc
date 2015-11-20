const
    SEEK_SET = 0;
    SEEK_CUR = 1;
    SEEK_END = 2;

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
    EAGAIN = 18;

function lio_compat_open(filename: PChar; mode: byte): THandle; cdecl; external name 'lio_compat_open';
function lio_compat_close(io_res: THandle): integer; cdecl; external name 'lio_compat_close';

function lio_compat_read(dest: Pointer; blocksize, blockcount: TSize; io_res: THandle): TSSize; cdecl; external name 'lio_compat_read';
function lio_compat_readahead(dest: Pointer; size: TSize; io_res: THandle): TSSize; cdecl; external name 'lio_compat_readahead';
function lio_compat_write(src: Pointer; blocksize, blockcount: TSize; io_res: THandle): TSSize; cdecl; external name 'lio_compat_write';
function lio_compat_eof(io_res: THandle): integer; cdecl; external name 'lio_compat_eof';
function lio_compat_seek(io_res: THandle; offset: qword; origin: integer): boolean; cdecl; external name 'lio_compat_seek';
function lio_compat_tell(io_res: THandle): int64; cdecl; external name 'lio_compat_tell';

{ TODO errno setzen nicht vergessen }

Procedure Do_Close(f: THandle);
begin
    lio_compat_close(f);
end;

Procedure Do_Erase(p:pchar);
begin { TODO } end;

{ truncate at a given position }
procedure do_truncate (handle:thandle;fpos:longint);
begin { TODO } end;

Procedure Do_Rename(p1,p2:pchar);
begin { TODO } end;


Function Do_Write(f: THandle; src: Pointer; len: TSize): TSize;
var
    ret: TSSize;
begin
    ret := lio_compat_write(src, 1, len, f);
    if ret < 0 then begin
        InOutRes := 101;
        Do_Write := 0;
    end else begin
        InOutRes := 0;
        Do_Write := ret;
    end;
end;

Function Do_Read(f: THandle; dest: Pointer; len: TSize): TSize;
var
    ret: TSSize;
begin
    InOutRes := 0;

    ret := lio_compat_read(dest, 1, len, f);
    if ret < 0 then begin
        InOutRes := 101;
        exit(0);
    end else begin
        Do_Read := ret;
    end;
end;

function Do_FilePos(f: THandle): TSize;
var
    ret: int64;
begin
    InOutRes := 0;
    ret := lio_compat_tell(f);
    if ret < 0 then begin
        InOutRes := 101;
        exit(0);
    end else begin
        exit(ret);
    end;
end;

Procedure Do_Seek(f: THandle; position: TSize);
var
    ret: boolean;
begin
    InOutRes := 0;
    ret := lio_compat_seek(f, position, SEEK_SET);
    if not ret then begin
        InOutRes := 101;
    end;
end;

Function Do_SeekEnd(f: THandle): TSize;
var
    ret: boolean;
begin
    InOutRes := 0;
    ret := lio_compat_seek(f, 0, SEEK_END);
    if not ret then begin
        InOutRes := 101;
        exit(0);
    end else begin
        exit(Do_FilePos(f));
    end;
end;

Function Do_FileSize(f: THandle): Longint;
var
    curpos: TSize;
begin
    InOutRes := 0;
    curpos := Do_FilePos(f);
    Do_SeekEnd(f);
    Do_FileSize := Do_FilePos(f);
    Do_Seek(f, curpos);
end;

Procedure Do_Open(var f; name: PChar; flags: longint);
var
    c_mode: byte;
    autoclose: boolean;
    append: boolean;
    trunc: boolean;
    frec: ^FileRec;
begin
    InOutRes := 0;
    frec := @FileRec(f);

    append := (flags and $100 <> 0);
    trunc := (flags and $1000 <> 0);
    autoclose := (flags and $10000 <> 0);

    // Wenn eine Datei noch geoeffnet ist, muss sie zuerst
    // geschlossen werden.
    if autoclose
       and ((frec^.mode = fminput) or (frec^.mode = fmoutput) or (frec^.mode = fminout))
       and (frec^.handle <> 0)
    then begin
        Do_Close(frec^.handle);
    end;

    if name <> '' then begin

        // Eine normale Datei
        case flags and 3 of
            0:  begin
                    c_mode := IO_OPEN_MODE_READ;
                    frec^.mode := fmInput;
                end;

            1:  begin
                    c_mode := IO_OPEN_MODE_WRITE;
                    if append then begin
                        c_mode := c_mode or IO_OPEN_MODE_APPEND;
                    end else if trunc then begin
                        c_mode := c_mode or IO_OPEN_MODE_CREATE or IO_OPEN_MODE_TRUNC;
                    end;
                    frec^.mode := fmOutput;
                end;
            2:  begin
                    if append then begin
                        c_mode := IO_OPEN_MODE_READ or IO_OPEN_MODE_WRITE
                               or IO_OPEN_MODE_CREATE or IO_OPEN_MODE_APPEND;
                    end else if trunc then begin
                        c_mode := IO_OPEN_MODE_READ or IO_OPEN_MODE_WRITE
                               or IO_OPEN_MODE_CREATE or IO_OPEN_MODE_TRUNC;
                    end else begin
                        c_mode := IO_OPEN_MODE_READ or IO_OPEN_MODE_WRITE;
                    end;
                    frec^.mode := fmInOut;
                end;
        end;

        frec^.handle := lio_compat_open(name, c_mode);

        if frec^.handle = 0 then begin
            InOutRes := 2;
            frec^.mode := fmClosed;
        end;

    end else begin

        // stdin/out
        case frec^.mode of
            fminput:    frec^.handle := StdInputHandle;
            else        frec^.handle := StdOutputHandle;
        end;

    end;
end;
