; der Bootloader von CommOS

org 7C00h



jmp Bootloader						; überspringe den BIOS Parameter Block (FAT Infos)

OSName      db "MSWIN4.1"				; der Name des Betriebsystems, aus Kompatibilitätsgründen (Erkennung von FAT) "MSWIN4.1"
BytesPerSec dw 512
SecPerClus  db 1
RsvdSecCnt  dw 1
NumFATs     db 2
RootEntCnt  dw 224
TotSec      dw 2880
MediaType   db 0F0h
FATsize     dw 9
SecPerTrack dw 18					; Sektoren pro Spur, üblicherweise 18 bei Disketten
NumHeads    dw 2
HiddenSec   dd 0
TotSec32    dd 0
DrvNum      db 0
Reserved    db 0
ExBPSign    db 29h					; Bootsignatur, gibt an dass folgende 3 Felder vorhanden sind
VolumeID    dd 0FFFFFFFFh				; Disketten serial number (ID)
DiskLabel   db "LOST DISK 1"
FileSysType db "FAT12   "				; hier kommt FAT12 hin, ein paar ältere treiber determinieren so die FAT Version

Bootloader:



; deaktiviere Interrupts und setzte Richtung für string Befehle (von links nach rechts)
cli
cld

; setze alle daten segment register (ds, es und ss) zu adresse 0000, und den stack pointer zu offset FFFF
xor ax,ax
mov ds,ax
mov es,ax
mov ss,ax
mov sp,0FFFFh



; speichere das boot laufwerk
mov [drive],dl

; check if the drive is not supported by Interrupt 13h
test dh,00100000b
jnz Drive_Error



; lade das root directory
imul ax, word [RootEntCnt], 32					; ax = 32 bytes per entry * RootEntCnt
xor dx,dx
div word [BytesPerSec]						; ax / bytes per sector
mov cx,ax							; -> Anzahl der Sektoren für das Wurzelverzeichnis

; round up if edx isn't zero (shoudn't be, but if other BIOS Parameter Block)
or dx,dx
jz sectors_ok
inc ax
sectors_ok:



; now read the sectors of the root directory to address 0000:7E00
mov al,[NumFATs]
xor ah,ah
mul word [FATsize]
add ax,[RsvdSecCnt]						; Startposition des Wurzelverzeichnisses

mov bx,7E00h
call Read_Sectors						; use Interrupt ?

mov cx,[RootEntCnt]						; ecx = counter (count of entries in the root directory)
mov di,7E00h



Search_File:							; search the file

push ecx
mov ecx,11							; länge des Dateinamens (8.3)
mov esi,File_OSLoader
push edi
repne cmpsb							; repeat until the first unequal letter is found, oder alle 11 buchstaben durchgecheckt sind
pop edi
pop ecx
je Load_FAT							; wenn sie (alle) gleich sind, dann ist die Datei gefunden
add di,32

loop Search_File
jmp File_not_found						; wenn hier, dann keine Einträge mehr, -> datei nicht gefunden



Load_FAT:
 mov dx,[di+001Ah] ;Startcluster der Datei
 mov [FileCluster],dx
 mov al,[NumFATs]
 xor ah,ah
 mul word [FATsize]
 mov cx,ax ;Anzahl der Sektore für die beiden FAT-Tabellen
 mov ax,[RsvdSecCnt] ;StartSektor der beiden FAT-Tabellen
 mov bx,7E00h ;FAT-Tabellen nach 7E00h laden
 call ReadSectors

; Lade Datei nach 1000:0000
 mov ax,1000h
 mov es,ax
 xor bx,bx
 push bx



LoadFile:
 mov ax,[FileCluster]
 pop bx
 call CluSec ;Cluster zum Sektor umwandeln
 mov cl,[SecPerClus]
 xor ch,ch
 call ReadSectors
 push bx

;Aus FAT-Eintrag lesen (12 Bit)
 mov ax,[FileCluster]
 mov cx,ax
 mov dx,ax
 shr dx,1 ;durch zwei
 add cx,dx
 mov bx,7E00h
 add bx,cx
 mov dx,[bx]
 test ax,0001h ;gerader oder ungerader Cluster?
jnz OddCluster ;ungerader Cluster
 and dx,0FFFh
jmp Finish

OddCluster:
 shr dx,4

Finish:
 mov [FileCluster],dx
 cmp dx,0FFFh ;letzter zu ladender Cluster der Datei?
; wrong EOC !!!!!! 
 
 
 
jne LoadFile
 pop bx

 mov ax,1000h
 mov ds,ax
 mov es,ax

; reboot ?

