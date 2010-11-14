unit multilang;
{$MODE ObjFPC}
{$H+}

interface

    type
        TLanguage = Pointer;

    function NumLanguages: integer;
    function GetLanguage(name: String): TLanguage;
    function GetLanguageById(id: integer): TLanguage;
    function GetLanguageName(lang: TLanguage): String;
    procedure SetLanguage(lang: TLanguage);

    function FormatNum(const fmt: String; const args: array of const): String;

implementation

    uses sysutils;

    type
        tres = packed record
            name: AnsiString;
            value: AnsiString;
        end;

        tms_dict = packed record
            resstr: ^tres;
            translation: PChar;
        end;
        ptms_dict = ^tms_dict;

    type
        TGetNumber = function(n: integer): integer; cdecl;
        TLanguageData = record
            lang:       PChar;
            numbers:    integer;
            get_number: TGetNumber;
            strings:    ptms_dict;
        end;
        PLanguage = ^TLanguageData;
        PPLanguage = ^PLanguage;

    var
        lang: PPLanguage;
        num_lang: integer;
        cur_lang: PLanguage;


    function LoadTranslation(name, value: AnsiString; Hash: Longint; arg: Pointer): AnsiString;
    var
        dict: ptms_dict;
        i: integer;
    begin
        dict := arg;
        if dict = nil then begin
            exit(value);
        end;

        i := 0;
        while dict[i].resstr <> nil do begin
            if Pointer(name) = Pointer(dict[i].resstr^.name) then begin
                exit(dict[i].translation);
            end;
            Inc(i);
        end;

        exit(value);
    end;

    function NumLanguages: integer;
    begin
        exit(num_lang);
    end;

    function GetLanguage(name: String): TLanguage;
    var
        i: integer;
    begin
        for i := 0 to num_lang - 1 do begin
            if lang[i]^.lang = name then begin
                exit(lang[i]);
            end;
        end;

        exit(nil);
    end;

    function GetLanguageById(id: integer): TLanguage;
    begin
        exit(lang[id]);
    end;

    function GetLanguageName(lang: TLanguage): String;
    var
        ldata: PLanguage;
    begin
        ldata := lang;
        exit(ldata^.lang);
    end;

    procedure SetLanguage(lang: TLanguage);
    begin
        cur_lang := lang;
        SetResourceStrings(@LoadTranslation, cur_lang^.strings);
    end;

    function FormatNum(const fmt: String; const args: array of const): String;
    var
        s: String;
        i, j, outi: integer;
        num, value, number: integer;
        colon: integer;
    begin
        s := '';
        SetLength(s, Length(fmt));

        outi := 1;
        i := 1;
        while i <= Length(fmt) do begin
            if (fmt[i] = '%') and (fmt[i + 1] = '[') then begin

                // Nummer des Arguments bestimmen
                colon := Pos(':', PChar(@fmt[i + 2]));
                num := StrToInt(Copy(fmt, i + 2, colon - 1));
                value := args[num].VInteger;
                number := cur_lang^.get_number(value);

                // Start-Doppelpunkt finden
                Inc(i, 2 + colon);
                for j := 1 to cur_lang^.get_number(value) do begin
                    colon := Pos(':', PChar(@fmt[i]));
                    Inc(i, colon);
                end;

                // Wort kopieren
                if number = cur_lang^.numbers - 1 then begin
                    colon := Pos(']', PChar(@fmt[i]));
                end else begin
                    colon := Pos(':', PChar(@fmt[i]));
                end;
                Move(fmt[i], s[outi], colon - 1);
                Inc(outi, colon - 1);

                // Ende des Ausdrucks suchen
                Inc(i, Pos(']', PChar(@fmt[i])));
            end else begin
                s[outi] := fmt[i];
                Inc(outi);
                Inc(i);
            end;
        end;

        SetLength(s, outi - 1);
        exit(Format(s, args));
    end;

function default_get_number(n: integer): integer; cdecl;
begin
    if n = 1 then begin
        exit(0);
    end else begin
        exit(1);
    end;
end;

var
    tmslang_start: PLanguage; external name '__start_tmslang';
    tmslang_end: PLanguage; external name '__stop_tmslang';

    default_lang: TLanguageData;

initialization
    with default_lang do begin
        lang        := 'C';
        numbers     := 2;
        get_number  := @default_get_number;
        strings     := nil;
    end;

    cur_lang := @default_lang;

    lang := @tmslang_start;
    num_lang := PPLanguage(@tmslang_end) - PPLanguage(@tmslang_start);

end.

