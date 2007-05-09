unit crt;

interface

procedure CursorOn;
procedure CursorOff;

procedure ClrScr;
procedure GotoXY(x, y: integer);
procedure TextColor(c: byte);
procedure TextBackground(c: byte);

function readkey: char;

implementation

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
    Write(#27'[2J');
end;

procedure GotoXY(x, y: integer);
begin
    Write(#27'[', (x - 1), ';', (y - 1), 'H');
end;

procedure TextColor(c: byte);
begin
    c := (c and 2) or ((c and 4) shr 2) or ((c and 1) shl 2);

    if c <> curTextColor then begin
        Write(#27'[', 30 + (c mod 8), 'm');
        curTextColor := c;
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

function readkey: char;
var
    c: char;
begin
    Flush(output);
    repeat
        Read(oldStdin, c);
    until c <> #26;
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
    
begin
    curTextBackground := 0;
    curTextColor := 7;
 
    oldStdin := input;
    TextRec(oldStdin).BufPtr := @TextRec(oldStdin).Buffer;
    
    TextRec(input).InOutFunc := @ReadInput;
end.
