unit packages;
{$MODE ObjFPC}

interface
uses sysutils, classes;

type
    EPackageFileFormat = class(Exception) end;

    TPackage = class;
    TPackageSection = class;
    TPackageVersion = class;

    TPackageSet = class
        public
            constructor create;
            destructor destroy; override;

            procedure ScanFile(filename, repository: String);
            function add(name: String): TPackage;
            function GetCurrentVersion
                (pkgname, sectname: String): TPackageVersion;

            packages:   TCollection;
    end;

    TPackage = class(TCollectionItem)
        private
            function GetSection(sectname: String): TPackageSection;

        public
            constructor create(parent: TCollection); override;
            destructor destroy; override;

            function add(name: String): TPackageSection;

            name:       String;
            desc:       String;
            sections:   TCollection;

            property section[sectname: String]: TPackageSection 
                read GetSection;
    end;

    TPackageSection = class(TCollectionItem)
        public
            constructor create(parent: TCollection); override;
            destructor destroy; override;

            function add(number: String): TPackageVersion;

            section:    String;
            versions:   TCollection;
    end;

    TPackageVersion = class(TCollectionItem)
        private
            function GetPackage: TPackage;
            function GetSection: TPackageSection;

        public
            constructor create(parent: TCollection); override;

            property pkg: TPackage read GetPackage;
            property section: TPackageSection read GetSection;

            repository: String;
            version:    String;
            size:       dword;
    end;

implementation
    
constructor TPackageSet.create;
begin
    packages := TCollection.create(TPackage);
end;
        
destructor TPackageSet.destroy;
begin
    packages.free;
end;

procedure TPackageSet.ScanFile(filename, repository: String);
var
    f: Text;
    line: String;
    status: integer;

    pkg: TPackage;
    section: TPackageSection;
    version: TPackageVersion;
begin
    Assign(f, filename);
    Reset(f);
    status := 0;
    pkg := nil;
    try
        while not eof(f) do begin
            ReadLn(f, line);
            
            if line = '' then begin
                continue;
            end;

            case line[1] of
                'P':
                    begin
                        status := 1;
                        pkg := self.add(Copy(line, 3, length(line)));
                    end;
                'D':
                    if status = 1 then begin
                        pkg.desc := Copy(line, 3, length(line));
                    end else begin
                        raise EPackageFileFormat.create(
                            'D in status ' + IntToStr(status));
                    end;
                'S':
                    if status >= 1 then begin
                        status := 2;
                        section := pkg.add(Copy(line, 3, length(line)));
                    end else begin
                        raise EPackageFileFormat.create(
                            'S in status ' + IntToStr(status));
                    end;
                'V':
                    if status >= 2 then begin
                        status := 3;
                        version := section.add(Copy(line, 3, length(line)));
                        version.repository := repository;
                    end else begin
                        raise EPackageFileFormat.create(
                            'V in status ' + IntToStr(status));
                    end;
                's':
                    if status >= 3 then begin
                        version.size := StrToInt(Copy(line, 3, length(line)));
                    end else begin
                        raise EPackageFileFormat.create(
                            's in status ' + IntToStr(status));
                    end;
                else
                    begin
                        raise EPackageFileFormat.create(
                            'Unbekannter Schluessel: ' + line[1]);
                    end;
            end;
        end;
    finally
        Close(f);
    end;
end;

function TPackageSet.add(name: String): TPackage;
var
    pkg: TPackage;
begin
    pkg := TPackage(packages.add());
    pkg.name := name;

    exit(pkg);
end;
            
function TPackageSet.GetCurrentVersion
    (pkgname, sectname: String): TPackageVersion;
var
    i: integer;
    pkg: TPackage;
    sect: TPackageSection;
begin
    for i := 0 to packages.count - 1 do begin
        pkg := TPackage(packages.items[i]);
        if pkg.name = pkgname then begin
            sect := pkg.section[sectname];
            if (sect <> nil) and (sect.versions.count > 0) then begin
                // TODO Aktuellste Version nehmen
                exit(TPackageVersion(sect.versions.items[0]));
            end;
        end;
    end;

    exit(nil);
end;

constructor TPackage.create(parent: TCollection);
begin
    inherited;
    name := '';
    desc := '';
    sections := TCollection.create(TPackageSection);
end;
        
destructor TPackage.destroy;
begin
    sections.free;
end;

function TPackage.add(name: String): TPackageSection;
var
    sect: TPackageSection;
begin
    sect := TPackageSection(sections.add());
    sect.section := name;

    exit(sect);
end;
            
function TPackage.GetSection(sectname: String): TPackageSection;
var
    i: integer;
    sect: TPackageSection;
begin
    for i := 0 to sections.count - 1 do begin
        sect := TPackageSection(sections.items[i]);
        if sect.section = sectname then begin
            exit(sect);
        end;
    end;

    exit(nil);
end;

constructor TPackageSection.create(parent: TCollection);
begin
    inherited;
    section := '';    
    versions := TCollection.create(TPackageVersion);
end;

destructor TPackageSection.destroy;
begin
    versions.free;
end;

function TPackageSection.add(number: String): TPackageVersion;
var
    version: TPackageVersion;
begin
    version := TPackageVersion(versions.add());
    version.version := number;

    exit(version);
end;

constructor TPackageVersion.create(parent: TCollection);
begin
    inherited;
    version := '';
end;
            
function TPackageVersion.GetPackage: TPackage;
begin
    GetPackage := TPackage(section.collection);
end;

function TPackageVersion.GetSection: TPackageSection;
begin
    GetSection := TPackageSection(self.collection);
end;

end.
