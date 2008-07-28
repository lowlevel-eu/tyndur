unit tar;
{$MODE ObjFPC}

interface

type
    TTarArchive = class
        constructor create(filename: String);
        constructor create(buf: PByte; size: dword);

        destructor destroy; override;

        function HasNext: boolean;
        function NextFilename: String;
        function NextSize: dword;

        procedure ExtractFile(var outfile: file);
        procedure SkipFile;

        private
            procedure ReadNextBlock;
            function BufferIsZero: boolean;

            f: file;
            in_buffer: PByte;
            in_size: dword;
            buffer: array [1..512] of Byte;
            end_of_archive: boolean;
    end;

implementation

type
    TTarFileHeader = packed record
        name:   array [1..100] of char;
        mode:   qword;
        uid:    qword;
        gid:    qword;
        size:   array [1..12] of char;
        time:   array [1..12] of char;
        cksum:  qword;
        ftype:  byte;
        link:   array [1..100] of char;

        ustar_magic:    array [1..6] of char;
        ustar_ver:      word;
        username:       array [1..32] of char;
        groupname:      array [1..32] of char;
        dev_minor:      qword;
        dev_major:      qword;
        name_prefix:    array [1..155] of char;
    end;
    PTarFileHeader = ^TTarFileHeader;


function OctalToInt(oct: String): dword;
var
    i: longint;
begin
    Result := 0;
    for i := length(oct) downto 1 do begin
        Inc(Result, (Ord(oct[i]) - 48) * (1 shl (3 * (length(oct) - i - 1))));
    end;
end;


constructor TTarArchive.create(filename: String);
begin
    in_buffer := nil;
    Assign(f, filename);
    Reset(f, 512);
    end_of_archive := false;

    ReadNextBlock();
end;

constructor TTarArchive.create(buf: PByte; size: dword);
begin
    in_buffer := buf;
    in_size := size;
    end_of_archive := false;

    ReadNextBlock();
end;

destructor TTarArchive.destroy;
begin
    Close(f);
end;

procedure TTarArchive.ReadNextBlock;
var
    block_size: dword;
begin
    if in_buffer <> nil then begin
        block_size := in_size;
        if block_size > 512 then begin
            block_size := 512;
            Dec(in_size, 512);
        end else begin
            in_size := 0;
            end_of_archive := true;
            FillByte((in_buffer + block_size)^, 512 - block_size, 0);
        end;
        Move(in_buffer^, buffer, block_size);
        Inc(in_buffer, block_size);
    end else begin
        BlockRead(f, buffer, 1);
    end;
end;

function TTarArchive.BufferIsZero: boolean;
var
    i: longint;
begin
    for i := 1 to 512 do begin
        if PByte(@buffer[i])^ <> 0 then begin
            exit(false);
        end;
    end;
    exit(true);
end;

function TTarArchive.HasNext: boolean;
begin
    HasNext := not end_of_archive;
end;

function TTarArchive.NextFilename: String;
var
    i: longint;
begin
    NextFilename := PTarFileHeader(@buffer)^.name;

    for i := 1 to 100 do begin
        if NextFilename[i] = #0 then begin
            SetLength(NextFilename, i - 1);
            break;
        end;
    end;
end;

function TTarArchive.NextSize: dword;
begin
    NextSize := OctalToInt(PTarFileHeader(@buffer)^.size);
end;

procedure TTarArchive.ExtractFile(var outfile: File);
var
    filename: String;
    size: dword;
    i: dword;
begin
    size := NextSize;
    filename := NextFilename;

    Write('[', filename, '] Entpacken...');
    for i := 1 to size div 512 do begin
        ReadNextBlock;
        BlockWrite(outfile, buffer, 512);
        if i mod 16 = 0 then begin
            Write(#13, '[', filename, '] ', i*512, '/', size, ' Bytes entpackt');
        end;
    end;

    if (size mod 512) > 0 then begin
        ReadNextBlock;
        BlockWrite(outfile, buffer, size mod 512);
    end;
    WriteLn(#13, '[', filename, '] ', size, '/', size, ' Bytes entpackt');

    ReadNextBlock;

    if not end_of_archive and BufferIsZero() then begin
        ReadNextBlock;
        if not end_of_archive and BufferIsZero() then begin
            end_of_archive := true;
        end;
    end;
end;

procedure TTarArchive.SkipFile;
var
    size: dword;
    i: dword;
begin
    size := NextSize;

    for i := 1 to (size + 511) div 512 do begin
        ReadNextBlock;
    end;

    ReadNextBlock;
end;

end.
