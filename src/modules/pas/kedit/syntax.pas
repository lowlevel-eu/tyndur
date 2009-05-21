unit syntax;
{$MODE ObjFPC}

interface

type
    TSyntaxColor = (syn_keyword, syn_string, syn_string_special, syn_comment,
        syn_type, syn_compiler, syn_number, syn_label, syn_other,
        syn_trailing_space);

    TSyntaxChange = record
        posY:   integer;
        color:  TSyntaxColor;
    end;

    TSyntax = class
        protected
            f_state: integer;
            color: TSyntaxColor;
            line: String;
            pos: integer;

            function Highlight(c: TSyntaxColor): TSyntaxChange;
        public
            constructor create;

            procedure StartLine(s: String; state: integer); virtual; abstract;
            function Next: TSyntaxChange; virtual; abstract;
            property GetState: integer read f_state;
    end;

    TSyntax_C = class(TSyntax)
        public
            procedure StartLine(s: String; state: integer); override;
            function Next: TSyntaxChange; override;
    end;


function SyntaxColor(c: TSyntaxColor): byte;
function SyntaxBgColor(c: TSyntaxColor): byte;


implementation

const
    Keywords_C: Array [1..15] of String = (
        'if', 'else', 'for', 'while', 'do', 'switch', 'case', 'default', 'goto',
        'break', 'continue', 'return', 'sizeof', 'typedef', 'asm'
    );
    Types_C: Array [1..16] of String = (
        'void', 'char', 'short', 'int', 'long', 'signed', 'unsigned', 'float', 'double',
        'static', 'inline', 'extern', 'enum', 'struct', 'union', 'volatile'
    );


function SyntaxColor(c: TSyntaxColor): byte;
begin
    case c of
        syn_comment:        exit(9);
        syn_other:          exit(7);
        syn_number:         exit(4);
        syn_string:         exit(4);
        syn_string_special: exit(5);
        syn_keyword:        exit(6);
        syn_type:           exit(2);
        syn_compiler:       exit(5);
        syn_label:          exit(6);
    end;

    exit(8);
end;

function SyntaxBgColor(c: TSyntaxColor): byte;
begin
    case c of
        syn_trailing_space: exit(4);
    end;

    exit(0);
end;

function Matches(s: String; pos: integer; pattern: String): boolean;
begin
    Matches := (Copy(s, pos, length(pattern)) = pattern);
end;

function MatchesNumber(s: String; pos: integer): integer;
var
    i: integer;
    long: integer;
    needfig: boolean;
    hex: boolean;
    prefix: boolean;
begin
    if (pos > 1) and (s[pos - 1] <> ' ') then begin
        exit(0);
    end;

    long := 0;
    needfig := false;
    hex := false;
    prefix := false;
    for i := pos to length(s) do begin
        if not (s[i] in ['A'..'Z', 'a'..'z', '0'..'9', '_', '-', '+']) then begin
            if needfig then begin
                MatchesNumber := 0;
            end else begin
                MatchesNumber := i - pos;
            end;
            exit;
        end else if (s[i] in ['-', '+']) and not prefix and (i = pos) then begin
            prefix := true;
            needfig := true;
        end else if (s[i] in ['l', 'L']) then begin
            Inc(long);
            if long > 2 then begin
                MatchesNumber := 0;
                exit;
            end;
        end else if (long > 0) then begin
            MatchesNumber := 0;
            exit;
        end else if (s[i] = 'x') and not prefix and (i = pos + 1) then begin
            needfig := true;
            hex := true;
        end else if (s[i] = 'x') and prefix and (i = pos + 2) then begin
            needfig := true;
            hex := true;
        end else if s[i] in ['0' .. '9'] then begin
            needfig := false;
        end else if (s[i] in ['A' .. 'F', 'a' .. 'f']) and hex then begin
            needfig := false;
        end else begin
            MatchesNumber := 0;
            exit;
        end;
    end;

    if needfig then begin
        MatchesNumber := 0;
    end else begin
        MatchesNumber := length(s) - pos + 1;
    end;
end;

function MatchesKeyword(s: String; pos: integer; p: Array of String): integer;
var
    i: integer;
begin
    if (pos > 1) and (s[pos - 1] in ['A'..'Z', 'a'..'z', '0'..'9', '_']) then begin
        exit(0);
    end;

    for i := Low(p) to High(p) do begin
        if Matches(s, pos, p[i]) then begin
            if (pos + Length(p[i]) <= Length(s)) and (s[pos + Length(p[i])] in ['A'..'Z', 'a'..'z', '0'..'9', '_']) then begin
                exit(0);
            end;
            exit(length(p[i]));
        end;
    end;
    exit(0);
end;

function MatchesLabel(s: String; pos: integer): integer;
var
    i: integer;
begin
    for i := pos to length(s) do begin
        if (s[i] = ':') and (i > pos) then begin
            exit(i - pos + 1);
        end else if not (s[i] in ['A'..'Z', 'a'..'z', '0'..'9', '_']) then begin
            exit(0);
        end;
    end;
    exit(0);
end;

function MatchesType(s: String; pos: integer): integer;
var
    i: integer;
    has_underscore: boolean;
    is_type: boolean;
