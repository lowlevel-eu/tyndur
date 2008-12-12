unit kedit_buf;

interface

uses kedit_main;

procedure DoScrolling(state: pkedit_state; var redrawEditor: boolean);
procedure DeleteLine(state: pkedit_state; var redrawEditor: boolean);
procedure InsertLine(state: pkedit_state; var redrawEditor: boolean);


implementation

uses sysutils, kedit_tui;

procedure DoScrolling(state: pkedit_state; var redrawEditor: boolean);
begin
    with state^ do begin
        if cursX <= scrollX then begin
            scrollX := cursX - 1;
            redrawEditor := true;
        end;

        if cursX > scrollX + TextCols then begin
            scrollX := cursX - TextCols;
            redrawEditor := true;
        end;

        if cursY <= scrollY then begin
            scrollY := cursY - 1;
            redrawEditor := true;
        end;

        if cursY > scrollY + TextLines then begin
            scrollY := cursY - TextLines;
            redrawEditor := true;
        end;
    end;
end;

procedure DeleteLine(state: pkedit_state; var redrawEditor: boolean);
var
    i: longint;
begin
    with state^ do begin
        for i := cursY to MaxLines-1 do begin
            text^[i] := text^[i+1];
        end;
        text^[MaxLines] := '';

        redrawEditor := true;
    end;
end;

procedure InsertLine(state: pkedit_state; var redrawEditor: boolean);
var
    i: longint;
begin
    with state^ do begin
        if cursy = MaxLines then begin
            Beep;
            exit;
        end;

        for i := MaxLines downto cursy+1 do begin
            text^[i] := text^[i-1];
        end;

        text^[cursY + 1] := Copy(text^[cursy], cursx, 255);
        text^[cursY]     := Copy(text^[cursy], 1, cursx-1);

        redrawEditor := true;
    end;
end;


end.
