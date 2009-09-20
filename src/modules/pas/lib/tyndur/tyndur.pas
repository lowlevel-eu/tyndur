unit tyndur;

interface

    const
        SK_F1       = #59;
        SK_F2       = #60;
        SK_F3       = #61;
        SK_F4       = #62;
        SK_F5       = #63;
        SK_F6       = #64;
        SK_F7       = #65;
        SK_F8       = #66;
        SK_F9       = #67;
        SK_F10      = #68;
        SK_F11      = #133;
        SK_F12      = #134;

        SK_INS      = #82;
        SK_DEL      = #83;
        SK_HOME     = #71;
        SK_END      = #79;
        SK_PGUP     = #73;
        SK_PGDN     = #81;

        SK_UP       = #72;
        SK_DOWN     = #80;
        SK_LEFT     = #75;
        SK_RIGHT    = #77;

        SK_CTRL_PGUP    =   #132;
        SK_CTRL_PGDN    =   #118;

        SK_MIN = #59;
        SK_MAX = #134;


    function ReadUnicodeChar: UnicodeChar;
    function ReadEscapeSequence: char;


implementation

    uses crt;

    function c_mbstowcs(dest: PUnicodeChar; src: PChar; len: SizeUInt):
        SizeUInt; cdecl; external name 'mbstowcs';

    function ReadUnicodeChar: UnicodeChar;
    var
        keys: Array [1..5] of char;
        pkeys: PChar;

        res: Array [1..2] of UnicodeChar;
        pres: PUnicodeChar;

        ret: SizeUInt;
        i: integer;
    begin
        pkeys := @keys[1];
        pres := @res[1];
        for i := 1 to 4 do begin
            keys[i] := readkey;
            keys[i+1] := #0;
            ret := c_mbstowcs(pres, pkeys, 1);
            if ret <> SizeUInt(-1) then begin
                break;
            end;
        end;
        exit(res[1]);
    end;

    function ReadEscapeSequence: char;
    begin
        ReadEscapeSequence := #0;

        case readkey of
            'O':
                case readkey of
                    'P': exit(SK_F1);
                    'Q': exit(SK_F2);
                    'R': exit(SK_F3);
                    'S': exit(SK_F4);
                end;

            '[':
                case readkey of
                    'A': exit(SK_UP);
                    'B': exit(SK_DOWN);
                    'C': exit(SK_RIGHT);
                    'D': exit(SK_LEFT);
                    'F': exit(SK_END);
                    'H': exit(SK_HOME);

                    '3': if readkey = '~' then exit(SK_DEL);
                    '5': if readkey = '~' then exit(SK_PGUP);
                    '6': if readkey = '~' then exit(SK_PGDN);

                    '1':
                        case readkey of
                            '5': if readkey = '~' then exit(SK_F5);
                            '7': if readkey = '~' then exit(SK_F6);
                            '8': if readkey = '~' then exit(SK_F7);
                            '9': if readkey = '~' then exit(SK_F8);
                        end;

                    '2':
                        case readkey of
                            '0': if readkey = '~' then exit(SK_F9);
                            '1': if readkey = '~' then exit(SK_F10);
                            '3': if readkey = '~' then exit(SK_F11);
                            '4': if readkey = '~' then exit(SK_F12);
                            '~': exit(SK_INS);
                        end;
                end;
        end;
    end;


end.
