unit tui;
{$MODE ObjFPC}
{$H+}
{$COPERATORS ON}

interface

uses sysutils;

type
    (**
     * Basisklasse fuer alle Widgets.
     *)
    TUIObject = class
        public
            color: byte;
            bgcolor: byte;

        protected
            f_w, f_h: integer;

        public
            constructor create;

            (**
             * Ordnet die Kinder des Widgets in der gegebenen Breite und Hoehe
             * an, so dass sie anschliessend gezeichnet werden koennen
             *)
            procedure align(w, h: integer); virtual;

            (**
             * Zeichnet das Widget an die gegebene Position (und ruft die
             * Zeichenroutine aller Kinder auf)
             *)
            procedure draw(x, y: integer); virtual;

            property width: integer read f_w write f_w;
            property height: integer read f_h write f_h;
    end;
    PTUIObject = ^TUIObject;

    (**
     * Abstrakte Basisklasse fuer RowContainer und ColContainer, die alle ihre
     * Kinder unter- bzw. nebeneinander anordnen.
     *)
    TUIRowColContainer = class(TUIObject)
        public
            (**
             * @param num Anzahl der Kindobjekte
             *)
            constructor create(num: integer);
            destructor free;

        protected
            cols: integer;
            children: PTUIObject;

            f_fixed: PInteger;
            f_prop: PInteger;

        public
            spacer: boolean;

        protected
            procedure SetChild(i: integer; obj: TUIObject);

            (**
             * Setzt fuer das i-te Kindobjekt eine fest Breite/Hoehe
             * (in Zeichen)
             *)
            procedure SetFixedWidth(i: integer; w: integer);

            (**
             * Setzt fuer das i-te Kindobject eine proportionale Groesse
             * (in Anteilen). Der proportional verteilte Platz ist der freie
             * uebrige Platz nachdem alle festen Groessen vergeben sind.
             *)
            procedure SetPropWidth(i: integer; w: integer);

        public
            property _children[i: integer]: TUIObject write SetChild; default;
            property fixed[i: integer]: integer write SetFixedWidth;
            property prop[i: integer]: integer write SetPropWidth;
    end;

    (** TUIRowColContainer in Spalten *)
    TUIColContainer = class(TUIRowColContainer)
        public
            procedure align(w, h: integer); override;
            procedure draw(x, y: integer); override;
    end;

    (** TUIRowColContainer in Zeilen *)
    TUIRowContainer = class(TUIRowColContainer)
        public
            procedure align(w, h: integer); override;
            procedure draw(x, y: integer); override;
    end;

    (**
     * Widget fuer eine Flaeche um ein Objekt. Das Objekt wird mittig
     * in der Flaeche platziert falls es kleiner ist.
     *)
    TUIFrame = class(TUIObject)
        protected
            f_obj: TUIObject;
            f_obj_w: integer;
            f_obj_h: integer;

        public
            constructor create(obj: TUIObject; w, h: integer);
            procedure align(w, h: integer); override;
            procedure draw(x, y: integer); override;

            property obj: TUIObject read f_obj write f_obj;
    end;

    (** Textanzeige (mehrzeilig, automatischer Zeilenumbruch) *)
    TUILabel = class(TUIObject)
        protected
            f_content: UnicodeString;

        public
            procedure draw(x, y: integer); override;
            property content: UnicodeString read f_content write f_content;
    end;

    (** Ein beschrifteter Knopf *)
    TUIButton = class(TUILabel)
        public
            procedure draw(x, y: integer); override;
    end;

    (** Ein editierbares Textfeld *)
    TUITextField = class(TUILabel)
    end;

    (** Ein Radioknopf *)
    TUIRadioButton = class(TUILabel)
        protected
            f_active: boolean;

        public
            procedure draw(x, y: integer); override;
            property active: boolean read f_active write f_active;
    end;

    (** Ein Menueeintrag *)
    TMenuItem = record
        s: UnicodeString;
        opaque: Pointer;
    end;
    PMenuItem = ^TMenuItem;

    (**
     * Zeigt ein Menue mit mehreren Auswahlmoeglichkeiten an, wovon eine
     * selektiert sein kann und dann farblich hervorgehoben wird.
     *)
    TUIMenu = class(TUIObject)
        protected
            f_num: integer;
            f_cur: integer;
            f_selected: integer;
            f_items: PMenuItem;

        public
            sel_color: byte;
            sel_bgcolor: byte;

        public
            constructor create(num: integer);
            destructor free;

            procedure draw(x, y: integer); override;
            procedure AddItem(s: UnicodeString; opaque: Pointer);
            property selected: integer read f_selected write f_selected;
    end;


    ETUIContainerFull = class(Exception)
    end;

