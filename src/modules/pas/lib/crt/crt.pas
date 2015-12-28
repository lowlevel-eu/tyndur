unit crt;

interface

procedure CursorOn;
procedure CursorOff;

procedure ClrScr;
procedure GotoXY(x, y: integer);
procedure TextColor(c: byte);
procedure TextBackground(c: byte);

function WhereX: word;
function WhereY: word;

function keypressed: boolean;
function readkey: char;
function nextkey: char;

procedure Delay(ms: dword);

implementation
uses sysutils;

function lio_compat_readahead(dest: Pointer; size: TSize; io_res: THandle): TSSize; cdecl; external name 'lio_compat_readahead';
const
    EAGAIN = 18;

const
  TextRecNameLength = 256;
  TextRecBufSize    = 256;

type
  TLineEndStr = string [3];
  TextBuf = array[0..TextRecBufSize-1] of char;
  TextRec = Packed Record
    Handle    : THandle;
    Mode      : longint;
    bufsize   : SizeInt;
    _private  : SizeInt;
    bufpos,
    bufend    : SizeInt;
    bufptr    : ^textbuf;
    openfunc,
    inoutfunc,
    flushfunc,
    closefunc : pointer;
    UserData  : array[1..32] of byte;
    name      : array[0..textrecnamelength-1] of char;
    LineEnd   : TLineEndStr;
    buffer    : textbuf;
  End;
    
    FileFunc = Procedure(var t : TextRec);

var
    curTextColor: byte;
    curTextBackground: byte;

    oldStdin: Text;

procedure CursorOn;
begin end;

procedure CursorOff;
begin end;

procedure ClrScr;
begin
    Write(#27'[H'#27'[2J');
end;

procedure GotoXY(x, y: integer);
begin
    Write(#27'[', y, ';', x, 'H');
end;

procedure TextColor(c: byte);
var
    bold: boolean;
begin
    c := c and 15;
    bold := (c and 8) <> 0;

    if c <> curTextColor then begin
        curTextColor := c;
        c := (c and 2) or ((c and 4) shr 2) or ((c and 1) shl 2);

        if bold then begin
            Write(#27'[1;', 30 + (c mod 8), 'm');
        end else begin
            Write(#27'[0;', 30 + (c mod 8), 'm');
            Write(#27'[', 40 + (curTextBackground mod 8), 'm');
        end;
    end;
end;

procedure TextBackground(c: byte);
begin
    c := (c and 2) or ((c and 4) shr 2) or ((c and 1) shl 2);

    if c <> curTextBackground then begin
        Write(#27'[', 40 + (c mod 8), 'm');
        curTextBackground := c;
    end;
end;

function WhereX: word;
begin
    // FIXME WhereX
    WhereX := 0;
end;

function WhereY: word;
begin
    // FIXME WhereY
    WhereY := 0;
end;

var
    lastKey: char;

function keypressed: boolean;
var
    handle: THandle;
    ret: integer;
    c: char;
begin
    if lastKey = #26 then begin
        handle := TextRec(oldStdin).Handle;
        ret := lio_compat_readahead(@c, 1, handle);
        if ret = -EAGAIN then begin
            lastkey := #26;
        end else begin
            FileRead(handle, lastKey, 1);
        end;
    end;

    keypressed := lastKey <> #26;
end;

function nextkey: char;
begin
    if keypressed then begin
        nextkey := lastkey;
    end else begin
        nextkey := #26;
    end;
end;

function readkey: char;
var
    c: char;
begin
    Flush(output);

    if lastKey = #26 then begin
        repeat
            Read(oldStdin, c);
        until c <> #26;
    end else begin
        c := lastKey;
        lastKey := #26;
    end;
    exit(c);
end;

procedure ReadInput;
var 
    c: char;
    i: integer;
begin
    with TextRec(input) do begin
        bufpos := 0;
        bufend := 0;

        repeat
            c := readkey;

            case c of
                #8: begin
                    Dec(bufend);
                    Write(#27'[1D '#27'[1D');
                end;

                else begin
                    bufptr^[bufend] := c;
                    Inc(bufend);
                    Write(c);
                end;
            end;
        until (c = #10) or (bufend = bufsize);
    end;
end;


procedure c_msleep(ms: dword); cdecl; external name 'msleep';

procedure Delay(ms: dword);
begin
    c_msleep(ms);
end;
    
begin
    curTextBackground := 0;
    curTextColor := 7;
        
    lastKey := #26;
 
    oldStdin := input;
    TextRec(oldStdin).BufPtr := @TextRec(oldStdin).Buffer;
    
    TextRec(input).InOutFunc := @ReadInput;
end.
