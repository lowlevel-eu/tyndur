unit ircwindow;
{$MODE ObjFPC}

interface

const
    BUFFER_LINES = 30;

type
    TIRCCommandHandler = procedure(const s: String);

    TIRCWindow = class
        private
            fTitle: String;
            fCommandHandler: TIRCCommandHandler;

            outBuffer: string;
            posy: byte;

            inBuffer: array [0..BUFFER_LINES - 1] of record
                s: String;
                color: byte;
            end;

            inBufferPos: integer;

            procedure ScrollLines(num: integer);
            procedure SetTitle(title: String);

            function AutoCompleteOutBuffer(): String;

        public 
            constructor Init(title: String);
            procedure Draw(content: boolean);
            procedure Incoming(s: String; color: byte);
            procedure CheckInput;

            property title: String read fTitle write SetTitle;
            property commandHandler: TIRCCommandHandler 
                read fCommandHandler write fCommandHandler;
    end;

implementation
uses sysutils, crt;

constructor TIRCWindow.Init(title: String);
var
    i: integer;
begin
    posy := 0;
    fTitle := title;
    fCommandHandler := nil;

    inBufferPos := 0;
    for i := 0 to BUFFER_LINES - 1 do begin
        inBuffer[i].s := '';
        inBuffer[i].color := 7;
    end;
end;

procedure TIRCWindow.Draw(content: boolean);
var
    firstLine, lastLine: integer;
    i, curY: integer;
begin
    if content then begin
        TextBackground(0);
        ClrScr;
    end;

    GotoXY(1,1);
    TextColor(14);
    TextBackground(1);
    Write('kIRC: ' + fTitle : 80);

    if content then begin

        if inBufferPos = 0 then begin
            firstLine := BUFFER_LINES - 1;
        end else begin
            firstLine := inBufferPos - 1;
        end;

        lastLine := inBufferPos;
        curY := 0;
        while curY < 22 do begin
            Inc(curY, (length(inBuffer[firstLine].s) + 79) div 80);
            Dec(firstLine);

            if (firstLine < 0) then begin
                firstLine := BUFFER_LINES - 1;
            end;

            if inBuffer[firstLine].s = '' then begin
                break;
            end;
        end;
        firstLine := (firstLine + 1) mod BUFFER_LINES;

        GotoXY(1,1);
        Write(firstLine, ' - ', lastLine);

        GotoXY(1,2);
        TextBackground(0);
        
        TextColor(inBuffer[firstLine].color);
        Write(Copy(inBuffer[firstLine].s, 80 * (curY - 22), length(inBuffer[firstLine].s)));
        curY := 2 + (length(inBuffer[firstLine].s) + 79) div 80;
        
        i := (firstLine + 1) mod BUFFER_LINES;
        while i <> lastLine do begin
            GotoXY(1, curY);
            //if inBuffer[i].s <> '' then begin
                TextColor(inBuffer[i].color);
                Write(inBuffer[i].s);
                Inc(curY, (length(inBuffer[i].s) + 79) div 80);
            //end;
            i := (i + 1) mod BUFFER_LINES;
        end;

    end;

    TextBackground(1);
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
    Draw(false);
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
    Draw(false);
end;

procedure TIRCWindow.Incoming(s: String; color: byte);
var
    lines: integer;
begin
    lines := (length(s) + 79) div 80;
    if lines = 0 then begin
        lines := 1;
    end;

    inBuffer[inBufferPos].s := s;
    inBuffer[inBufferPos].color := color;

    inBufferPos := (inBufferPos + 1) mod BUFFER_LINES;
    inBuffer[inBufferPos].s := '';

    (*
    Draw(true);
    *)
    
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
    autoCompletion, curToken: String;
begin
    if not keypressed then begin
        exit;
    end;

    TextColor(7);
    TextBackground(1);

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
        #9:     begin
                    autoCompletion := AutoCompleteOutBuffer();
                    if autoCompletion <> '' then begin
                        outBuffer := outBuffer + autoCompletion  + ' ';
                        GotoXY(1, 24);
                        Write(Copy(outBuffer, length(outBuffer) - 78, 79));
                    end;
                end;
        #10:    ;
        else    begin
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
                        
    GotoXY(1, 25);
    TextBackground(0);
    TextColor(8);
    
    curToken := Copy(outBuffer, LastDelimiter(' ', outBuffer) + 1, length(outBuffer));
    if length(curToken) > 2 then begin
        autoCompletion := AutoCompleteOutBuffer;
        if autoCompletion <> '' then begin
            autoCompletion := curToken + autoCompletion; 
        end else begin
            autoCompletion := '';
        end;
    end else begin
         autoCompletion := '';
    end;
    Write(Copy(autoCompletion, 1, 79), StringOfChar(' ', 79 - length(autoCompletion)));
    
    TextBackground(1);
    TextColor(7);
    GotoXY(length(outBuffer) + 1, 24);
end;


function TIRCWindow.AutoCompleteOutBuffer(): String;
var
    completionToken, curToken: String;
    spacePos: integer;
    curInBufferLine: integer;

    i: integer;
begin
    spacePos := LastDelimiter(' ', outBuffer);
    completionToken := Copy(outBuffer, spacePos + 1, length(outBuffer));

    curInBufferLine := inBufferPos;
    AutoCompleteOutBuffer := '';
    repeat
        curToken := '';
        for i := 1 to length(inBuffer[curInBufferLine].s) do begin
            if inBuffer[curInBufferLine].s[i] in ['A' .. 'Z', 'a' .. 'z', '0' .. '9'] then begin
                curToken := curToken + inBuffer[curInBufferLine].s[i];
            end else begin
                // TODO CompareText benutzen
                if Copy(curToken, 1, length(completionToken)) = completionToken then begin
                    // TODO: Bei mehreren Treffern gemeinsamen Anfang suchen
                    AutoCompleteOutBuffer := Copy(curToken, length(completionToken) + 1, length(curToken));
                end;
                curToken := '';
            end;
        end;
                
        // TODO CompareText benutzen
        if Copy(curToken, 1, length(completionToken)) = completionToken then begin
            // TODO: Bei mehreren Treffern gemeinsamen Anfang suchen
            AutoCompleteOutBuffer := Copy(curToken, length(completionToken) + 1, length(curToken));
        end;

        curInBufferLine := (curInBufferLine + 1) mod BUFFER_LINES;
    until curInBufferLine = inBufferPos;
end;

end.