implementation

uses crt, strutils;

procedure FillRect(x, y, w, h: integer);
var
    i: integer;
begin
    for i := y to y + h - 1 do begin
        GotoXY(x, i);
        Write(StringOfChar(' ', w));
    end;
end;

procedure WritePadded(s: UnicodeString; w: integer);
begin
    if Length(s) >= w then begin
        Write(Copy(s, 1, w));
    end else begin
        Write(Utf8Encode(s));
        Write(StringOfChar(' ', w - Length(s)));
    end;
end;


constructor TUIObject.create;
begin
    color := 7;
    bgcolor := 0;
end;

procedure TUIObject.align(w, h: integer);
begin
    f_w := w;
    f_h := h;
end;

procedure TUIObject.draw(x, y: integer);
begin
    TextColor(color);
    TextBackground(bgcolor);
    FillRect(x, y, f_w, f_h);
end;

constructor TUIRowColContainer.create(num: integer);
var
    i: integer;
begin
    if num < 1 then begin
        raise Exception.create('TUIColContainer: num < 1');
    end;

    spacer := false;

    cols := num;
    children := GetMem(sizeof(PTUIObject) * num);
    f_fixed := GetMem(sizeof(integer) * num);
    f_prop := GetMem(sizeof(integer) * num);

    FillByte(children^, sizeof(PTUIObject) * num, 0);
    FillByte(f_fixed^, sizeof(integer) * num, 0);
    FillByte(f_prop^, sizeof(integer) * num, 0);
end;

destructor TUIRowColContainer.free;
var
    i: integer;
begin
    for i := 0 to cols - 1 do begin
        if children[i] <> nil then begin
            children[i].free;
        end;
    end;

    FreeMem(children);
    FreeMem(f_fixed);
    FreeMem(f_prop);
end;

procedure TUIRowColContainer.SetChild(i: integer; obj: TUIObject);
begin
    children[i] := obj;
end;

procedure TUIRowColContainer.SetFixedWidth(i: integer; w: integer);
begin
    f_fixed[i] := w;
    f_prop[i] := 0;
end;

procedure TUIRowColContainer.SetPropWidth(i: integer; w: integer);
begin
    f_prop[i] := w;
    f_fixed[i] := 0;
end;

procedure TUIColContainer.align(w, h: integer);
var
    i: integer;
    prop_space: integer;
    cur_w: integer;
begin
    f_w := w;
    f_h := h;

    prop_space := 0;
    for i := 0 to cols - 1 do begin
        if f_fixed[i] > 0 then begin
            if w >= f_fixed[i] then begin
                w -= f_fixed[i];
            end else begin
                f_fixed[i] := w;
                w := 0;
            end;

            if children[i] <> nil then begin
                children[i].align(f_fixed[i], h);
            end;
        end else begin
            prop_space += f_prop[i];
        end;

        if spacer and (w > 0) and (i < cols - 1) then begin
            Dec(w);
        end;
    end;

    for i := 0 to cols - 1 do begin
        if f_fixed[i] = 0 then begin
            if prop_space = 0 then begin
                if children[i] <> nil then begin
                    children[i].align(0, h);
                end
            end else begin
                cur_w := (w * f_prop[i]) div prop_space;
                if children[i] <> nil then begin
                    children[i].align(cur_w, h);
                end;
                prop_space -= f_prop[i];
                w -= cur_w;
            end;
        end;
    end;
