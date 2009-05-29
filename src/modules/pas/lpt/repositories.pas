unit repositories;
{$MODE ObjFPC}

interface
uses crt, sysutils, classes, helpers, repository_base, repository_http;

procedure ReadPkgsrc(filename: String);
procedure AddRepository(name, url: String; repostype: Trepostype);
function GetRepository(name: String): TRepository;
function GetAllRepositories(): TCollection;

implementation

var
    repos: TCollection;

procedure AddRepository(name, url: String; repostype: Trepostype);
var
    repo: TRepository;
begin
    if repostype = http then begin
        repo := THTTPRepository.create(repos);
    end else begin
        raise EInvalidRepository.create('Repository unbekannten Types!');
    end;
    repo.name := name;
    repo.url := url;
end;

function GetAllRepositories(): TCollection;
begin
    exit(repos);
end;

procedure ReadPkgsrc(filename: String);
var
    repo: TRepository;
    f: Text;
    name: String;
    url: String;
    repostype: Trepostype;
    space: integer;
begin
    Assign(f, filename);
    Reset(f);
    while not eof(f) do begin
        ReadLn(f, url);

        // Leerzeilen ignorieren
        if (url = '') or (url[0] = '#') then begin
            continue;
        end;

        // Name und URL am ersten Leerzeichen auftrennen
        space := Pos(' ', url);
        if  space > 0 then begin
            name := Copy(url, 1, space - 1);
            url := Copy(url, space + 1, length(url));
            if Pos('http://', url) > 0 then begin
                repostype := http;
            end else begin
                TextColor(12);
                WriteLn('Ungueltige Paketquelle: ' + url);
                TextColor(7);
                continue;
            end
        end else begin
            TextColor(12);
            WriteLn('Ungueltige Paketquelle: ' + url);
            TextColor(7);
            continue;
        end;

        // Neues Repository in die Liste aufnehmen
        AddRepository(name, url, repostype);
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
