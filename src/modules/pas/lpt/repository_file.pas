unit repository_file;
{$MODE ObjFPC}

interface
uses crt, sysutils, classes, tar, packages, helpers, repository_base;

type
    TFileRepository = class(TRepository)
        public
            constructor create(parent : TCollection);
            function repostype: Trepostype; override;
            function Download(version: TPackageVersion): TTarArchive; override;
            procedure PrepareLists; override;
            procedure FetchLists(pkgset: TPackageSet); override;
    end;

implementation

constructor TFileRepository.create(parent : TCollection);
begin
    inherited;
end;

function TFileRepository.repostype: Trepostype;
begin
    exit(files);
end;

function TFileRepository.Download(version: TPackageVersion): TTarArchive;
var
    archiveURI: String;
begin
    archiveURI := self.url + version.pkg.name + '-'
        + version.section.section + '-' + version.version + '-' + getArch +
        '.tar';
    exit(TTarArchive.create(archiveURI));
end;

procedure TFileRepository.PrepareLists;
begin
    (* Die Paketliste liegt bereits lokal, wozu also kopieren? *)
    exit;
end;

procedure TFileRepository.FetchLists(pkgset: TPackageSet);
begin
    if FileExists(self.url + 'packages.' + getArch) then begin
        ScanFile(pkgset, self.url + 'packages.' + getArch, self.name);
    end else begin
        raise EPackageList.create(Format(rsNoPackageList, [self.name]));
    end;
end;

end.