end;

procedure TUIColContainer.draw(x, y: integer);
var
    i: integer;
    w: integer;
begin
    w := 0;
    for i := 0 to cols - 1 do begin
        if children[i] <> nil then begin
            children[i].draw(x + w, y);
            w += children[i].width;
        end else begin
            w += f_fixed[i];
        end;

        if spacer then begin
            Inc(w);
        end;
    end;

    if w < f_w then begin
        TextColor(color);
        TextBackground(bgcolor);
        FillRect(x + w, y, f_w - w, f_h);
    end;
end;

procedure TUIRowContainer.align(w, h: integer);
var
    i: integer;
    prop_space: integer;
    cur_h: integer;
begin
    f_w := w;
    f_h := h;

    prop_space := 0;
    for i := 0 to cols - 1 do begin
        if f_fixed[i] > 0 then begin
            if w >= f_fixed[i] then begin
                h -= f_fixed[i];
            end else begin
                f_fixed[i] := h;
                h := 0;
            end;

            if children[i] <> nil then begin
                children[i].align(w, f_fixed[i]);
            end;
        end else begin
            prop_space += f_prop[i];
        end;

        if spacer and (h > 0) and (i <> cols - 1) then begin
            Dec(h);
        end;
    end;

    for i := 0 to cols - 1 do begin
        if f_fixed[i] = 0 then begin
            if prop_space = 0 then begin
                if children[i] <> nil then begin
                    children[i].align(w, 0);
                end;
            end else begin
                cur_h := (h * f_prop[i]) div prop_space;
                if children[i] <> nil then begin
                    children[i].align(w, cur_h);
                end;
                prop_space -= f_prop[i];
                h -= cur_h;
            end;
        end;
    end;
end;

procedure TUIRowContainer.draw(x, y: integer);
var
    i: integer;
    h: integer;
begin
    h := 0;
    for i := 0 to cols - 1 do begin
        if children[i] <> nil then begin
            children[i].draw(x, y + h);
            h += children[i].height;
        end else begin
            h += f_fixed[i];
        end;

        if spacer then begin
            Inc(h);
        end;
    end;

    if h < f_h then begin
        TextColor(color);
        TextBackground(bgcolor);
        FillRect(x, y + h, f_w, f_h - h);
    end;
end;

constructor TUIFrame.create(obj: TUIObject; w, h: integer);
begin
    f_obj := obj;
    f_obj_w := w;
    f_obj_h := h;
end;

procedure TUIFrame.align(w, h: integer);
begin
    f_w := w;
    f_h := h;

    if f_obj_w < w then begin
        w := f_obj_w;
    end;

    if f_obj_h < h then begin
        h := f_obj_h;
    end;

    f_obj.align(w, h);
end;

procedure TUIFrame.draw(x, y: integer);
var
    w, h: integer;

    obj_x, obj_y: integer;
begin
    w := f_w;
    h := f_h;

    if f_obj_w < w then begin
        w := f_obj_w;
    end;

    if f_obj_h < h then begin
        h := f_obj_h;
    end;

    obj_x := x + ((f_w - w) div 2);
    obj_y := y + ((f_h - h) div 2);

    TextColor(color);
    TextBackground(bgcolor);

    // Linker Rand
    FillRect(x, y, obj_x - x, f_h);

    // Rechter Rand
    FillRect(obj_x + w, y, (x + f_w) - (obj_x + w), f_h);

    // Oberer Rand
    FillRect(obj_x, y, w, obj_y - y);

    // Unterer Rand
    FillRect(obj_x, obj_y + h, w, (y + f_h) - (obj_y + h));

    // Kindobjekt
    f_obj.draw(obj_x, obj_y);
