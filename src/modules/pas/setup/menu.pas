unit menu;

interface
uses tui;

procedure MainMenu(screen: TUIObject; frame: TUIFrame);
function StartModule(module: String; back: boolean): boolean;

implementation

uses tyndur, crt, setup_keyboard, setup_lpt, setup_network;

resourcestring
    rsChooseModule = 'Bitte wähle aus dem Menü das gewünschte Konfigurationsprogramm aus.';

    rsModKeyboard = 'Tastaturbelegung';
    rsModLpt = 'lpt-Quellen';
    rsModNetwork = 'Netzwerk';
    rsQuit = 'Beenden';

function StartModule(module: String; back: boolean): boolean;
begin
    ClrScr;
    if module = 'keyboard' then begin
        SetupKeyboard(back);
    end else if module = 'lpt' then begin
        SetupLPT(back);
    end else if module = 'network' then begin
        SetupNetwork(back, false);
    end else if module = 'network-detect' then begin
        SetupNetwork(back, true);
    end else begin
        exit(false);
    end;

    ClrScr;
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

    a := TUIMenu.create(5);
    with a do begin
        bgcolor := 0;
        color := 14;

        AddItem(rsModKeyboard, nil);
        AddItem(rsModLpt, nil);
        AddItem(rsModNetwork, nil);
        AddItem('', nil);
        AddItem(rsQuit, nil);
    end;
    mainw[1] := a;
    mainw.fixed[1] := 5;

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
                            a.selected := (a.selected + 5 - 1) mod 5;
                            if a.selected = 3 then begin
                                a.selected := 2;
                            end;
                        end;
                    SK_DOWN:
                        begin
                            a.selected := (a.selected + 1) mod 5;
                            if a.selected = 3 then begin
                                a.selected := 4;
                            end;
                        end;
                end;
            #10:
                case a.selected of
                    0:  StartModule('keyboard', true);
                    1:  StartModule('lpt', true);
                    2:  StartModule('network', true);
                    4:  break;
                end;
        end;
    until false;

    frame.obj := nil;
    mainw.free;
end;

end.
