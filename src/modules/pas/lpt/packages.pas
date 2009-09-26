unit packages;
{$MODE ObjFPC}

interface
uses sysutils, classes, tar;

type
    EPackageFileFormat = class(Exception) end;
    EDependency = class(Exception) end;

    TPackage = class;
    TPackageSection = class;
    TPackageVersion = class;

    TDependency = class(TCollectionItem)
        public
            pkgname:    String;
            section:    String;
    end;

    TPackageSet = class
        public
            constructor create;
            destructor destroy; override;

            function add(name: String): TPackage;
            procedure AddAll(pkgset: TPackageSet);
            procedure AddVersion(ver: TPackageVersion);
            procedure AddDependencies(from: TPackageSet);
            function GetCurrentVersion
                (pkgname, sectname: String): TPackageVersion;

        public
            packages:   TCollection;
    end;

    TPackage = class(TCollectionItem)
        private
            function GetSection(sectname: String): TPackageSection;

        public
            constructor create(parent: TCollection); override;
            destructor destroy; override;

            function add(name: String): TPackageSection;

        public
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

        public
            section:    String;
            versions:   TCollection;
    end;

    TPackageVersion = class(TCollectionItem)
        private
            instDep: TCollection;
            runDep: TCollection;

        public
            constructor create(parent: TCollection); override;

            procedure addDependency(typ: char; dep: String);
            procedure copyDependencies(from: TPackageVersion);

            property preinstDependencies: TCollection read instDep;
            property runDependencies: TCollection read runDep;

        public
            repository: String;
            version:    String;
            size:       dword;
            pkg:        TPackage;
            section:    TPackageSection;
    end;

implementation

resourcestring
    rsPackageMissing = 'Paket %s fehlt';

constructor TPackageSet.create;
begin
    packages := TCollection.create(TPackage);
end;
        
destructor TPackageSet.destroy;
begin
    packages.free;
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
            
procedure TPackageSet.AddAll(pkgset: TPackageSet);
var
    i: integer;
    pkg, newPkg: TPackage;    
begin
    for i := 0 to pkgset.packages.count - 1 do begin
        pkg := TPackage(pkgset.packages.items[i]);

        newPkg := TPackage(packages.add());
        newPkg.sections.free();

        newPkg.name := pkg.name;
        newPkg.desc := pkg.desc;
        newPkg.sections := pkg.sections;
    end;
end;

procedure TPackageSet.AddVersion(ver: TPackageVersion);
var
    i: integer;
    pkg: TPackage;    
    sect: TPackageSection;
    newVer: TPackageVersion;
begin
    pkg := nil;
    sect := nil;

    for i := 0 to packages.count - 1 do begin
        pkg := TPackage(packages.items[i]);
        if pkg.name = ver.pkg.name then begin
            break;
        end;
    end;

    if (pkg = nil) or (pkg.name <> ver.pkg.name) then begin
        pkg := TPackage(packages.add());
        pkg.name := ver.pkg.name;
        pkg.desc := ver.pkg.desc;
    end;
    
    for i := 0 to pkg.sections.count - 1 do begin
        sect := TPackageSection(pkg.sections.items[i]);
        if sect.section = ver.section.section then begin
            break;
        end;
    end;
    
    if (sect = nil) or (sect.section <> ver.section.section) then begin
        sect := TPackageSection(pkg.add(ver.section.section));
        sect.section := ver.section.section;
    end;

    newVer := sect.add(ver.version);
    newVer.pkg := pkg;
    newVer.section := sect;
    newVer.repository := ver.repository;
    newVer.size := ver.size;    
    newVer.copyDependencies(ver);
end;
            
procedure TPackageSet.AddDependencies(from: TPackageSet);
var
    pkg: TPackage;
    sect: TPackageSection;
    ver: TPackageVersion;
    dep: TDependency;

    depPkg: TPackageVersion;

    i, j, k, l: integer;
    changed: boolean;
begin
    repeat
        changed := false;
        for i := 0 to packages.count - 1 do begin
            pkg := TPackage(packages.items[i]);
            for j := 0 to pkg.sections.count - 1 do begin
                sect := TPackageSection(pkg.sections.items[j]);
                for k := 0 to sect.versions.count - 1 do begin
                    ver := TPackageVersion(sect.versions.items[k]);
                    
                    for l := 0 to ver.runDependencies.count - 1 do begin
                        dep := TDependency(ver.runDependencies.items[l]);
                        depPkg := from.GetCurrentVersion(dep.pkgname, dep.section);
                        if GetCurrentVersion(dep.pkgname, dep.section) <> nil then begin
                            // Das Paket ist schon drin
                        end else if depPkg <> nil then begin
                            AddVersion(depPkg);
                            changed := true;
                        end else begin
                            raise EDependency.create(Format(rsPackageMissing,
                                [dep.pkgname]));
                        end;
                    end;
                end;
            end;
        end;
    until not changed;
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
    instDep := TCollection.create(TDependency);
    runDep := TCollection.create(TDependency);
end;

procedure TPackageVersion.addDependency(typ: char; dep: String);
var
    depList: TCollection;
    newDep: TDependency;
    i: integer;
begin
    case typ of
        'i': 
            depList := instDep;

        'r':
            depList := runDep;
            
        else
            raise EPackageFileFormat.create(
                'Unbekannter Abhaengigkeitstyp ' + typ);
    end;

    i := Pos('/', dep);
    if i <= 0 then begin
        raise EPackageFileFormat.create(
            'Kategorie ist nicht angegeben');
    end;

    newDep := TDependency(depList.add());
    newDep.pkgname := Copy(dep, 1, i - 1);
    newDep.section := Copy(dep, i + 1, length(dep));
end;

procedure TPackageVersion.copyDependencies(from: TPackageVersion);
var
    i: integer;
    dep, newDep: TDependency;
begin
    for i := 0 to from.runDependencies.count - 1 do begin
        dep := TDependency(from.runDependencies.items[i]);
        newDep := TDependency(runDep.add());
        newDep.pkgname := dep.pkgname;
        newDep.section := dep.section;
    end;
    
    for i := 0 to from.preinstDependencies.count - 1 do begin
        dep := TDependency(from.preinstDependencies.items[i]);
        newDep := TDependency(instDep.add());
        newDep.pkgname := dep.pkgname;
        newDep.section := dep.section;
    end;
end;

end.
