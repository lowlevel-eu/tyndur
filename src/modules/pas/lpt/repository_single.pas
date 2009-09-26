unit repository_single;
{$MODE ObjFPC}

interface
uses sysutils, classes, tar, packages, helpers, repository_base;

type
    TSFRepository = class(TRepository)
        public
            function repostype: Trepostype; virtual;
            constructor create(parent : TCollection);
            function Download(version: TPackageVersion): TTarArchive; override;
            procedure PrepareLists; override;
            procedure FetchLists(pkgset: TPackageSet); override;
    end;

implementation

resourcestring
    rsLocalPkgDesc = 'Keine (lokale Datei)';

constructor TSFRepository.create(parent : TCollection);
begin
    inherited;
end;

function TSFRepository.repostype: Trepostype;
begin
    exit(singlefile);
end;

function TSFRepository.Download(version: TPackageVersion): TTarArchive;
var
    tar: TTarArchive;
begin
    (* Einzelpakete enthalten nur sich selbst. Nur als Tar öffnen reicht. *)
    tar := TTarArchive.create(self.url);
    exit(tar);
end;

procedure TSFRepository.PrepareLists;
begin
    (* Einzelpakete haben keine Listen. *)
    exit;
end;

procedure TSFRepository.FetchLists(pkgset: TPackageSet);
var
    package: TTarArchive;
    line: String;
    i: integer;
    size: integer;

    package_file_string: String;
    pkg_name, pkg_version, pkg_section, pkg_arch: String;

    pkg: TPackage;
    section: TPackageSection;
    version: TPackageVersion;
begin
    package := TTarArchive.create(self.url);
    while package.hasNext do begin
        line := package.nextFilename;
        size := package.nextSize;
        i := Pos('packageinfo', line);
        if i > 0 then begin
            (* Metadaten laden *)
            package_file_string :=  package.ExtractString();

            (* Newline-getrennte Infos auslesen. *)
            (* In Reihenfolge: Name, Version, Sektion, Architektur *)
            i := Pos(#10, package_file_string);
            pkg_name := Copy(package_file_string, 1, i - 1);
            package_file_string :=
                Copy(package_file_string, i + 1, Length(package_file_string));

            i := Pos(#10, package_file_string);
            pkg_version := Copy(package_file_string, 1, i - 1);
            package_file_string :=
                Copy(package_file_string, i + 1, Length(package_file_string));

            i := Pos(#10, package_file_string);
            pkg_section := Copy(package_file_string, 1, i - 1);
            package_file_string :=
                Copy(package_file_string, i + 1, Length(package_file_string));

            i := Pos(#10, package_file_string);
            pkg_arch := Copy(package_file_string, 1, i - 1);
            package_file_string :=
                Copy(package_file_string, i + 1, Length(package_file_string));

            (* Prüfen ob Metadaten zu Paket passen *)
            if (line = 'packages/'+pkg_name+'/'+pkg_version+'/packageinfo-'+pkg_section) then begin
                pkg := pkgset.add(pkg_name);
                pkg.desc := rsLocalPkgDesc;
                section := pkg.add(pkg_section);
                version := section.add(pkg_version);
                version.pkg := pkg;
                version.section := section;
                version.repository := self.name;
                break;
            end;
        end else begin
            package.skipFile();
        end;
    end;
    package.destroy();
end;

begin
end.
