unit repositories;
{$MODE ObjFPC}

interface
uses sysutils, classes;

type
    TRepository = class(TCollectionItem)
        public
            name:   String;
            url:    String;
    end;


procedure ReadPkgsrc(filename: String);
function GetRepository(name: String): TRepository;


implementation

var
    repos: TCollection;

procedure ReadPkgsrc(filename: String);
var
    repo: TRepository;
    f: Text;
    name: String;
    url: String;
    space: integer;
begin
    Assign(f, filename);
    Reset(f);
    while not eof(f) do begin
        ReadLn(f, url);

        // Leerzeilen ignorieren
        if url = '' then begin
            continue;
        end;

        // Name und URL am ersten Leerzeichen auftrennen
        space := Pos(' ', url);
        if  space > 0 then begin
            name := Copy(url, 1, space - 1);
            url := Copy(url, space + 1, length(url));
        end else begin
            WriteLn('Ungueltige Paketquelle: ' + url);
            continue;
        end;

        // Neues Repository in die Liste aufnehmen
        repo := TRepository(repos.add());
        repo.name := name;
        repo.url := url;
    end;
    Close(f);
end;

function GetRepository(name: String): TRepository;
var
    i:      integer;
    repo:   TRepository;
begin
    for i := 0 to repos.count - 1 do begin
        repo := TRepository(repos.items[i]);
        if repo.name = name then begin
            exit(repo);
        end;
    end;

    exit(nil);
end;

begin
    repos := TCollection.create(TRepository);
end.
