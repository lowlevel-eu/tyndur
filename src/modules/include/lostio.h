/*
 * Copyright (c) 2007 The LOST Project. All rights reserved.
 *
 * This code is derived from software contributed to the LOST Project
 * by Antoine Kaufmann.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *     This product includes software developed by the LOST Project
 *     and its contributors.
 * 4. Neither the name of the LOST Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/** \addtogroup LostIO
 *  @{
 */

#ifndef _LOSTIO_H_
#define _LOSTIO_H_
#include "types.h"
#include "collections.h"


///ID des Verzeichnis Typs
#define LOSTIO_TYPES_DIRECTORY  1

///ID des Ramfile Typs
#define LOSTIO_TYPES_RAMFILE    128


#define LOSTIO_FLAG_EOF 0x10000
#define LOSTIO_FLAG_BROWSABLE 0x20000
///Der Knoten ist ein Symlink.
#define LOSTIO_FLAG_SYMLINK 0x40000

/*#define LOSTIO_MODE_READ 0x1
#define LOSTIO_MODE_WRITE 0x2
#define LOSTIO_MODE_APPEND 0x4
#define LOSTIO_MODE_BROWSE 0x8
*/
///Typ der Typehandle-IDs
typedef byte typeid_t;

///Knoten im VFS-Baum
typedef struct vfstree_node_t
{
    typeid_t        type;
    char*           name;
    qword           size;
    void*           data;
    qword           resid;
    dword           flags;

    list_t*         children;
    struct vfstree_node_t* parent;
} vfstree_node_t;



///Handle fuer eine geoeffnete Datei
typedef struct
{
    dword           id;
    pid_t           pid;
    dword           flags;
    io_resource_t*  source;
    qword           pos;
    
    ///Modulspezifische Daten
    void*           data;
    
    vfstree_node_t* node;
} lostio_filehandle_t;


///Rueckgabe der 
typedef struct
{
    void*           data;
    size_t          size;
} read_hdl_reply;


///Handle um einen bestimmten Knotentyp zu beschreiben
typedef struct
{
    typeid_t        id;
    bool            (*not_found)(char**, byte, pid_t,io_resource_t*);
    bool            (*pre_open)(char**, byte, pid_t,io_resource_t*);
    void            (*post_open)(lostio_filehandle_t*);

    read_hdl_reply  (*read)(lostio_filehandle_t*,size_t,size_t);
    size_t          (*write)(lostio_filehandle_t*,size_t,size_t,void*);
    int             (*seek)(lostio_filehandle_t*,int,int);
    int             (*close)(lostio_filehandle_t*);
} typehandle_t;



///LostIO-Schnittstelle initialisieren
void lostio_init();

///LostIO-Interne vorgaenge abarbeiten
void lostio_dispatch();

///Typehandle in die Liste einfuegen
void lostio_register_typehandle(typehandle_t* typehandle);

///Typehandle anhand der ID finden
typehandle_t* get_typehandle(typeid_t id);



///Neuen Knoten im VFS-Baum erstellen
bool vfstree_create_node(char* path, typeid_t type, size_t size, void* data, dword flags);

///Knoten aus dem VFS-Baum loeschen
bool vfstree_delete_node(char* path);

///
void vfstree_clear_node(vfstree_node_t* node);

///Dateinamen aus einem ganzen Pfad extrahieren
char* vfstree_basename(char* path);

///Verzeichnisnamen aus einem Pfad extrahieren
char* vfstree_dirname(char* path);

///Pointer auf einen Knoten anhand seines Pfades ermitteln
vfstree_node_t* vfstree_get_node_by_path(char* path);



///Den Ramfile-Typ benutzbar machen
void lostio_type_ramfile_use();

///Den Ramfile-Typ unter einer bestimmten ID benutzbar machen
void lostio_type_ramfile_use_as(typeid_t id);

///Den Verzechnis-Typ benutzbar machen
void lostio_type_directory_use();

///Den Verzechnis-Typ unter einer bestimmten ID benutzbar machen
void lostio_type_directory_use_as(typeid_t id);



/** @}  */

#endif

