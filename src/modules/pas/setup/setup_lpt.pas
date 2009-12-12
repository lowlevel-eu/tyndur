unit setup_lpt;

interface

procedure SetupLPT(back: boolean);

implementation

uses sysutils, dos, tyndur, crt, tui;

resourcestring
    rsTitle = 'tyndur-Konfiguration: Quelle für lpt';
    rsAnyKey = 'Bitte eine beliebige Taste drücken';

    rsChoose = 'Bitte wähle aus dem Menü die Quelle für lpt aus.';
    rsStable = 'Stabile Version (gothmog)';
    rsCurrent = 'Entwicklerversion (current)';

    rsBack = 'Zurück';

procedure SetRepo(repo: String);
var
    cfg: text;
begin
    Assign(cfg, 'file:/config/lpt/pkgsrc');
    Rewrite(cfg);
    WriteLn(cfg, 'lpt ', repo);
    Close(cfg);

    TextBackground(0);
    TextColor(7);
    ClrScr;
    Exec('lpt', 'scan');

    WriteLn;
    WriteLn(rsAnyKey);
    readkey;
end;

procedure SetupLPT(back: boolean);
var
    mainw: TUIRowContainer;

    a: TUIMenu;
    b: TUILabel;

    buttons: TUIColContainer;
    btnOk: TUIButton;
    btnCancel: TUIButton;
    screen: TUIRowContainer;
    title: TUILabel;
    f: TUIFrame;
    num_items: integer;
begin
    screen := TUIRowContainer.create(2);
    screen.bgcolor := 2;
    screen.color := 1;

    title := TUILabel.create;
    title.content := Utf8Decode(rsTitle);
    title.bgcolor := 1;
    title.color := 15;
    screen[0] := title;
    screen.fixed[0] := 1;

    f := TUIFrame.create(nil, 60, 12);
    screen[1] := f;
    screen.prop[1] := 1;

    mainw := TUIRowContainer.create(2);
    mainw.spacer := true;
    f.obj := mainw;

    if back then begin
        num_items := 4;
    end else begin
        num_items := 2;
    end;

    a := TUIMenu.create(num_items);
    with a do begin
        bgcolor := 0;
        color := 14;

        AddItem(rsStable, nil);
        AddItem(rsCurrent, nil);
        if back then begin
            AddItem('', nil);
            AddItem(Utf8Decode(rsBack), nil);
        end;
    end;
    mainw[1] := a;
    mainw.fixed[1] := num_items;

    b := TUILabel.create;
    b.content := Utf8Decode(rsChoose);
    mainw[0] := b;
    mainw.prop[0] := 30;

    a.selected := 0;
    repeat
        // Zeichnen
        screen.align(80, 24);
        screen.draw(1, 1);

        case readkey of
            #27:
                case ReadEscapeSequence of
                    SK_UP:
                        begin
                            a.selected := (a.selected + num_items - 1) mod num_items;
                            if a.selected = 2 then begin
                                a.selected := 1;
                            end;
                        end;
                    SK_DOWN:
                        begin
                            a.selected := (a.selected + 1) mod num_items;
                            if a.selected = 2 then begin
                                a.selected := 3;
                            end;
                        end;
                end;
            #10:
                case a.selected of
                    0:
                        begin
                            SetRepo('http://lpt.tyndur.org/gothmog/');
                            break;
                        end;
                    1:
                        begin
                            SetRepo('http://lpt.tyndur.org/current/');
                            break;
                        end;
                    3:  break;
                end;
        end;
    until false;

    f.obj := nil;
    mainw.free;
    screen.free;
end;

end.
