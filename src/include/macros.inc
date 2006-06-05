; Die Makros cextern und cglobal müssen an Stelle von extern und global
; verwendet werden, da je nach verwendeter Variante des GCCs die Symbole in
; den Assembler-Quellen mit Unterstrichen deklariert werden müssen oder nicht.

%ifdef HAVE_ASM_USCORE

%macro cextern 1-*
%rep %0
extern _%1
%define %1 _%1
%rotate 1
%endrep
%endmacro

%macro cglobal 1-*
%rep %0
global _%1
%define %1 _%1
%rotate 1
%endrep
%endmacro

%else

%define cextern extern
%define cglobal global

%endif