unit ircwindow;
{$MODE ObjFPC}

interface

type
    TIRCCommandHandler = procedure(const s: String);

    TIRCWindow = class
        private
            fTitle: String;
            fCommandHandler: TIRCCommandHandler;

            outbuffer: string;
            posy: byte;

            procedure ScrollLines(num: integer);
            procedure SetTitle(title: String);

        public 
            constructor Init(title: String);
            procedure Draw;
            procedure Incoming(s: String; color: byte);
            procedure CheckInput;

            property title: String read fTitle write SetTitle;
            property commandHandler: TIRCCommandHandler 
                read fCommandHandler write fCommandHandler;
    end;

implementation
uses crt;

constructor TIRCWindow.Init(title: String);
begin
    posy := 0;
    fTitle := title;
    fCommandHandler := nil;
end;

procedure TIRCWindow.Draw;
begin
    GotoXY(1,1);
    TextColor(14);
    TextBackground(1);
    Write('kIRC: ' + fTitle : 80);

    GotoXY(1, 24);
    Write(StringOfChar(' ' , 80));
    GotoXY(1, 24);
    TextColor(7);
    Write(outBuffer);
    GotoXY(1, 24);
end;
            
procedure TIRCWindow.setTitle(title: String);
begin
    fTitle := title;
    Draw;
end;

procedure TIRCWindow.ScrollLines(num: integer);
var
    i: integer;
begin
    TextBackground(0);
    GotoXY(1, 24);
    for i := 1 to num do begin
        Write(StringOfChar(' ' , 80));
    end;
    WriteLn;
    Draw;
end;

procedure TIRCWindow.Incoming(s: String; color: byte);
var
    lines: integer;
begin
    lines := (length(s) + 79) div 80;
    if lines = 0 then begin
        lines := 1;
    end;

    GotoXY(1, 2 + posy);
    if posy + lines > 22 then begin
        ScrollLines(posy + lines - 22);
        Dec(posy, posy + lines - 22);
        GotoXY(1, 2 + posy);
    end;

    TextBackground(0);
    TextColor(color);
    Write(s);
    Inc(posy, lines);
    GotoXY(length(outBuffer) + 1, 24);
end;

procedure TIRCWindow.CheckInput;
var
    c: char;
begin
    if not keypressed then begin
        exit;
    end;

    c := readkey;
    case c of
        #8:     if Length(outBuffer) > 0 then begin
                    if length(outBuffer) < 80 then begin
                        GotoXY(length(outBuffer), 24);
                        Write(' ');
                        SetLength(outBuffer, Length(outBuffer) - 1);
                        GotoXY(length(outBuffer) + 1, 24);
                    end else begin
                        SetLength(outBuffer, Length(outBuffer) - 1);
                        GotoXY(1, 24);
                        Write(Copy(outBuffer, length(outBuffer) - 78, 79));
                    end;
                end;
        #10:    ;
        else    begin
                    TextColor(7);
                    TextBackground(1);
                    outBuffer := outBuffer + c;
                   
                    if length(outBuffer) < 80 then begin
                        GotoXY(length(outBuffer), 24);
                        Write(c);
                    end else begin
                        GotoXY(1, 24);
                        Write(Copy(outBuffer, length(outBuffer) - 78, 79));
                    end;
                end;
    end;

    if c = #10 then begin
        if fCommandHandler <> nil then begin
            fCommandHandler(outBuffer);
    
            TextColor(7);
            TextBackground(1);
            GotoXY(1, 24);
            Write(StringOfChar(' ', 80));
            GotoXY(1, 24);
            outBuffer := '';
        end;
    end;
end;



end.
