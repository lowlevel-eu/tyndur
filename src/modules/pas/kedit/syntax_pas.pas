unit syntax_pas;
{$mode ObjFPC}

interface

uses syntax;

type
    TSyntax_Pas = class(TSyntax)
    public
        procedure StartLine(s: String; state: integer); override;
        function Next: TSyntaxChange; override;
    end;


implementation
const
    Keywords_Pas: Array [1..88] of String = (
        'absolute', 'abstract', 'and', 'ansistring', 'array', 'assembler', 'asm',
        'automated', 'begin', 'case', 'cdecl',
        'class', 'compilerprog', 'const', 'constructor',
        'deconstructor', 'default', 'deprecated', 'div', 'do', 'downto',
        'dword', 'dynamic', 'else', 'end', 'export', 'external', 'far',
        'forward', 'finalization', 'for',
        'function', 'generic', 'goto', 'inline', 'if', 'implementation', 'in',
        'interface', 'label', 'longint', 'message', 'mod', 'near', 'nil', 'not',
        'object', 'overlay', 'overload', 'override', 'of', 'or', 'packed',
        'pchar', 'platform', 'pointer', 'private', 'procedure',
        'program', 'property', 'protected', 'public', 'published', 'raise',
        'record', 'reintroduce', 'resourcestring',
        'repeat', 'sealed', 'set', 'shl', 'shortint', 'shr', 'specialize', 'stdcall',
        'then', 'threadvar', 'to',
        'type', 'unit', 'until', 'uses', 'var', 'virtual', 'while', 'with',
        'word', 'xor'
    );

    Types_Pas: Array [1..8] of String = (
        'integer', 'byte', 'real', 'char', 'string', 'boolean', 'string',
        'file'
    );


procedure TSyntax_Pas.StartLine(s: String; state: integer);
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


function TSyntax_Pas.Next: TSyntaxChange;
var
    c: char;
    tmp: integer;
    curly_comment: boolean;
    star_comment: boolean;
begin
    Next.posY := 0;
    Next.color := color;

    while pos <= length(line) do begin
        c := line[pos];

        case f_state of
            0:
                case c of
                    { FIXME: Kommentar-Ebenen funktionieren nicht richtig }
                    '{':
                        if Matches(line, pos, '{$') then begin
                            f_state := 1;
                            exit(Highlight(syn_compiler));
                        end else begin
                            if star_comment = true then begin
                                curly_comment := false;
                            end else begin
                                curly_comment := true;
                                f_state := 2;
                                exit(Highlight(syn_comment));
                            end;
                        end;

                    '(':
                        if Matches(line, pos, '(*') then begin
                            if curly_comment = true then begin
                                star_comment := false;
                            end else begin
                                star_comment := true;
                                f_state := 2;
                                exit(Highlight(syn_comment));
                            end;
                        end;

                    '/':
                        if (Matches(line, pos, '//')) then begin
                            f_state := 1;
                            exit(Highlight(syn_comment));
                        end;

                    '0' .. '9', '-', '+', '$', '%', '&':
                        begin
                            tmp := MatchesNumber(line, pos);

                            if tmp <> 0 then begin
                                Next := Highlight(syn_number);
                                Inc(pos, tmp);
                                f_state := 3;
                                exit;
                            end;
                        end;

                    '''':
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
                            tmp := MatchesKeyword(line, pos, Keywords_Pas);
                            if tmp <> 0 then begin
                                Next := Highlight(syn_keyword);
                                Inc(pos, tmp);
                                f_state := 6;
                                exit;
                            end;

                            tmp := MatchesKeyword(line, pos, Types_Pas);
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

            2: { Pascal-Kommentar }
                if Matches(line, pos, '}') then begin
                    if (star_comment = true) and (curly_comment = false) then
                        begin
                        exit;
                    end else begin
                        f_state := 0;
                        Inc(pos, 1);
                        exit(Highlight(syn_other));
                    end;
                end else if Matches(line, pos, '*)') then begin
                    if (curly_comment = true) and (star_comment = false) then
                        begin
                        exit;
                    end else begin
                        f_state := 0;
                        Inc(pos, 2);
                        exit(Highlight(syn_other));
                    end;
                end;

            3, 6, 7, 8: { Ende eines gefaerbten Worts }
                begin
                    f_state := 0;
                    exit(Highlight(syn_other));
                end;

            4: { String }
                case c of
                    { TODO: Escape-Sequenzen }

                    '''':
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
