(*
 * Copyright (c) 2011 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Alexander Kluth.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *)

unit syntax_perl;
{$mode ObjFPC}

interface

uses syntax;

type
    TSyntax_Perl = class(TSyntax)
    public
        procedure StartLine(s: String; state: integer); override;
        function Next: TSyntaxChange; override;
    end;


implementation
const
    Keywords_Perl: Array [1..61] of String = (
        'not', 'exp', 'log', 'srand', 'xor', 's', 'qq', 'qx', 'xor',
        'length', 'uc', 'ord', 'and', 'print', 'chr',
        'ord', 'for', 'qw', 'q', 'join', 'use', 'sub', 'tied', 'qx',
        'eval', 'int', 'for', 'abs', 'ne', 'open', 'hex', 'exp',
        'ref', 'scalar', 'srand', 'sqrt', 'cos', 'printf',
        'each', 'return', 'local', 'undef', 'or', 'oct', 'time',
        'foreach', 'chdir', 'kill', 'exec', 'gt', 'sort', 'split',
        'if', 'else', 'elsif', 'while', 'do', 'switch', 'case', 'my',
        'require'
    );

    Vars_Perl: Array [1..25] of String = (
        '$_', '$!', '$|', '$%', '$=', '$-', '$~', '$^', '$&', '$''', '$`', '$+',
        '$/', '$\', '$,', '$"', '$$', '$#', '$?', '$*', '$0', '$[', '$]', '$;', '$@'
    );

procedure TSyntax_Perl.StartLine(s: String; state: integer);
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


function TSyntax_Perl.Next: TSyntaxChange;
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
                        begin
                            f_state := 2;
                            exit(Highlight(syn_string_special));
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
                                exit(Highlight(syn_comment));
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
                            tmp := MatchesKeyword(line, pos, Keywords_Perl);
                            if tmp <> 0 then begin
                                Next := Highlight(syn_keyword);
                                Inc(pos, tmp);
                                f_state := 6;
                                exit;
                            end;

                            tmp := MatchesKeyword(line, pos, Vars_Perl);
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

            2: { Regulärer Ausdruck }
                if Matches(line, pos, '/') then begin
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
