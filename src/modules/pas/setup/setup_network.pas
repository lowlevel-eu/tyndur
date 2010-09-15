unit setup_network;
{$MODE ObjFPC}

interface

procedure SetupNetwork(back, autodetect: boolean);

implementation

uses dos, sysutils, tyndur, crt, tui;

resourcestring
    rsTitle = 'tyndur-Konfiguration: Netzwerk';
    rsConfigureNetwork = 'Bitte in folgende Felder die Netzwerkeinstellungen eintragen:';

    rsDriver = 'Treiber';
    rsIPAddress = 'IP-Adresse';
    rsGateway = 'Gateway';

    rsOK = 'OK';
    rsCancel = 'Abbrechen';

function c_servmgr_need(service_name: PChar): boolean; cdecl; external name 'servmgr_need';

procedure SetNetworkConfig(driver, ip, gateway: AnsiString);
var
    path: String;
    f: text;
begin
    if driver <> '' then begin
        // Treiber laden
        c_servmgr_need(@driver[1]);

        // IP/Gateway setzen
        path := 'tcpip:/' + driver + '/0/ip';
        Assign(f, path);
        {$i-}
        Rewrite(f);
        {$i+}
        if IOResult = 0 then begin
            WriteLn(f, ip);
            Close(f);
        end;
    end;

    path := 'tcpip:/route';
    Assign(f, path);
    Rewrite(f);
    WriteLn(f, '0.0.0.0 ', gateway, ' 0.0.0.0');
    Close(f);

    // Dauerhaft speichern
    path := 'file:/config/servmgr/tcpip/cmd';
    Assign(f, path);
    Rewrite(f);
    WriteLn(f, '/modules/tcpip ip=', ip, ' gw=', gateway);
    Close(f);

    path := 'file:/config/servmgr/nic/deps';
    Assign(f, path);
    Rewrite(f);
    WriteLn(f, driver);
    Close(f);
end;

procedure GetNetworkConfig(var driver, ip, gateway: AnsiString);
var
    path: String;
    f: text;
    s: String;
    space: integer;
begin
    ip := '10.0.2.14';
    gateway := '10.0.2.2';

    // IP und Gateway aus dem tcpip-Aufruf rauspfriemeln
    path := 'file:/config/servmgr/tcpip/cmd';
    Assign(f, path);
    Reset(f);
    ReadLn(f, s);
    Close(f);

    repeat
        space := Pos(' ' , s);
        if space <= 0 then begin
            space := Length(s) + 1;
        end;

        if Copy(s, 1, 3) = 'ip=' then begin
            ip := Copy(s, 4, space - 4);
        end else if Copy(s, 1, 3) = 'gw=' then begin
            gateway := Copy(s, 4, space - 4);
        end;

        s := Copy(s, space + 1, Length(s));
    until s = '';

    // Der Treiber steht in den Abhaengigkeiten von nic
    path := 'file:/config/servmgr/nic/deps';
    Assign(f, path);
    Reset(f);
    ReadLn(f, driver);
    Close(f);
end;

procedure DetectNetworkConfig(var driver, ip, gateway: AnsiString);
var
    srec: SearchRec;
begin
    GetNetworkConfig(driver, ip, gateway);
    driver := '';

    dos.FindFirst('pci:/devices/*', 0, srec);
    while (driver = '') and (DosError = 0) do begin
        if srec.name = '10ec:8139' then begin
            driver := 'rtl8139';
        end else if srec.name = '10ec:8029' then begin
            driver := 'ne2k';
        end else if srec.name = '1039:0900' then begin
            driver := 'sis900';
        end else if srec.name = '8086:1004' then begin
            driver := 'e1000';
        end else if srec.name = '8086:100e' then begin
            driver := 'e1000';
        end else if srec.name = '8086:100f' then begin
            driver := 'e1000';
        end else if srec.name = '1022:2000' then begin
            driver := 'pcnet';
        end;
        dos.FindNext(srec);
    end;
    dos.FindClose(srec);
end;

procedure SetupNetwork(back, autodetect: boolean);
var
    mainw: TUIRowContainer;

    b: TUILabel;

    cntDriver:          TUIColContainer;
    txtDriver:          TUILabel;
    txtDriverValue:     TUILabel;

    cntIP:              TUIColContainer;
    txtIP:              TUILabel;
    txtIPValue:         TUILabel;

    cntGateway:         TUIColContainer;
    txtGateway:         TUILabel;
    txtGatewayValue:    TUILabel;

    buttons:            TUIColContainer;
    btnOk:              TUIButton;
    btnCancel:          TUIButton;

    screen: TUIRowContainer;
    title: TUILabel;
    f: TUIFrame;


const
    num_items = 5;
var
    items: Array[0..num_items - 1] of TUILabel;
    cur_item: integer;
    i: integer;

    driver, ip, gateway: AnsiString;

    c: char;
