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
 PathSeparator = ';';
{ FileNameCaseSensitive is defined separately below!!! }
// cHEAPSIZE = 512*1024;
 MaxExitCode = 255;
 CtrlZMarksEOF: boolean = false; (* #26 is considered as end of file *)

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

    envp: Pointer;

procedure DisableFlushing(var f: Text);

implementation

procedure InitMessaging; cdecl; external name 'init_messaging';

{ include system independent routines }

{$I system.inc}


{*****************************************************************************
                         System Dependent Exit code
*****************************************************************************}
procedure system_exit;
begin
    asm
        mov SYSCALL_PM_EXIT_PROCESS, %eax
        int $0x30
    end;
end;


{*****************************************************************************
                              ParamStr/Randomize
*****************************************************************************}

{ number of args }
function paramcount : longint;
begin
  {paramcount := argc - 1;}
  paramcount:=0; 
end;

{ argument number l }
function paramstr(l : longint) : string;
begin
  {if (l>=0) and (l+1<=argc) then
   paramstr:=strpas(argv[l])
  else}
   paramstr:='';
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

procedure SysInitStdIO;
var
    c_stdin, c_stdout, c_stderr: text;
begin
    Do_Open(c_stdin,  'console:/stdin',  0);
    Do_Open(c_stdout, 'console:/stdout', 0);
    Do_Open(c_stderr, 'console:/stderr', 0);

    StdInputHandle  := TextRec(c_stdin).handle;
    StdOutputHandle := TextRec(c_stdout).handle;
    StdErrorHandle  := TextRec(c_stderr).handle;

    OpenStdIO(Input,  fmInput,  StdInputHandle); 
    OpenStdIO(Output, fmOutput, StdOutputHandle);
    OpenStdIO(StdOut, fmOutput, StdOutputHandle);
    OpenStdIO(StdErr, fmOutput, StdErrorHandle);
end;

Begin
    InitHeap;
    InitMessaging;

    // stdin/out/err initialisieren
    repeat
        SysInitStdIO;
    until TextRec(output).handle <> 0;

    // TODO setup_arguments (argv, argc)
    argc := 0;
    argv := nil;
    envp := nil;

    // IOError initialisieren
    InOutRes := 0;
End.
