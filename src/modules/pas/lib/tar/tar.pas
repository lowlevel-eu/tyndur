unit tar;
{$MODE ObjFPC}

interface

const
    TAR_BUFFER_SIZE  = 65536;

    TAR_TYPE_FILE      = '0';
    TAR_TYPE_HARDLINK  = '1';
    TAR_TYPE_SYMLINK   = '2';
    TAR_TYPE_DIRECTORY = '5';

type
    TTarArchive = class
        constructor create(filename: String);
        constructor create(buf: PByte; size: dword);

        destructor destroy; override;

        function HasNext: boolean;

        function NextFilename: String;
        function NextSize: dword;
        function NextFiletype: char;
        function NextLinkTarget: String;

        function ExtractString(silent: boolean = false): String;

        procedure ExtractFile(var outfile: file);
        procedure ExtractLink(basepath: String);
        procedure SkipFile;

        private
            procedure ReadNextBlock(buffer_offset: dword);
            function BufferIsZero: boolean;
            procedure FileDone;

        private
            f: file;
            in_buffer: PByte;
            in_size: dword;
            buffer: array [1..TAR_BUFFER_SIZE] of Byte;
            end_of_archive: boolean;
    end;

implementation

{$IF DEFINED(LINUX)}
uses baseunix;

procedure c_io_create_link(target, link: PChar; hardlink: boolean);
begin
    if hardlink then begin
        fpLink(target, link);
    end else begin
        fpSymlink(target, link);
    end;
end;
{$ELSEIF DEFINED(TYNDUR)}
procedure c_io_create_link(target, link: PChar; hardlink: boolean);
    cdecl; external name 'io_create_link';
{$ELSE}
{$ERROR OS not supported!}
{$ENDIF}

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

    ReadNextBlock(0);
end;

constructor TTarArchive.create(buf: PByte; size: dword);
begin
    in_buffer := buf;
    in_size := size;
    end_of_archive := false;

    ReadNextBlock(0);
end;

destructor TTarArchive.destroy;
begin
    Close(f);
end;

procedure TTarArchive.ReadNextBlock(buffer_offset: dword);
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
        Move(in_buffer^, buffer[1 + buffer_offset], block_size);
        Inc(in_buffer, block_size);
    end else begin
        BlockRead(f, buffer[1 + buffer_offset], 1);
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

function TTarArchive.NextLinkTarget: String;
var
    i: longint;
begin
    NextLinkTarget := PTarFileHeader(@buffer)^.link;

    for i := 1 to 100 do begin
        if NextLinkTarget[i] = #0 then begin
            SetLength(NextLinkTarget, i - 1);
            break;
        end;
    end;
end;

function TTarArchive.NextSize: dword;
begin
    NextSize := OctalToInt(PTarFileHeader(@buffer)^.size);
end;

function TTarArchive.NextFileType: char;
begin
    NextFileType := Chr(PTarFileHeader(@buffer)^.ftype);
    if NextFileType = #0 then begin
        NextFileType := TAR_TYPE_FILE;
    end;
end;

procedure TTarArchive.FileDone;
begin
    // Header der naechsten Datei einlesen
    ReadNextBlock(0);

    // Zwei Leerbloecke bedeuten das Ende der Datei
    if not end_of_archive and BufferIsZero() then begin
        ReadNextBlock(0);
        if not end_of_archive and BufferIsZero() then begin
            end_of_archive := true;
        end;
    end;
end;

function TTarArchive.ExtractString(silent: boolean = false): string;
var
    filename: String;
    size: dword;
    i: dword;
    buffer_pos: dword;
    temp, target: String;
begin
    size := NextSize;
    filename := NextFilename;

    if NextFiletype <> TAR_TYPE_FILE then begin
        exit('');
    end;

    target := '';

    if not silent then begin
        Write('[', filename, '] Entpacken...');
    end;
    for i := 1 to size div TAR_BUFFER_SIZE do begin
        buffer_pos := 0;
        repeat
            ReadNextBlock(buffer_pos);
            Inc(buffer_pos, 512);
        until buffer_pos = TAR_BUFFER_SIZE;

        SetString(temp, PChar(@buffer), TAR_BUFFER_SIZE);
        target := target + temp;
        if not silent then begin
            Write(#13, '[', filename, '] ', i * TAR_BUFFER_SIZE, '/', size, ' Bytes entpackt');
        end;
    end;

    if (size mod TAR_BUFFER_SIZE) > 0 then begin
        for i := 0 to ((size mod TAR_BUFFER_SIZE) div 512) do begin
            ReadNextBlock(i * 512);
        end;
        SetString(temp, PChar(@buffer), size);
        target := target + temp;
    end;
    if not silent then begin
        WriteLn(#13, '[', filename, '] ', size, '/', size, ' Bytes entpackt');
    end;

    FileDone;

    ExtractString := target;
end;

procedure TTarArchive.ExtractFile(var outfile: File);
var
    filename: String;
    size: dword;
    i: dword;
    buffer_pos: dword;
begin
    size := NextSize;
    filename := NextFilename;

    if NextFiletype <> TAR_TYPE_FILE then begin
        exit;
    end;

    Write('[', filename, '] Entpacken...');
    for i := 1 to size div TAR_BUFFER_SIZE do begin
        buffer_pos := 0;
        repeat
            ReadNextBlock(buffer_pos);
            Inc(buffer_pos, 512);
        until buffer_pos = TAR_BUFFER_SIZE;

        BlockWrite(outfile, buffer, TAR_BUFFER_SIZE);
        Write(#13, '[', filename, '] ', i * TAR_BUFFER_SIZE, '/', size, ' Bytes entpackt');
    end;

    if (size mod TAR_BUFFER_SIZE) > 0 then begin
        for i := 0 to ((size mod TAR_BUFFER_SIZE - 1) div 512) do begin
            ReadNextBlock(i * 512);
        end;
        BlockWrite(outfile, buffer, size mod TAR_BUFFER_SIZE);
    end;
    WriteLn(#13, '[', filename, '] ', size, '/', size, ' Bytes entpackt');

    FileDone;
end;

procedure TTarArchive.ExtractLink(basepath: String);
var
    target_fn, target_path: String;
    link_fn, link_path:     String;
    hard:                   boolean;
begin
    if not (NextFiletype in [TAR_TYPE_SYMLINK, TAR_TYPE_HARDLINK]) then begin
        exit;
    end;

    link_fn := NextFilename;
    link_path := basepath + '/' + link_fn + #0;

    target_fn := NextLinkTarget;
    target_path := basepath + '/' + target_fn + #0;

    hard := (NextFiletype = TAR_TYPE_HARDLINK);

    c_io_create_link(@target_path[1], @link_path[1], hard);
    WriteLn('[', link_fn, '] Link auf ' + target_fn + ' erstellt');

    FileDone;
end;

procedure TTarArchive.SkipFile;
var
    size: dword;
    i: dword;
begin
    size := NextSize;

    for i := 1 to (size + 511) div 512 do begin
        ReadNextBlock(0);
    end;

    FileDone;
end;

end.
