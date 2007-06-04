program pashello;
uses crt;

var
    x: String;
    c: char;
    f: text;

    p: Pointer;
begin
    p := GetMem(42);
    WriteLn(HexStr(dword(p), 8));
    p := GetMem(42);
    WriteLn(HexStr(dword(p), 8));
    p := GetMem(1900);
    WriteLn(HexStr(dword(p), 8));

(*
    WriteLn('Hello World!');
    Write('foo: ');
    ReadLn(x);
    WriteLn('bar: ', x);
    Write('foo: ');
    ReadLn(x);
    WriteLn('baz: ', x);
    repeat
        c := readkey;
        Write(Ord(c), ' ');
        Flush(output);
    until false;
*)
    (*Assign(f, 'floppy:/devices/fd0|fat:/etc/init.cfg');
    Reset(f);

    while not eof(f) do begin
        ReadLn(f, x);
        WriteLn(x);
    end;
    WriteLN('EOF');

    Close(f);*)

(*
    Assign(f, 'floppy:/devices/fd0|fat:/etc/test.txt');
    Rewrite(f);
    WriteLn('z'); flush(output);
    WriteLn(f, 'Eine Testdatei');
    WriteLn('a'); flush(output);
    WriteLn(f, 'Mit Text');
    WriteLn('b'); flush(output);
    WriteLn(f, 'Zum Testen');
    WriteLn('c'); flush(output);
    Close(f);
    *)
end.
