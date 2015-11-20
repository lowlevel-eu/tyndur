unit setup_keyboard;

{$MODE ObjFPC}

interface

procedure SetupKeyboard(back: boolean);

implementation

uses sysutils, tyndur, crt, tui;

resourcestring
    rsTitle = 'tyndur-Konfiguration: Tastaturbelegung';

    rsGerman = 'Deutsch (de)';
    rsSwissGerman = 'Schweizerdeutsch (de_ch)';
    rsUSAmerican = 'US-Amerikanisch (us)';
    rsBack = 'Zurück';

    rsChooseLayout = 'Bitte wähle aus dem Menü Deine Tastaturbelegung aus.';

    rsFailReadKeymap = 'Konnte Tastaturbelegung nicht einlesen';
    rsFailApplyKeymap = 'Konnte Tastaturbelegung nicht anwenden';

procedure SetLayout(layout: String);
var
    f: File;
    buf: Array [1..2048] of byte;
    path: String;

    cfg: text;
begin
    path := 'file:/system/keymaps/' + layout + '.kbd';

    try
        Assign(f, path);
        filemode := fmOpenRead;
        Reset(f, 2048);
        BlockRead(f, buf, 1);
        Close(f);
    except
        on e: EInOutError do begin
            WriteLn;
            WriteLn(rsFailReadKeymap);
            {$I-} Close(f); IOResult; {$I+}
            readkey;
            exit;
        end;
    end;

    try
        Assign(f, 'vterm:/keymap');
        Rewrite(f, 2048);
        BlockWrite(f, buf, 1);
        Close(f);
    except
        on e: EInOutError do begin
            WriteLn;
            WriteLn(rsFailApplyKeymap);
            {$I-} Close(f); IOResult; {$I+}
            readkey;
            exit;
        end;
    end;

    try
        Assign(cfg, 'file:/config/keyboard.lsh');
        Rewrite(cfg);
        WriteLn(cfg, '#!file:/apps/sh');
        WriteLn(cfg);
        WriteLn(cfg, 'cp ', path, ' vterm:/keymap');
        Close(cfg);
    except
        on e: EInOutError do begin
            {$I-} Close(cfg); IOResult; {$I+}
            exit;
        end;
    end;
end;

procedure SetupKeyboard(back: boolean);
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
    title.content := rsTitle;
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
        num_items := 5;
    end else begin
        num_items := 3;
    end;

    a := TUIMenu.create(num_items);
    with a do begin
        bgcolor := 0;
        color := 14;

        AddItem(rsGerman, nil);
        AddItem(rsSwissGerman, nil);
        AddItem(rsUSAmerican, nil);
        if back then begin
            AddItem('', nil);
            AddItem(Utf8Decode(rsBack), nil);
        end;
    end;
    mainw[1] := a;
    mainw.fixed[1] := num_items;

    b := TUILabel.create;
    b.content := Utf8Decode(rsChooseLayout);
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
                            if a.selected = 3 then begin
                                a.selected := 2;
                            end;
                        end;
                    SK_DOWN:
                        begin
                            a.selected := (a.selected + 1) mod num_items;
                            if a.selected = 3 then begin
                                a.selected := 4;
                            end;
                        end;
                end;
            #10:
                case a.selected of
                    0:
                        begin
                            SetLayout('de');
                            break;
                        end;
                    1:
                        begin
                            SetLayout('de_ch');
                            break;
                        end;
                    2:
                        begin
                            SetLayout('us');
                            break;
                        end;
                    4:  break;
                end;
        end;
    until false;

    f.obj := nil;
    mainw.free;
    screen.free;
end;

end.
