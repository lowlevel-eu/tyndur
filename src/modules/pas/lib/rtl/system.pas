{ no stack check in system }
{S-}
unit system;

{$DEFINE ENDIAN_LITTLE}

interface

{ include system-independent routine headers }

{$I systemh.inc}

type
    TSize = dword;

const
 LineEnding = #10;
 LFNSupport = true;
 DirectorySeparator = '/';
 DriveSeparator = ':';
 ExtensionSeparator = '.';
 PathSeparator = ';';
 AllFilesMask = '*';
{ FileNameCaseSensitive is defined separately below!!! }
// cHEAPSIZE = 512*1024;
 MaxExitCode = 255;
 MaxPathLen = 255;
 CtrlZMarksEOF: boolean = false; (* #26 is considered as end of file *)

 AllowDirectorySeparators: set of char = ['/'];
 AllowDriveSeparators: set of char = [':', '|'];

//var
//  HEAP: array [0..cHEAPSIZE-1] of byte;

{ include heap support headers }

const
{ Default filehandles }
  UnusedHandle    : THandle = 0;
  StdInputHandle  : THandle = 0;
  StdOutputHandle : THandle = 0;
  StdErrorHandle  : THandle = 0;

  FileNameCaseSensitive : boolean = true;

  sLineBreak = LineEnding;
  DefaultTextLineBreakStyle : TTextLineBreakStyle = tlbsLF;

var
    argc: dword;
    argv: PPChar;
    commandline: PChar;

    envp: Pointer;

procedure DisableFlushing(var f: Text);

implementation

var
    ppb_size: integer; external name '__ppb_size';
    ppb_ptr: Pointer; external name '__ppb_ptr';
    ppb_shm_id: integer; external name '__ppb_shm_id';

procedure InitMessaging; cdecl; external name 'init_messaging';
procedure InitEnvironment; cdecl; external name 'init_envvars';
procedure InitWaitpid; cdecl; external name 'init_waitpid';
function GetCmdline: PChar; cdecl; external name 'get_cmdline';

{ include system independent routines }


procedure c_memcpy(dest: Pointer; src: Pointer; count: TSize);
    cdecl; external name 'memcpy';

{$DEFINE FPC_SYSTEM_HAS_MOVE}
procedure Move(const source;var dest;count:SizeInt);[public, alias: 'FPC_MOVE'];
begin
    c_memcpy(@dest, @source, count);
end;

{$I system.inc}


{*****************************************************************************
                         System Dependent Exit code
*****************************************************************************}
procedure c_exit(exit_code: integer); cdecl; external name 'exit';

procedure system_exit;
begin
    c_exit(exitcode);
end;


{*****************************************************************************
                              ParamStr/Randomize
*****************************************************************************}

{ number of args }
function paramcount : longint;
begin
    paramcount := argc - 1;
end;

{ argument number l }
function paramstr(l: longint) : String;
var
    p: PChar;
begin
    if (l >= 0) and (l <= argc) then begin
        paramstr := StrPas(argv[l]);
    end else begin
        paramstr := '';
    end;
end;

{ set randseed to a new pseudo random value }
procedure randomize;
begin
  {regs.realeax:=$2c00;
  sysrealintr($21,regs);
  hl:=regs.realedx and $ffff;
  randseed:=hl*$10000+ (regs.realecx and $ffff);}
  randseed:=0;
end;

function do_isdevice(handle:longint):boolean;
begin
  do_isdevice := true;
end;

function GetProcessID: SizeUInt;
begin
  { TODO }
  exit(0);
end;

function CheckInitialStkLen(stklen : SizeUInt) : SizeUInt;
begin
  result := stklen;
end;

procedure DisableFlushing(var f: Text); alias: 'disable_flushing';
begin
    TextRec(f).FlushFunc := nil;
end;

{*****************************************************************************
                         SystemUnit Initialization
*****************************************************************************}

(*
function GetRealStdIO(filename: String): String;
var
    ConsoleFile: text;
begin
    Assign(ConsoleFile, filename);
    Reset(ConsoleFile);
    Read(ConsoleFile, Result);
    Close(ConsoleFile);
end;
*)

procedure c_stdio_init(); cdecl; external name 'stdio_init';
function c_setvbuf(f: Pointer; buf: Pointer; mode: integer; size: TSize):
    integer; cdecl; external name 'setvbuf';

var
    c_stdin:  dword; external name 'stdin';
    c_stdout: dword; external name 'stdout';
    c_stderr: dword; external name 'stderr';

procedure SysInitStdIO;
(*
var
    filename: String;
    c_stdin, c_stdout, c_stderr: text;
*)    
begin
(*
    TextRec(c_stdin).mode := fmClosed;
    TextRec(c_stdout).mode := fmClosed;
    TextRec(c_stderr).mode := fmClosed;
    
    filename := GetRealStdIO('console:/stdin') + #0;
    Do_Open(c_stdin,  @filename[1], 0);

    filename := GetRealStdIO('console:/stdout') + #0;
    Do_Open(c_stdout, @filename[1], 0);
    
    filename := GetRealStdIO('console:/stderr') + #0;
    Do_Open(c_stderr, @filename[1], 0);

    StdInputHandle  := TextRec(c_stdin).handle;
    StdOutputHandle := TextRec(c_stdout).handle;
    StdErrorHandle  := TextRec(c_stderr).handle;
*)    

    c_stdio_init();

    StdInputHandle  := c_stdin;
    StdOutputHandle := c_stdout;
    StdErrorHandle  := c_stderr;

    c_setvbuf(c_stdout, nil, 1, 0);

    OpenStdIO(Input,  fmInput,  StdInputHandle); 
    OpenStdIO(Output, fmOutput, StdOutputHandle);
    OpenStdIO(StdOut, fmOutput, StdOutputHandle);
    OpenStdIO(StdErr, fmOutput, StdErrorHandle);
end;

function ppb_get_argc(ppb: Pointer; size: integer): integer;
    cdecl; external name 'ppb_get_argc';
procedure ppb_copy_argv(ppb: Pointer; size: integer; argv: PPChar; argc: integer);
    cdecl; external name 'ppb_copy_argv';

procedure InitParams;
var
    i: integer;
    p: PChar;
    space: boolean;
begin
    argc := ppb_get_argc(ppb_ptr, ppb_size);
    if argc < 0 then begin
        argv := nil;
        argc := 0;
        exit;
    end;

    argv := GetMem((argc + 1) * sizeof(PChar));
    ppb_copy_argv(ppb_ptr, ppb_size, argv, argc);
end;

Begin
    InitHeap;
    InitMessaging;
    InitWaitpid;

    // stdin/out/err initialisieren
    repeat
        SysInitStdIO;
    until TextRec(output).handle <> 0;

    (*argc := 0;
    argv := nil;*)
    // FIXME argv muss ein Pointerarray sein, ansonsten funktioniert
    // objpas.paramstr nicht
    InitParams;

    // TODO C-Kompatibler Zugriff.
    InitEnvironment;
    envp := nil;

    // IOError initialisieren
    InOutRes := 0;

    InitUnicodeStringManager;
End.