db  0EAh ;springe
dw 0000h ;Offset
dw 1000h ;Segment





;input: ax = logischer Startsektor
;       bx = Offsetadresse an dem die Sektoren geladen werden sollen
;       cx = Anzahl der zu ladenden Sektore
;       es = Segmentadresse an dem die Sektoren geladen werden sollen
;output: ax = letzter Sektor + 1
;        bx = letze 512 Byte + 512
ReadSectors:
 push ax
 push bx
 push cx
 call LogPhySec
ReadSectors2:
 mov ax,0201h ;ein Sektor laden
 mov cl,[PhySector]
 mov ch,[PhyTrack]
 mov dl,[drive]
 mov dh,[PhyPage]
 int 13h
jc ReadSectors2
 pop cx
 pop bx
 pop ax
 add bx,[BytesPerSec]
 inc ax
 loop ReadSectors
ret



;input:  ax = logischer Sektor
;output: ds:PhySec = physikalischer Sektor
;        ds:PhyTrack = Spur
;        ds:PhyPage = Seite
LogPhySec:
 push ax
 push dx
 xor dx,dx
 div word [SecPerTrack]
 inc dl
 mov [PhySector],dl
 xor dx,dx
 div word [NumHeads]
 mov [PhyPage],dl
 mov [PhyTrack],al
 pop dx
 pop ax
ret



;input:  ax = Cluster
;output: ax = Sektor
CluSec:
 push cx
 push dx
 sub ax,2 ;die ersten beiden Cluster-Einträge sind in der FAT reserviert
 mov cl,[SecPerClus]
 xor ch,ch
 mul cx
 mov cx,[RsvdSecCnt]
 add ax,cx
 push ax
 mov ax,32
 mul word [RootEntCnt]
 div word [BytesPerSec]
 mov cx,ax
 pop ax
 add ax,cx
 push ax
 mov al,[NumFATs]
 xor ah,ah
 mul word [FATsize]
 mov dx,ax
 pop ax
 add ax,dx
 pop dx
 pop cx
ret



Drive_Error:							; Error Handler wenn das Boot-Laufwerk ungültig ist (z.B. USB Stick, oder CD ROM)
mov esi,MSG_Drive_Error
call Print_Text

jmp Reboot_keypress



File_not_found:							; Error Handler wenn Datei nicht gefunden wurde
mov esi,MSG_not_found
call Print_Text

;jmp Reboot_keypress



Reboot_keypress:
; reboot after a key press
xor ah,ah
Int 16h

; folgender Code startet den Computer neu (springt zum BIOS reboot)
; this code jumps to the BIOS reboot
db 0EAh
dw 0000h
dw 0FFFFh



; a function to write a text onto the screen (ds:esi = text)
Print_Text:

mov bx,0007h					; Page Number = 0, Attribute = 07h
mov ah,0Eh					; function 0Eh: Teletype Output

lodsb						; load the first character

Next_Char:
int 10h
lodsb						; AL = next character
cmp AL,00h					; last letter?
jne Next_Char					; if not print next letter

ret



MSG_load	db	"LOST wird geladen...", 10, 13
MSG_not_found	db	"LOST: Datei nicht gefunden.", 10, 13
MSG_Drive_Error	db	"Invalid drive", 10, 13
File_OSLoader	db	"LOST    SYS"


drive       db 0
FileCluster dw 0
PhySector   db 0
PhyTrack    db 0
PhyPage     db 0

times 510-($-$$) db 0
dw 0AA55h


















;Bootstrap für LOST assemblieren mit nasm

org 7C00h

jmp EndBPB ;unter dem BIOS-Parameter-Block

OSName db "L O S T "
BytesPerSec dw 512
SecPerClus db 1
RsvdSecCnt dw 1
NumFATs db 2
RootEntCnt dw 224
TotSec dw 2880
MediaType db 0F0h
FATsize dw 9
SecPerTrack dw 18
NumHeads dw 2
HiddenSec dd 0
TotSec32 dd 0
DrvNum db 0
Reserved db 0
ExBPSign db 29h ;keine Ahnung wofür das ist
VolumeID dd 0FFFFFFFFh
DiskLabel db "LOST       "
FileSysType db "FAT12   "

EndBPB:
cli
xor ax,ax
mov ds,ax
mov es,ax
mov ss,ax
mov sp,0FFFFh
sti

mov [drive],dl ;Laufwerk

;load rootdirectory
mov ax,32 ;32 Byte pro Eintrag
mul word [RootEntCnt]
div word [BytesPerSec]
mov cx,ax ;Anzahl der Sektoren für das Wurzelverzeichnis

