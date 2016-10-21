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
    rsDHCP = 'Über DHCP konfigurieren';
    rsStatic = 'Statische Konfiguration';
    rsIPAddress = 'IP-Adresse';
    rsGateway = 'Gateway';

    rsOK = 'OK';
    rsCancel = 'Abbrechen';

function c_servmgr_need(service_name: PChar): boolean; cdecl; external name 'servmgr_need';

procedure SetNetworkConfig(dhcp: boolean; driver, ip, gateway: AnsiString);
var
    path: String;
    f: text;
begin
    if driver <> '' then begin
        // Treiber laden
        c_servmgr_need(@driver[1]);

        // IP/Gateway setzen
        path := 'tcpip:/' + driver + '/0/ip';
        while not FileExists(path) do begin
            yield;
        end;

        Assign(f, path);
        {$i-}
        Rewrite(f);
        {$i+}
        if IOResult = 0 then begin
            WriteLn(f, ip);
            Close(f);
        end;
        IOResult; // Fehlercode ggf. zurücksetzen
    end;

    path := 'tcpip:/route';
    Assign(f, path);
    Rewrite(f);
    WriteLn(f, '0.0.0.0 ', gateway, ' 0.0.0.0');
    Close(f);

    if dhcp then begin
        path := 'tcpip:/' + driver + '/0/dhcp';
        Assign(f, path);
        {$i-}
        Rewrite(f);
        WriteLn(f, 'configure');
        Close(f);
        {$i+}
        IOResult; // Fehlercode ggf. zurücksetzen
    end;

    // Dauerhaft speichern
    path := 'file:/config/servmgr/tcpip/cmd';
    Assign(f, path);
    Rewrite(f);

    if dhcp then begin
        WriteLn(f, '/modules/tcpip ip=', ip, ' gw=', gateway, ' dhcp=1');
    end else begin
        WriteLn(f, '/modules/tcpip ip=', ip, ' gw=', gateway, ' dhcp=0');
    end;
    Close(f);

    path := 'file:/config/servmgr/nic/deps';
    Assign(f, path);
    Rewrite(f);
    WriteLn(f, driver);
    Close(f);
end;

procedure GetNetworkConfig(var dhcp: boolean; var driver, ip, gateway: AnsiString);
var
    path: String;
    f: text;
    s: String;
    space: integer;
begin
    ip := '10.0.2.14';
    gateway := '10.0.2.2';
    dhcp := true;

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
        end else if Copy(s, 1, 5) = 'dhcp=' then begin
            dhcp := (Copy(s, 6, space - 6) <> '0');
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

procedure DetectNetworkConfig(var dhcp: boolean; var driver, ip, gateway: AnsiString);
var
    srec: SearchRec;
begin
    GetNetworkConfig(dhcp, driver, ip, gateway);
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
        end else if srec.name = '8086:10d3' then begin
            driver := 'e1000';
        end else if srec.name = '8086:10f5' then begin
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
    txtDriverValue:     TUITextField;

    radDHCP:            TUIRadioButton;
    radStatic:          TUIRadioButton;
    cntStatic:          TUIRowContainer;
    frmStatic:          TUIFrame;

    cntIP:              TUIColContainer;
    txtIP:              TUILabel;
    txtIPValue:         TUITextField;

    cntGateway:         TUIColContainer;
    txtGateway:         TUILabel;
    txtGatewayValue:    TUITextField;

    buttons:            TUIColContainer;
    btnOk:              TUIButton;
    btnCancel:          TUIButton;

    screen: TUIRowContainer;
    title: TUILabel;
    f: TUIFrame;


const
    num_items = 7;
