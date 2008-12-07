unit kedit_main;

interface

const
    MaxLines = 32768;
    MaxColumns = 255;

type
    type_text = Array [1..MaxLines] of AnsiString;

    // Beschreibt einen offenen Puffer
    kedit_state = record
        filename:   String;

        cursX:      longint;
        cursY:      longint;
        scrollX:    longint;
        scrollY:    longint;

        replace:    boolean;
        readonly:   boolean;

        text:       ^type_text;
    end;
    pkedit_state = ^kedit_state;

var
    changed:            boolean; // FIXME Gehoert in kedit_state
    highlight:          boolean;
    exitEditor:         boolean;
    redrawEditor:       boolean;

procedure SaveFile(state: pkedit_state);
procedure LoadFile(state: pkedit_state);
procedure LoadFile(state: pkedit_state; loadname: string; create: boolean);
procedure RemoveRO(state: pkedit_state);

function RTrim(stext: string):string;


implementation

uses crt, dos, sysutils, kedit_tui;



function RTrim(stext: string):string;
begin
    repeat
        if stext[Length(stext)] = ' ' then begin
            Delete(stext, Length(stext), 1);
        end;
    until (Length(stext) = 0) or (stext[Length(stext)] <> ' ');
    RTrim := stext;
end;




procedure SaveFile(state: pkedit_state);
var
    savename: string;
    i, j: longint;
    sfile: textfile;
    emptylines: longint;
    attr: word;
begin
    with state^ do begin
        TextBackground(1);

        // Dateinamen abfragen
        if filename='(unbenannt)' then begin
            GotoXY(1,1);
            Write('Dateiname:'+Space(69));
            GotoXY(12,1);
            ReadLn(savename);
        end else begin
            GotoXY(1,1);
            Write(space(79));
            GotoXY(1,1);
            Write('Dateiname [',filename,']: ');
            ReadLn(savename);
            if savename='' then begin
                savename := filename;
            end;
        end;

        Textbackground(0);

        // Ueberpruefen, ob die Datei schon existiert und ggf.
        // schreibgeschuetzt ist
        if FileExists(savename) then begin
            GetFAttr(sfile, attr);

            Textbackground(1);
            GotoXY(1,1);
            Write(space(79));
            GotoXY(1,1);

            if attr and dos.ReadOnly = dos.Readonly then begin
                Write('Die Datei ',savename,' existiert und ist schreibgeschützt. [ENTER]');
                Textbackground(0);
                readkey;
                exit;
            end else begin
                case YesNoCancel('Die Datei ' + savename + ' gibt''s schon. Überschreiben?', false) of
                    OPT_YES:    {Do nothing};
                    OPT_NO:     exit;
                end;
            end;
        end;

        // Datei schreiben
        filename := savename;
        Assign(sfile, savename);
        Rewrite(sfile);
        emptylines := 0;

        // FIXME Leerzeilen am Dateiende
        for i := 1 to MaxLines do begin
            if text^[i]='' then begin
                Inc(emptylines)
            end else begin
                for j := 1 to emptylines do begin
                    WriteLn(sfile);
                end;
                WriteLn(sfile,text^[i]);
                emptylines:=0;
            end;
        end;
        Close(sfile);
    end;

    changed := false;
end;

procedure LoadFile(state: pkedit_state; loadname: string; create: boolean);
var lfile: textfile;
    cnt: longint;
    i: integer;
    attr: word;
begin
    if loadname = '' then begin
        Textbackground(1);
        gotoxy(1,1); Write(space(79));
        gotoxy(1,1); Write('Zu ladende Datei: '); ReadLn(loadname);
        Textbackground(0);
    end;

    if not FileExists(loadname) then begin
        if create then begin
            state^.filename := loadname;
            exit;
        end else begin
            gotoxy(1,1); Write(space(79));
            gotoxy(1,1); Textbackground(4); Write('Datei ', loadname, ' nicht gefunden.'); Textbackground(0);
            readkey;
            gotoxy(1,1); Write(space(79));
            exit;
        end;
    end;

    with state^ do begin
        Assign(lfile,loadname);
        GetFAttr(lfile, attr);
        if attr and dos.ReadOnly = dos.ReadOnly then readonly := true;
        Reset(lfile);

        for i := 1 to MaxLines do begin
            text^[i] := '';
        end;

        cnt := 0;
        while not(eof(lfile)) do begin
            Inc(cnt);
            ReadLn(lfile, text^[cnt]);
        end;

        Close(lfile);
        filename := loadname;
    end;

    gotoxy(1,1);
    Write(space(79));
end;

procedure LoadFile(state: pkedit_state);
begin
    LoadFile(state, '', false);
end;

procedure RemoveRO(state: pkedit_state);
begin
    ShowCursor(false);
    FillRectangle(19,9,61,15);
    DrawDblFrame(19,9,61,15);
    GotoXY(21,10); TextColor(15); Write('Schreibschutz entfernen'); TextColor(7);
    GotoXY(21,11); Write('Wollen Sie den Schreibschutz wirklich');
    GotoXY(21,12); Write('aufheben? Die Datei muss dann unter');
    GotoXY(21,13); Write('einem anderen Namen gespeichert werden.');
    GotoXY(38,14); Write('[J/N]');

    repeat
        case readkey of
            'n','N':
                break;

            'j','J':
                begin
                    state^.filename := '(unbenannt)';
                    state^.readonly := false;
                    break;
                end;
        end;
    until false;

    ShowCursor(true);
    redrawEditor := true;
end;

end.