begin
    if (pos > 1) and (s[pos - 1] in ['A'..'Z', 'a'..'z', '0'..'9', '_']) then begin
        exit(0);
    end;

    has_underscore := false;
    is_type := false;
    for i := pos to length(s) do begin
        if (s[i] = '_') and (i > pos) then begin
            has_underscore := true;
            is_type := false;
        end else if has_underscore and (s[i] = 't') and (i > pos) then begin
            has_underscore := false;
            is_type := true;
        end else if not (s[i] in ['A'..'Z', 'a'..'z', '0'..'9', '_']) then begin
            if is_type then begin
                exit(i - pos);
            end else begin
                exit(0);
            end;
        end else begin
            has_underscore := false;
            is_type := false;
        end;
    end;

    if is_type then begin
        exit(length(s) - pos + 1);
    end else begin
        exit(0);
    end;
end;

function MatchesTrailingSpace(s: String; pos: integer): boolean;
var
    i: integer;
begin
    for i := pos to length(s) do begin
        if (s[i] <> ' ') then begin
            exit(false);
        end;
    end;
    exit(true);
end;

constructor TSyntax.create;
begin
    f_state := 0;
    line := '';
    pos := 0;
    color := syn_other;
end;

function TSyntax.Highlight(c: TSyntaxColor): TSyntaxChange;
begin
    Highlight.posY := pos;
    Highlight.color := color;
    color := c;
end;

procedure TSyntax_C.StartLine(s: String; state: integer);
begin
    line := s;
    f_state := state;
    pos := 1;

    case f_state of
        1:   color := syn_comment;
        2:   color := syn_comment;
        3:   color := syn_number;
        4:   color := syn_string;
        5:   color := syn_string_special;
        6:   color := syn_keyword;
        7:   color := syn_type;
        8:   color := syn_label;
        else color := syn_other;
    end;
end;

function TSyntax_C.Next: TSyntaxChange;
var
    c: char;
    tmp: integer;
begin
    Next.posY := 0;
    Next.color := color;

    while pos <= length(line) do begin
        c := line[pos];

        case f_state of

            0: { Normaler Text }
                case c of
                    '/':
                        if Matches(line, pos, '//') then begin
                            f_state := 1;
                            exit(Highlight(syn_comment));
                        end else if Matches(line, pos, '/*') then begin
                            f_state := 2;
                            exit(Highlight(syn_comment));
                        end;

                    '0' .. '9', '-', '+':
                        begin
                            tmp := MatchesNumber(line, pos);
                            if tmp <> 0 then begin
                                Next := Highlight(syn_number);
                                Inc(pos, tmp);
                                f_state := 3;
                                exit;
                            end;
                        end;

                    '#':
                        begin
                            if Copy(line, 1, pos - 1) = Space(pos - 1) then begin
                                f_state := 1;
                                exit(Highlight(syn_compiler));
                            end;
                        end;

                    '"':
                        begin
                            Next := Highlight(syn_string);
                            Inc(pos);
                            f_state := 4;
                            exit;
                        end;
                    ' ':
                        begin
                            if MatchesTrailingSpace(line, pos) then begin
                                Next := Highlight(syn_trailing_space);
                                pos := length(line) + 1;
                                exit;
                            end;
                        end;

                    else
                        begin
                            tmp := MatchesKeyword(line, pos, Keywords_C);
                            if tmp <> 0 then begin
                                Next := Highlight(syn_keyword);
                                Inc(pos, tmp);
                                f_state := 6;
                                exit;
                            end;

                            tmp := MatchesKeyword(line, pos, Types_C);
                            if tmp <> 0 then begin
                                Next := Highlight(syn_type);
                                Inc(pos, tmp);
                                f_state := 7;
                                exit;
                            end;

                            tmp := MatchesType(line, pos);
                            if tmp <> 0 then begin
                                Next := Highlight(syn_type);
                                Inc(pos, tmp);
                                f_state := 7;
                                exit;
                            end;

                            tmp := MatchesLabel(line, pos);
                            if tmp <> 0 then begin
                                Next := Highlight(syn_label);
                                Inc(pos, tmp);
                                f_state := 8;
                                exit;
                            end;
                        end;
                end;

            1: { Kommentar bis Zeilenende }
                begin
                    pos := length(line);
                    break;
                end;

            2: { C-Kommentar }
                if Matches(line, pos, '*/') then begin
                    f_state := 0;
                    Inc(pos, 2);
                    exit(Highlight(syn_other));
                end;

            3, 6, 7, 8: { Ende eines gefaerbten Worts }
                begin
                    f_state := 0;
                    exit(Highlight(syn_other));
                end;

            4: { String }
                case c of
                    '\':
                        begin
                            f_state := 5;
                            Next := Highlight(syn_string_special);
                            Inc(pos);
                            exit;
                        end;

                    '"':
                        begin
                            f_state := 0;
                            Inc(pos);
                            exit(Highlight(syn_other));
                        end;
                end;

            5: { Escaptes Zeichen in einem String }
                begin
                    f_state := 4;
                    Inc(pos);
                    exit(Highlight(syn_string));
                end;

        end;

        Inc(pos);
    end;

    if f_state in [1, 3, 4, 5] then begin
        f_state := 0;
    end;
end;

end.
