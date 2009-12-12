program setup;
{$MODE ObjFPC}
{$H+}

uses
    crt, tui, multilang, dos,
    menu;

procedure SetLang;
var
    name: String;
    lang: TLanguage;
begin
    name := GetEnv('LANG');
    if name <> '' then begin
        lang := GetLanguage(name);
        if lang <> nil then begin
            SetLanguage(lang);
        end;
    end;
end;

resourcestring
    rsTitle = 'tyndur-Konfiguration';
var
    screen: TUIRowContainer;
    title: TUILabel;
    f: TUIFrame;
    show_menu: boolean;
    i: integer;
begin
    SetLang;

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

    show_menu := true;
    for i := 1 to system.ParamCount do begin
        if not StartModule(system.ParamStr(i), false) then begin
            show_menu := false;
            break;
        end;
    end;

    if show_menu then begin
        MainMenu(screen, f);
    end;

    screen.free;

    TextBackground(0);
    TextColor(7);
    ClrScr;
end.
