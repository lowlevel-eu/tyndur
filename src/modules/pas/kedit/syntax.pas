(*
 * Copyright (c) 2008-2011 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
 * by Kevin Wolf.
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


function SyntaxColor(c: TSyntaxColor): byte;
function SyntaxBgColor(c: TSyntaxColor): byte;
function Matches(s: String; pos: integer; pattern: String): boolean;
function MatchesNumber(s: string; pos: integer): integer;
function MatchesKeyword(s: string; pos: integer; p: Array of String): integer;
function MatchesLabel(s: string; pos: integer): integer;
function MatchesType(s: string; pos: integer): integer;
function MatchesTrailingSpace(s: string; pos: integer): boolean;


implementation
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
    unsigned: boolean;
    needfig: boolean;
    hex: boolean;
    prefix: boolean;
begin
    if (pos > 1) and (s[pos - 1] in ['A'..'Z', 'a'..'z', '0'..'9', '_']) then begin
        exit(0);
    end;

    long := 0;
    unsigned := false;
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
        end else if (s[i] in ['-', '+']) then begin
            if needfig then begin
                MatchesNumber := 0;
            end else begin
                MatchesNumber := i - pos;
            end;
            exit;
        end else if (s[i] in ['u', 'U']) then begin
            if unsigned then begin
                MatchesNumber := 0;
                exit;
            end;
            unsigned := true;

            if long > 0 then begin
                // LUL is nicht erlaubt
                long := 2;
            end;
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

end.