begin
    screen := TUIRowContainer.create(2);
    screen.bgcolor := 2;
    screen.color := 1;

    title := TUILabel.create;
    title.content := rsTitle;
    title.bgcolor := 1;
    title.color := 15;
    screen[0] := title;
    screen.fixed[0] := 1;

    f := TUIFrame.create(nil, 60, 12);
    screen[1] := f;
    screen.prop[1] := 1;

    mainw := TUIRowContainer.create(6);
    f.obj := mainw;

    b := TUILabel.create;
    b.content := Utf8Decode(rsConfigureNetwork);
    mainw[0] := b;
    mainw.prop[0] := 30;

    // Treiberauswahl
    txtDriver := TUILabel.create;
    txtDriver.content := rsDriver + ':';

    txtDriverValue := TUILabel.create;
    txtDriverValue.bgcolor := 1;
    txtDriverValue.color := 14;

    cntDriver := TUIColContainer.create(2);
    cntDriver.prop[0] := 1;
    cntDriver.prop[1] := 1;
    cntDriver[0] := txtDriver;
    cntDriver[1] := txtDriverValue;

    mainw[1] := cntDriver;
    mainw.fixed[1] := 1;

    // IP-Adresse
    txtIP := TUILabel.create;
    txtIP.content := rsIPAddress + ':';

    txtIPValue := TUILabel.create;
    txtIPValue.color := 14;

    cntIP := TUIColContainer.create(2);
    cntIP.prop[0] := 1;
    cntIP.prop[1] := 1;
    cntIP[0] := txtIP;
    cntIP[1] := txtIPValue;

    mainw[2] := cntIP;
    mainw.fixed[2] := 1;

    // Gateway
    txtGateway := TUILabel.create;
    txtGateway.content := rsGateway + ':';

    txtGatewayValue := TUILabel.create;
    txtGatewayValue.color := 14;

    cntGateway := TUIColContainer.create(2);
    cntGateway.prop[0] := 1;
    cntGateway.prop[1] := 1;
    cntGateway[0] := txtGateway;
    cntGateway[1] := txtGatewayValue;

    mainw[3] := cntGateway;
    mainw.fixed[3] := 1;

    // OK / Abbrechen
    btnOK := TUIButton.create;
    btnOK.content := rsOK;
    btnOK.color := 15;

    btnCancel := TUIButton.create;
    btnCancel.content := rsCancel;
    btnCancel.color := 15;

    buttons := TUIColContainer.create(5);

    buttons[0] := TUIObject.create;
    buttons[2] := TUIObject.create;
    buttons[4] := TUIObject.create;

    buttons.prop[0] := 1;
    buttons.prop[2] := 1;
    buttons.prop[4] := 1;

    buttons[1] := btnOK;
    buttons[3] := btnCancel;

    buttons.fixed[1] := 15;
    buttons.fixed[3] := 15;

    mainw[4] := TUIObject.create;
    mainw[5] := buttons;
    mainw.prop[4] := 10;
    mainw.fixed[5] := 1;


    // Reihenfolge der Felder
    cur_item := 3;
    items[0] := txtDriverValue;
    items[1] := txtIPValue;
    items[2] := txtGatewayValue;
    items[3] := btnOK;
    items[4] := btnCancel;

    // Standardwerte setzen
    if autodetect then begin
        DetectNetworkConfig(driver, ip, gateway);
    end else begin
        GetNetworkConfig(driver, ip, gateway);
    end;

    txtDriverValue.content := driver;
    txtIPValue.content := ip;
    txtGatewayValue.content := gateway;

    repeat
        // Ausgewaehltes Widget markieren
        for i := 0 to num_items - 1 do begin
            if i = cur_item then begin
                items[i].bgcolor := 1;
            end else begin
                items[i].bgcolor := 0;
            end;
        end;

        // Zeichnen
        screen.align(80, 24);
        screen.draw(1, 1);

        // Eingabe verarbeiten
        c := readkey;
        case c of
            #27:
                case ReadEscapeSequence of
                    SK_UP:      cur_item := (cur_item + num_items - 1)
                                    mod num_items;
                    SK_DOWN:    cur_item := (cur_item + 1) mod num_items;
                end;
            #9:
                cur_item := (cur_item + 1) mod num_items;
            #10:
                case cur_item of
                    3:
                        begin
                            SetNetworkConfig(txtDriverValue.content,
                                txtIPValue.content, txtGatewayValue.content);
                            break;
                        end;
                    4:  break;
                end;
            'A'..'Z', 'a'..'z', '0'..'9', '.':
                if not (items[cur_item] is TUIButton) then begin
                    items[cur_item].content := items[cur_item].content + c;
                end;
            #8:
                if not (items[cur_item] is TUIButton) then begin
                    items[cur_item].content := Copy(items[cur_item].content, 1,
                        Length(items[cur_item].content) - 1);
                end;
        end;
    until false;

    f.obj := nil;
    mainw.free;
    screen.free;
end;

end.