end;

procedure TUILabel.draw(x, y: integer);
var
    s: UnicodeString;
    pos: integer;
    cnt: integer;
    row: integer;
    line_start_pos: integer;
begin
    if f_h > 0 then begin
        TextColor(color);
        TextBackground(bgcolor);
        FillRect(x, y, f_w, f_h);

        pos := 0;
        row := 0;
        cnt := 1;
        line_start_pos := 1;

        while (pos < Length(f_content)) and (row < f_h) do begin

            s := ExtractWordPos(cnt, f_content, [' '], pos);
            Inc(cnt);
            if s = '' then begin
                break;
            end;

            // FIXME Woerter, die laenger als eine Zeile sind
            if pos - line_start_pos + Length(s) > f_w then begin
                Inc(row);
                line_start_pos := pos;
                if row >= f_h then begin
                    break;
                end;
            end;

            GotoXY(x + pos - line_start_pos, y + row);
            Write(Utf8Encode(s));
        end;
    end;
end;

procedure TUIButton.draw(x, y: integer);
var
    s: UnicodeString;
    diff: integer;
begin
    if f_w > 2 then begin
        TextColor(color);
        TextBackground(bgcolor);
        FillRect(x, y, f_w, f_h);

        if Length(content) > f_w - 2 then begin
            s := '<' + Copy(content, 1, f_w - 2) + '>';
        end else begin
            diff := f_w - 2 - Length(content);
            s := '<' + StringOfChar(' ', diff div 2) + content +
                StringOfChar(' ', diff - (diff div 2)) + '>';
        end;

        GotoXY(x, y);
        Write(Utf8Encode(s));
    end;
end;

procedure TUIRadioButton.draw(x, y: integer);
var
    s: UnicodeString;
    diff: integer;
begin
    if f_w > 4 then begin
        TextColor(color);
        TextBackground(bgcolor);
        FillRect(x, y, f_w, f_h);

        if active then begin
            s := '(*) ';
        end else begin
            s := '( ) ';
        end;

        s := s + Copy(content, 1, f_w - 4);

        GotoXY(x, y);
        Write(Utf8Encode(s));
    end;
end;

constructor TUIMenu.create(num: integer);
var
    i: integer;
begin
    if num < 1 then begin
        raise Exception.create('TUIColContainer: num < 1');
    end;

    sel_color := 14;
    sel_bgcolor := 1;

    f_cur := 0;
    f_selected := 0;
    f_num := num;
    f_items := GetMem(sizeof(TMenuItem) * num);
    FillByte(f_items^, sizeof(TMenuItem) * num, 0);
end;

destructor TUIMenu.free;
begin
    FreeMem(f_items);
end;

procedure TUIMenu.draw(x, y: integer);
var
    i: integer;
    last: integer;
begin
    TextColor(color);
    TextBackground(bgcolor);
    FillRect(x, y, f_w, f_h);

    last := f_cur - 1;
    if last >= f_h then begin
        last := f_h - 1;
    end;

    for i := 0 to last do begin
        GotoXY(x, y + i);
        if f_selected = i then begin
            TextColor(sel_color);
            TextBackground(sel_bgcolor);
        end;
        WritePadded(f_items[i].s, f_w);
        if f_selected = i then begin
            TextColor(color);
            TextBackground(bgcolor);
        end;
    end;
end;

procedure TUIMenu.AddItem(s: UnicodeString; opaque: Pointer);
begin
    if f_cur < f_num then begin
        f_items[f_cur].s := s;
        f_items[f_cur].opaque := opaque;
        Inc(f_cur);
    end else begin
        raise ETUIContainerFull.create('Menue ist voll ("' + s + '")');
    end;
end;


initialization

    ClrScr;

finalization

    TextBackground(0);
    TextColor(7);
    WriteLn;

end.
