
Enumeration
#File_Debug
#File_In
#File_Out
EndEnumeration

Structure Replace
Orig.s
New.s
EndStructure

NewList Repl.Replace()

; Open Debug Log
dbg = CreateFile(#File_Debug,"C:\asm-recode-debug.txt")
If dbg = 0
  MessageRequester("Fehler","Konnte Debugdatei nicht anlegen!")
  End
EndIf

Procedure WriteDebug(str$)
  UseFile(#File_Debug)
  WriteStringN(str$)
EndProcedure

infile$ = ProgramParameter()
;infile$ = OpenFileRequester("Eingabedatei auswählen","E:\Devel\LOST\Source-SVN\trunk\kernel\src\header.asm","Asm|*.asm|Alle Dateien|*.*",0)

Debug infile$
WriteDebug("Input File: "+infile$)

outfile$ = infile$+".tmp"
;outfile$ = OpenFileRequester("Ausgabedatei auswählen","E:\Devel\LOST\Source-SVN\trunk\kernel\src\header_new.asm","Asm|*.asm|Alle Dateien|*.*",0)

Debug outfile$
WriteDebug("Output File: "+outfile$)

in = ReadFile(#File_In,infile$)
If in = 0
  MessageRequester("Fehler","Konnte Eingabedatei nicht laden!")
  WriteDebug("ERR: Kann Eingabedatei nicht lesen: "+infile$)
  End
EndIf

out = CreateFile(#File_Out,outfile$)
If out = 0
  MessageRequester("Fehler","Konnte Ausgabedatei nicht erstellen!")
  WriteDebug("ERR: Kann Ausgabedatei nicht erstellen: "+outfile$)
  End
EndIf

While Eof(#File_In) = 0
  UseFile(#File_In)
  line$ = ReadString()
  line$ = LTrim(line$)
  If LCase(Mid(line$,0,6)) = "extern" Or LCase(Mid(line$,0,6)) = "global"
    pos = FindString(line$,";",0)
    If pos <> 0
      line$ = Mid(line$,0,pos-1)
    EndIf
    keyword$ = Mid(line$,8,Len(line$)-7)
    keyword$ = Trim(keyword$)
    Debug keyword$
    If Mid(keyword$,0,1) <> "_"
      newkeyword$ = "_"+keyword$
      ;Debug keyword$+" = "+newkeyword$
      AddElement(Repl())
      Repl()\Orig = keyword$
      Repl()\New = newkeyword$
    EndIf
  EndIf
Wend

UseFile(#File_In)
FileSeek(0)
line$ = ""

While Eof(#File_In) = 0
  UseFile(#File_In)
  line$ = ReadString()
  ResetList(Repl())
  newline$ = ""
  newline$ = line$
  While NextElement(Repl())
    newline$ = ReplaceString(newline$,Repl()\Orig,Repl()\New)
    Debug "Replace "+Repl()\Orig+" with "+Repl()\New
  Wend
  UseFile(#File_Out)
  WriteStringN(newline$)
Wend

CloseFile(#File_In)
CloseFile(#File_Out)
;CopyFile(outfile$,infile$)
    
; ExecutableFormat=Windows
; CursorPosition=94
; FirstLine=29
; EOF