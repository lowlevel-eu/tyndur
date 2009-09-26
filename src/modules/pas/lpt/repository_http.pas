unit repository_http;
{$MODE ObjFPC}

interface
uses crt, sysutils, classes, tar, packages, helpers, repository_base;

type
    THTTPRepository = class(TRepository)
        public
            constructor create(parent : TCollection);
            function repostype: Trepostype; override;
            function Download(version: TPackageVersion): TTarArchive; override;
            procedure PrepareLists; override;
            procedure FetchLists(pkgset: TPackageSet); override;
    end;

implementation

resourcestring
    rsDownloadingPkglist = 'Lade Paketliste %s von %s';

constructor THTTPRepository.create(parent : TCollection);
begin
    inherited;
end;

function THTTPRepository.repostype: Trepostype;
begin
    exit(http);
end;

function THTTPRepository.Download(version: TPackageVersion): TTarArchive;
var
    archiveURI: String;
begin
    archiveURI := self.url + version.pkg.name + '-'
        + version.section.section + '-' + version.version + '-' + getArch +
        '.tar';
    exit(DownloadToTarArchive(archiveURI));
end;

procedure THTTPRepository.PrepareLists;
var
    pkglisturl: String;
begin
    if self.url[length(self.url)] <> '/' then begin
        pkglisturl := self.url + '/packages.' + getArch;
    end else begin
        pkglisturl := self.url + 'packages.' + getArch;
    end;

    WriteLn(Format(rsDownloadingPkglist, [self.name, pkglisturl]));
    try
        DownloadToFile(pkglisturl, getConfigRoot + 'pkglist.' + self.name);
    except
        on e: Exception do begin
            raise EDownload.create(e.message);
        end;
    end;
end;

procedure THTTPRepository.FetchLists(pkgset: TPackageSet);
begin
    if FileExists(getConfigRoot + 'pkglist.' + self.name) then begin
        ScanFile(pkgset, getConfigRoot + 'pkglist.' + self.name, self.name);
    end else begin
        raise EPackageList.create(Format(rsNoPackageList, [self.name]));
    end;
end;

end.
