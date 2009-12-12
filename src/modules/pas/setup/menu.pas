unit menu;

interface
uses tui;

procedure MainMenu(screen: TUIObject; frame: TUIFrame);
function StartModule(module: String; back: boolean): boolean;

implementation

uses tyndur, crt, setup_keyboard, setup_lpt;

resourcestring
    rsChooseModule = 'Bitte wähle aus dem Menü das gewünschte Konfigurationsprogramm aus.';

    rsModKeyboard = 'Tastaturbelegung';
    rsModLpt = 'lpt-Quellen';
    rsQuit = 'Beenden';

function StartModule(module: String; back: boolean): boolean;
begin
    if module = 'keyboard' then begin
        SetupKeyboard(back);
    end else if module = 'lpt' then begin
        SetupLPT(back);
    end else begin
        exit(false);
    end;

    exit(true);
end;

procedure MainMenu(screen: TUIObject; frame: TUIFrame);
var
    mainw: TUIRowContainer;

    a: TUIMenu;
    b: TUILabel;

    buttons: TUIColContainer;
    btnOk: TUIButton;
    btnCancel: TUIButton;
begin
    mainw := TUIRowContainer.create(2);
    mainw.spacer := true;
    frame.obj := mainw;

    a := TUIMenu.create(4);
    with a do begin
        bgcolor := 0;
        color := 14;

        AddItem(rsModKeyboard, nil);
        AddItem(rsModLpt, nil);
        AddItem('', nil);
        AddItem(rsQuit, nil);
    end;
    mainw[1] := a;
    mainw.fixed[1] := 4;

    b := TUILabel.create;
    b.content := Utf8Decode(rsChooseModule);
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
                            a.selected := (a.selected + 4 - 1) mod 4;
                            if a.selected = 2 then begin
                                a.selected := 1;
                            end;
                        end;
                    SK_DOWN:
                        begin
                            a.selected := (a.selected + 1) mod 4;
                            if a.selected = 2 then begin
                                a.selected := 3;
                            end;
                        end;
                end;
            #10:
                case a.selected of
                    0:  StartModule('keyboard', true);
                    1:  StartModule('lpt', true);
                    3:  break;
                end;
        end;
    until false;

    frame.obj := nil;
    mainw.free;
end;

end.