mov al,[NumFATs]
xor ah,ah
mul word [FATsize]
add ax,[RsvdSecCnt] ;Startposition des Wurzelverzeichnisses nach ax

mov bx,7E00h ;nach 0000:7E00 das Wurzelverzeichnis laden
call ReadSectors

mov cx,[RootEntCnt]
mov di,7E00h

;search the filename
SearchFile:
push cx
mov cx,11 ;länge des Dateinamens
mov si,FileName
push di
rep cmpsb
pop di
pop cx
je LoadFAT
add di,32
loop SearchFile
jmp Error ;Datei nicht gefunden

LoadFAT:
mov dx,[di+001Ah] ;Startcluster der Datei
mov [FileCluster],dx
mov al,[NumFATs]
xor ah,ah
mul word [FATsize]
mov cx,ax ;Anzahl der Sektore für die beiden FAT-Tabellen
mov ax,[RsvdSecCnt] ;StartSektor der beiden FAT-Tabellen
mov bx,7E00h ;FAT-Tabellen nach 7E00h laden
call ReadSectors

;Lade Datei nach 1000:0000
mov ax,1000h
mov es,ax
xor bx,bx
push bx

LoadFile:
mov ax,[FileCluster]
pop bx
call CluSec ;Cluster zum Sektor umwandeln
mov cl,[SecPerClus]
xor ch,ch
call ReadSectors
push bx

;Aus FAT-Eintrag lesen (12 Bit)
mov ax,[FileCluster]
mov cx,ax
mov dx,ax
shr dx,1 ;durch zwei
add cx,dx
mov bx,7E00h
add bx,cx
mov dx,[bx]
test ax,0001h ;gerader oder ungerader Cluster?
jnz OddCluster ;ungerader Cluster
and dx,0FFFh
jmp Finish

OddCluster:
shr dx,4

Finish:
mov [FileCluster],dx
cmp dx,0FFFh ;letzter zu ladender Cluster der Datei?
jne LoadFile
pop bx

mov ax,1000h
mov ds,ax
mov es,ax

db 0EAh ;springe
dw 0000h ;Offset
dw 1000h ;Segment

;Datei wurde nicht gefunden
Error:
mov si,ErrorMsg
callWriteMsg
xor ah,ah
int 16h
;neu starten
db 0EAh
dw 0000h
dw 0FFFFh

WriteMsg:
lodsb
or al,al
jz WriteMsgBack
mov ah,0Eh
int 10h
jmp WriteMsg

WriteMsgBack:
ret

;input: ax = logischer Startsektor
;       bx = Offsetadresse an dem die Sektoren geladen werden sollen
;       cx = Anzahl der zu ladenden Sektore
;       es = Segmentadresse an dem die Sektoren geladen werden sollen
;output: ax = letzter Sektor + 1
;        bx = letze 512 Byte + 512
ReadSectors:
push ax
push bx
push cx
call LogPhySec
ReadSectors2:
mov ax,0201h ;ein Sektor laden
mov cl,[PhySector]
mov ch,[PhyTrack]
mov dl,[drive]
mov dh,[PhyPage]
int 13h
jc ReadSectors2
pop cx
pop bx
pop ax
add bx,[BytesPerSec]
inc ax
loop ReadSectors
ret

;input:  ax = logischer Sektor
;output: ds:PhySec = physikalischer Sektor
;        ds:PhyTrack = Spur
;        ds:PhyPage = Seite
LogPhySec:
push ax
push dx
xor dx,dx
div word [SecPerTrack]
inc dl
mov [PhySector],dl
xor dx,dx
div word [NumHeads]
mov [PhyPage],dl
mov [PhyTrack],al
pop dx
pop ax
ret

;input:  ax = Cluster
;output: ax = Sektor
CluSec:
push cx
push dx
sub ax,2 ;die ersten beiden Cluster-Einträge sind in der FAT reserviert
mov cl,[SecPerClus]
xor ch,ch
mul cx
mov cx,[RsvdSecCnt]
add ax,cx
push ax
mov ax,32
mul word [RootEntCnt]
div word [BytesPerSec]
mov cx,ax
pop ax
add ax,cx
push ax
mov al,[NumFATs]
xor ah,ah
mul word [FATsize]
mov dx,ax
pop ax
add ax,dx
pop dx
pop cx
ret

Msg db "LOST wird geladen...",00h
ErrorMsg db "LOST: Datei nicht gefunden.",0
FileName db "LOST    SYS"
drive db 0
FileCluster dw 0
PhySector db 0
PhyTrack db 0
PhyPage db 0

times 510-($-$$) db 0
dw 0AA55h

