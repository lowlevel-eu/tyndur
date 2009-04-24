unit tyndur;

interface

    function ReadUnicodeChar: UnicodeChar;


implementation

    uses crt;

    function ReadUnicodeChar: UnicodeChar;
    var
        keys: Array [1..5] of char;
        pkeys: PChar;

        res: Array [1..2] of UnicodeChar;
        pres: PUnicodeChar;

        ret: integer;
        i: integer;
    begin
        pkeys := @keys[1];
        pres := @res[1];
        for i := 1 to 4 do begin
            keys[i] := readkey;
            keys[i+1] := #0;
            ret := Utf8ToUnicode(pres, 2, pkeys, i);
            if ret >= 0 then begin
                break;
            end;
        end;
        exit(res[1]);
    end;

end.
