const
    SEEK_SET = 0;
    SEEK_CUR = 1;
    SEEK_END = 2;

function c_fopen(filename, mode: PChar): THandle; cdecl; external name 'fopen';
function c_fclose(f: THandle): integer; cdecl; external name 'fclose';

function c_fwrite(src: Pointer; blocksize, blockcount: TSize; f: THandle): TSize; cdecl; external name 'fwrite';
function c_fread(dest: Pointer; blocksize, blockcount: TSize; f: THandle): TSize; cdecl; external name 'fread';
function c_fseek(f: THandle; offset, whence: longint): longint; cdecl; external name 'fseek';
function c_ftell(f: THandle): TSize; cdecl; external name 'ftell';

function c_feof(f: THandle): boolean; cdecl; external name 'feof';

function c_setvbuf(f: THandle; buf: Pointer; mode: integer; size: TSize):
    integer; cdecl; external name 'setvbuf';


{ TODO errno setzen nicht vergessen }

Procedure Do_Close(f: THandle);
begin
    c_fclose(f);
end;

Procedure Do_Erase(p:pchar);
begin { TODO } end;

{ truncate at a given position }
procedure do_truncate (handle:thandle;fpos:longint);
begin { TODO } end;

Procedure Do_Rename(p1,p2:pchar);
begin { TODO } end;


Function Do_Write(f: THandle; src: Pointer; len: TSize): TSize;
begin 
    Do_Write := c_fwrite(src, 1, len, f);
end;

Function Do_Read(f: THandle; dest: Pointer; len: TSize): TSize;
begin 
    repeat
        Do_Read := c_fread(dest, 1, len, f);
    until (Do_Read <> 0) or (c_feof(f));
end;

function Do_FilePos(f: THandle): TSize;
begin
    Do_FilePos := c_ftell(f);
end;

Procedure Do_Seek(f: THandle; position: TSize);
begin
    c_fseek(f, position, SEEK_SET);
end;

Function Do_SeekEnd(f: THandle): TSize;
begin
    Do_SeekEnd := c_fseek(f, 0, SEEK_END);
end;

Function Do_FileSize(f: THandle): Longint;
var
    curpos: TSize;
begin
    curpos := Do_FilePos(f);
    Do_SeekEnd(f);
    Do_FileSize := Do_FilePos(f);
    Do_Seek(f, curpos);
end;

Procedure Do_Open(var f; name: PChar; flags: longint);
var
    c_mode: string[3];
    append: boolean;
    trunc: boolean;
    frec: ^FileRec;
begin
    frec := @FileRec(f);

    // Wenn eine Datei noch geoeffnet ist, muss sie zuerst
    // geschlossen werden.
    if (frec^.mode = fminput) or (frec^.mode = fmoutput) or (frec^.mode = fminout) then begin
        Do_Close(frec^.handle);
    end;
    
    append := (flags and $100 <> 0);
    trunc := (flags and $1000 <> 0);

    if name <> '' then begin

        // Eine normale Datei
        case flags and 3 of
            0:  begin
                    c_mode := 'r'#0;
                    frec^.mode := fmInput;
                end;

            1:  begin
                    if append then begin
                        c_mode := 'a'#0 
                    end else begin 
                        c_mode := 'w'#0;
                    end;
                    frec^.mode := fmOutput;
                end;
            2:  begin 
                    if append then begin
                        c_mode := 'a+'#0 
                    end else if trunc then begin
                        c_mode := 'w+'#0 
                    end else begin
                        c_mode := 'r+'#0;
                    end;
                    frec^.mode := fmInOut;
                end;
        end;

        frec^.handle := c_fopen(name, @c_mode[1]);
        if frec^.handle = 0 then begin
            InOutRes := 2;
        end else begin
            c_setvbuf(frec^.handle, nil, 1, 0);
        end;

    end else begin

        // stdin/out
        case frec^.mode of
            fminput:    frec^.handle := StdInputHandle;
            else        frec^.handle := StdOutputHandle;
        end;

    end;
end;
