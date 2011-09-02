(*
 * Copyright (c) 2009-2011 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Alexander Hartmut Kluth.
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

unit syntax_atandt;
{$mode ObjFPC}

interface

uses syntax;

type
    TSyntax_ATANDT = class(TSyntax)
    public
        procedure StartLine(s: String; state: integer); override;
        function Next: TSyntaxChange; override;
    end;


implementation
const
{ TODO: Keine Ahnung... }
{   Keywords_ATANDT: Array [1..48] of String = (

}
    Types_ATANDT: Array [1..90] of String = (
        '%ax', '%ah', '%al', '%bx', '%bh', '%bl', '%cx', '%ch', '%cl',
        '%dx', '%dh', '%dl', '%si', '%ss', '%sp', '%bp', '%cs', '%ds',
		'%es', '%fs', '%gs', '%eax', '%ebx', '%ecx', '%edx', '%edi',
		'%esi', '%ebp', '%esp', '%eip', '%rax', '%rbx', '%rcx',
		'%rdx', '%rdi', '%rsi', '%rbp', '%rsp', '%rip', '%r8', '%r9',
		'%r10', '%r11', '%r12', '%r13', '%r14', '%r15', '%dil', '%sil', '%bpl',
		'%spl', '%r8l', '%r9l', '%r10l', '%r11l', '%r12l', '%r13l', '%r14l',
		'%r15l', '%r8w', '%r9w', '%r10w', '%r11w', '%r13w', '%r14w', '%r15w',
		'%r8d', '%r9d', '%r10d', '%r11d', '%r12d', '%r13d', '%r14d', '%r15d',
		'%xmm0', '%xmm1', '%xmm2', '%xmm3', '%xmm4', '%xmm5', '%xmm6', '%xmm7',
		'%xmm8', '%xmm9', '%xmm10', '%xmm11', '%xmm12', '%xmm13', '%xmm14',
		'%xmm15'
    );

procedure TSyntax_ATANDT.StartLine(s: String; state: integer);
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


function TSyntax_ATANDT.Next: TSyntaxChange;
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
                        if Matches(line, pos, '/*') then begin
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
                            {tmp := MatchesKeyword(line, pos, Keywords_ATANDT);
                            if tmp <> 0 then begin
                                Next := Highlight(syn_keyword);
                                Inc(pos, tmp);
                                f_state := 6;
                                exit;
                            end;
                            }

                            tmp := MatchesKeyword(line, pos, Types_ATANDT);
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

                            { FIXME:  Keywords werden nicht erkannt }
                            Next := Highlight(syn_label);
                            Inc(pos, tmp);
                            f_state := 8;
                        end;
                end;

            1: { Kommentar bis Zeilenende }
                begin
                    pos := length(line);
                    break;
                end;

            2: { C-ähnlicher-Kommentar }
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