var
    items: Array[0..num_items - 1] of TUILabel;
    cur_item: integer;
    i: integer;

    driver, ip, gateway: AnsiString;
    dhcp: boolean;

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

    f := TUIFrame.create(nil, 60, 16);
    screen[1] := f;
    screen.prop[1] := 1;

    mainw := TUIRowContainer.create(9);
    f.obj := mainw;

    b := TUILabel.create;
    b.content := Utf8Decode(rsConfigureNetwork);
    mainw[0] := b;
    mainw.prop[0] := 30;

    // Treiberauswahl
    txtDriver := TUILabel.create;
    txtDriver.content := rsDriver + ':';

    txtDriverValue := TUITextField.create;
    txtDriverValue.bgcolor := 1;
    txtDriverValue.color := 14;

    cntDriver := TUIColContainer.create(2);
    cntDriver.prop[0] := 1;
    cntDriver.prop[1] := 1;
    cntDriver[0] := txtDriver;
    cntDriver[1] := txtDriverValue;

    mainw[1] := cntDriver;
    mainw.fixed[1] := 1;

    mainw[2] := nil;
    mainw.fixed[2] := 1;

    // DHCP
    radDHCP := TUIRadioButton.create;
    radDHCP.content := Utf8Decode(rsDHCP);
    radDHCP.active := true;
    mainw[3] := radDHCP;
    mainw.fixed[3] := 1;

    // Statische Adresse
    radStatic:= TUIRadioButton.create;
    radStatic.content := Utf8Decode(rsStatic);
    radStatic.active := false;
    mainw[4] := radStatic;
    mainw.fixed[4] := 1;

    mainw[5] := nil;
    mainw.fixed[5] := 1;

    cntStatic := TUIRowContainer.create(2);
    frmStatic := TUIFrame.create(cntStatic, 50, 4);
    mainw[6] := frmStatic;
    mainw.fixed[6] := 4;

    // IP-Adresse
    txtIP := TUILabel.create;
    txtIP.content := rsIPAddress + ':';
    txtIP.color := 8;

    txtIPValue := TUITextField.create;
    txtIPValue.color := 8;

    cntIP := TUIColContainer.create(2);
    cntIP.prop[0] := 1;
    cntIP.prop[1] := 1;
    cntIP[0] := txtIP;
    cntIP[1] := txtIPValue;

    cntStatic[0] := cntIP;
    cntStatic.fixed[0] := 1;

    // Gateway
    txtGateway := TUILabel.create;
    txtGateway.content := rsGateway + ':';
    txtGateway.color := 8;

    txtGatewayValue := TUITextField.create;
    txtGatewayValue.color := 8;

    cntGateway := TUIColContainer.create(2);
    cntGateway.prop[0] := 1;
    cntGateway.prop[1] := 1;
    cntGateway[0] := txtGateway;
    cntGateway[1] := txtGatewayValue;

    cntStatic[1] := cntGateway;
    cntStatic.fixed[1] := 1;

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

    mainw[7] := TUIObject.create;
    mainw[8] := buttons;
    mainw.prop[7] := 10;
    mainw.fixed[8] := 1;


    // Reihenfolge der Felder
    cur_item := 5;
    items[0] := txtDriverValue;
    items[1] := radDHCP;
    items[2] := radStatic;
    items[3] := txtIPValue;
    items[4] := txtGatewayValue;
    items[5] := btnOK;
    items[6] := btnCancel;

    // Standardwerte setzen
    if autodetect then begin
        DetectNetworkConfig(dhcp, driver, ip, gateway);
    end else begin
        GetNetworkConfig(dhcp, driver, ip, gateway);
    end;

    txtDriverValue.content := driver;
    txtIPValue.content := ip;
    txtGatewayValue.content := gateway;

    if not dhcp then begin
        radDHCP.active := false;
        radStatic.active := true;

        txtIP.color := 7;
        txtIPValue.color := 14;
        txtGateway.color := 7;
        txtGatewayValue.color := 14;
    end;

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
                    SK_UP: begin
                        cur_item := (cur_item + num_items - 1) mod num_items;
                        if (cur_item = 4) and not radStatic.active then begin
                            cur_item := 2;
                        end;
                    end;
                    SK_DOWN: begin
                        cur_item := (cur_item + 1) mod num_items;
                        if (cur_item = 3) and not radStatic.active then begin
                            cur_item := 5;
                        end;
                    end;
                end;
            #9:
                begin
                    cur_item := (cur_item + 1) mod num_items;
                    if (cur_item = 3) and not radStatic.active then begin
                        cur_item := 5;
                    end;
                end;
            #10:
                case cur_item of
                    1:
                        begin
                            radDHCP.active := true;
                            radStatic.active := false;

                            txtIP.color := 8;
                            txtIPValue.color := 8;
                            txtGateway.color := 8;
                            txtGatewayValue.color := 8;
                        end;
                    2:
                        begin
                            radDHCP.active := false;
                            radStatic.active := true;

                            txtIP.color := 7;
                            txtIPValue.color := 14;
                            txtGateway.color := 7;
                            txtGatewayValue.color := 14;
                        end;
                    5:
                        begin
                            SetNetworkConfig(radDHCP.active,
                                txtDriverValue.content, txtIPValue.content,
                                txtGatewayValue.content);
                            break;
                        end;
                    6:  break;
                end;
            'A'..'Z', 'a'..'z', '0'..'9', '.':
                if items[cur_item] is TUITextField then begin
                    items[cur_item].content := items[cur_item].content + c;
                end;
            #8:
                if items[cur_item] is TUITextField then begin
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
