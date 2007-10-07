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

            procedure ScanFile(filename: String);
            function add(name: String): TPackage;

            packages:   TCollection;
    end;

    TPackage = class(TCollectionItem)
        public
            constructor create(parent: TCollection); override;
            destructor destroy; override;

            function add(name: String): TPackageSection;

            name:       String;
            desc:       String;
            sections:   TCollection;
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
        public
            constructor create(parent: TCollection); override;

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

procedure TPackageSet.ScanFile(filename: String);
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
    section: TPackageSection;
begin
    section := TPackageSection(sections.add());
    section.section := name;

    exit(section);
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
    version := '';    
end;

end.
