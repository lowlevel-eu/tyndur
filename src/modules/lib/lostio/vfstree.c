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
#include "types.h"
#include "rpc.h"
#include "collections.h"
#include "lostio_internal.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"

vfstree_node_t vfstree_root;


/**
 * 
 *
 * @param path Pfad
 *
 * @return Pointer auf den neuen Pfad
 */
char* vfstree_dirname(char* path)
{
    int i;
    char* dirname_end = NULL;
    
    //Den Pfad rückwärts nach / durchsuchen
    for(i = strlen(path) - 1; i <= 0; i--)
    {
        if(path[i] == '/')
        {
            dirname_end = &(path[i]);
            break;
        }

    }
    
    char* dirname;
    if((dirname_end == NULL) || (dirname == path))
    {
        dirname = malloc(2);
        dirname[0] = '/';
        dirname[1] = '\0';
    }
    else
    {
        char* dirname = malloc((dword) dirname_end - (dword) path);
        memcpy(dirname, path, (dword) dirname_end - (dword) path - 1);
    }
    return dirname;
}


/**
 * 
 * 
 * @param path Pointer auf den Pfad
 * 
 * @return Pointer auf den obersten Pfadteil
 */
char* vfstree_basename(char* path)
{
    int i;
    char* dirname_end = path;
    
    //Den Pfad rückwärts nach / durchsuchen
    for(i = strlen(path) - 1; i >= 0; i--)
    {
        if(path[i] == '/')
        {
            dirname_end = &(path[i+1]);
            break;
        }
    }
    
    return dirname_end;
}


/**
 * Einen Knoten anhand seines Namens finden.
 *
 * @param name Name des gesuchten Knotens
 *
 * @return Einen Pointer auf den Knoten oder NULL falls er nicht gefunden wurde.
 */
vfstree_node_t* vfstree_get_node_by_name(vfstree_node_t* parent, char* name)
{
    int i = 0;
    vfstree_node_t* cur_node;
    while((cur_node = list_get_element_at((list_t*)parent->children, i++)))
    {
        if(strcmp(name, cur_node->name) == 0)
        {
            return cur_node;
        }
    }
    return NULL;    
}


/**
 * Einen Knoten anhand seines Pfades finden.
 *
 * @param path Pfad des gesuchten Knotens
 *
 * @return Einen Pointer auf den Knoten oder NULL falls er nicht gefunden wurde.
 */
vfstree_node_t* vfstree_get_node_by_path(char* path)
{
    //Fuehrenden Slash entfernen
    if(path[0] == '/')
    {
        path++;
    }
    
    //Mit Hilfe dieser Pointer wird der Pfad aufgeteilt
    char* part_begin = path;
    char* part_end = path;
    
    vfstree_node_t* cur_parent = &vfstree_root;
    
    if(strlen(part_begin) == 0)
    {
        return cur_parent;
    }

    int i;
    for(i = 0; i < strlen(path) + 1 ; i++)
    {
        //Ist ein weiterer Pfad Teil fertig?
        if((*part_end == '/') || (*part_end == '\0'))
        {
            //Den Slash durch ein Null-Byte ersetzen, um das Vergleichen zu
            //erleichtern
            *part_end = '\0';

            cur_parent = vfstree_get_node_by_name(cur_parent, part_begin);
            
            //Wenn der Eintrag nicht gefunden wurde wir NULL zurueck gegeben
            if(cur_parent == NULL)
            {
                return NULL;
            }
            
            part_begin = part_end + 1;
        }
        else
        {
            part_end++;
        }
    }

    return cur_parent;
}


/**
 * Erstellt einen neuen Knoten. Die Daten werden dabei _nicht kopiert sondern
 * nur ein Pointer darauf angelegt.
 *
 * @param parent Pointer auf das Verzeichnis, worin das Objekt erstellt werden
 *               soll.
 * @param type Typ des neuen Knoten
 * @param name Name des neuen Knoten(wird kopiert).
 * @param size Groesse
 * @param data Pointer auf den Inhalt(_nickt_ kopiert)
 *
 * @return TRUE, wenn der Knoten fehlerfrei erstellt wurde, sonst FALSE.
 */
bool vfstree_create_node(char* path, typeid_t type, size_t size, void* data)
{
    char* dirname = vfstree_dirname(path);
    char* name = vfstree_basename(path);
    vfstree_node_t* parent = vfstree_get_node_by_path(dirname);
    free(dirname);

    if(parent == NULL)
    {
        puts("Fehler beim erstellen der Datei");
        return FALSE;
    }
    
    vfstree_node_t* new_file = malloc(sizeof(vfstree_node_t));

    parent->children = list_push((list_t*)parent->children, (void*) new_file);
    new_file->type = type;

    new_file->name = malloc(strlen(name) + 1);
    memcpy(new_file->name, name, strlen(name) + 1);

    new_file->size = size;
    new_file->data = data;
    new_file->children = list_create();
    return TRUE;
}


/**
 *
 */
bool vfstree_delete_node(char* path)
{
    char* dirname = vfstree_dirname(path);
    char* basename = vfstree_basename(path);
    
    vfstree_node_t* parent = vfstree_get_node_by_path(dirname);
    vfstree_node_t* node_del = vfstree_get_node_by_path(path);
    free(dirname);

    if((parent == NULL) || (node_del == NULL))
    {
        return FALSE;
    }

    //Hierbei darf nichts dazwischen kommen, da sonst der index verrutschen
    //könnte.
    p();
    
    int i = 0;
    vfstree_node_t* node;
    
    while((node = (vfstree_node_t*) list_get_element_at(parent->children, i++)))
    {
        if(node == node_del)
        {
            break;
        }
    }
    
    //Abbrechen falls der Knoten nicht gefunden wurde
    if(node == NULL)
    {
        v();
        return FALSE;
    }
    
    list_remove(parent->children, i);
    
    v();

    //TODO: Kinder-Knoten Loeschen
    free(node);

    
    return TRUE;
}

