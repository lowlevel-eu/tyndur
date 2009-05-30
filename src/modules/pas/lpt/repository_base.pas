unit repository_base;
{$MODE ObjFPC}

interface
uses sysutils, classes, tar, packages, helpers;

type
    EDownload = class(Exception) end;
    EPackageList = class(Exception) end;
    EInvalidRepository = class(Exception) end;

    Trepostype = ( none, http, singlefile, files );

    TRepository = class(TCollectionItem)
        public
            name:   String;
            url:    String;


            (* Erzeugt ein Repository. *)
            constructor create(parent : TCollection);
            (* Liefert den Typ des Repository *)
            function repostype: Trepostype; virtual;
            (* Lädt die angegebene Paketversion aus dem Repository *)
            function Download(version: TPackageVersion): TTarArchive; virtual; abstract;
            (* Bereitet Paketlisten für das Repository vor (siehe lpt scan) *)
            procedure PrepareLists; virtual; abstract;
            (* Lädt die Paketliste des Repository in ein PackageSet *)
            procedure FetchLists(pkgset: TPackageSet); virtual; abstract;
    end;

implementation

constructor TRepository.create(parent: TCollection);
begin
    inherited;
end;

function TRepository.repostype: Trepostype;
begin
    exit(none);
end;

end.
