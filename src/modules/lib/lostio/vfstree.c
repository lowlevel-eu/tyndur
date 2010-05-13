/*  
 * Copyright (c) 2007 The tyndur Project. All rights reserved.
 *
 * This code is derived from software contributed to the tyndur Project
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
 * @{
 */

#include <stdint.h>
#include <stdbool.h>
#include "rpc.h"
#include "collections.h"
#include "lostio_internal.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include <syscall.h>

///Der Root-Knoten
vfstree_node_t vfstree_root;

///Die naechste zu vergebende resid
uint64_t next_resid = 1;


/**
 * Den Verzeichnisnamen von einem Pfad abtrennen und den hinteren Teil (naeher
 * beim :/)
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
    for(i = strlen(path) - 1; i >= 0; i--)
    {
        if(path[i] == '/')
        {
            dirname_end = &(path[i]);
            break;
        }

    }
    
    char* dirname;
    if((dirname_end == NULL) || (dirname_end == path))
    {
        dirname = malloc(2);
        dirname[0] = '/';
        dirname[1] = '\0';
    }
    else
    {
        dirname = malloc((uint32_t) dirname_end - (uint32_t) path + 1);
        memcpy(dirname, path, (uint32_t) dirname_end - (uint32_t) path);
        dirname[(uint32_t) dirname_end - (uint32_t) path] = 0;
    }
    return dirname;
}


/**
 * Den letzten Teil holen (Der gegenueber von :/)
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
 * @param parent Pointer auf den Elternknoten
 *
 * @return Einen Pointer auf den Knoten oder NULL falls er nicht gefunden wurde.
 */
vfstree_node_t* vfstree_get_node_by_name(vfstree_node_t* parent, char* name)
{
    if(name[0] == '/')
    {
        name++;
    }


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
    char c;
    for(i = 0; i <= strlen(path) ;i++)
    {
        //Ist ein weiterer Pfad Teil fertig?
        c = *part_end;
        if((c == '/') || (c == '\0'))
        {
            //Den Slash durch ein Null-Byte ersetzen, um das Vergleichen zu
            //erleichtern
            *part_end = '\0';
            //printf("part='%s'\n", part_begin);
            cur_parent = vfstree_get_node_by_name(cur_parent, part_begin);
            *part_end = c;
            //Wenn der Eintrag nicht gefunden wurde wir NULL zurueck gegeben
            if(cur_parent == NULL)
            {
                return NULL;
            }
            
            part_begin = ++part_end;

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
 * @param parent Eltern-Node
 * @param name Knotenname
 * @param type Typ des neuen Knoten
 * @param size Groesse
 * @param data Pointer auf den Inhalt(_nicht_ kopiert)
 *
 * @return true, wenn der Knoten fehlerfrei erstellt wurde, sonst false.
 */
bool vfstree_create_child(vfstree_node_t* parent, char* name, typeid_t type,
    size_t size, void* data, uint32_t flags)
{  
    vfstree_node_t* new_file = malloc(sizeof(vfstree_node_t));

    new_file->type = type;

    new_file->name = malloc(strlen(name) + 1);
    memcpy(new_file->name, name, strlen(name) + 1);

    new_file->size  = size;
    new_file->data  = data;
    new_file->flags = flags;

    new_file->children = list_create();
    new_file->parent = parent;

    p();
    new_file->resid = next_resid++;
    parent->children = list_push((list_t*)parent->children, (void*) new_file);
    parent->size++;
    v();

    return true;
}


/**
 * Erstellt einen neuen Knoten. Die Daten werden dabei _nicht kopiert sondern
 * nur ein Pointer darauf angelegt.
 *
 * @param path Pfad des neuen Knoten
 * @param type Typ des neuen Knoten
 * @param size Groesse
 * @param data Pointer auf den Inhalt(_nickt_ kopiert)
 *
 * @return true, wenn der Knoten fehlerfrei erstellt wurde, sonst false.
 */
bool vfstree_create_node(char* path, typeid_t type, size_t size, void* data,
    uint32_t flags)
{
    char* dirname = vfstree_dirname(path);
    char* name = vfstree_basename(path);
    //printf("Erstelle Knoten '%s', '%s' ('%s')\n", dirname, name, path);
    vfstree_node_t* parent = vfstree_get_node_by_path(dirname);
    free(dirname);

    if(parent == NULL)
    {
        puts("Fehler beim erstellen der Datei");
        return false;
    }
    
    return vfstree_create_child(parent, name, type, size, data, flags);
}

/**
 * Kindknoten anhand seines Namens loeschen
 *
 * @param parent Elternknoten
 * @param name Name des gewuenschten Kindknotens
 *
 * @return true bei Erfolg, false im Fehlerfall
 */
bool vfstree_delete_child(vfstree_node_t* parent, const char* name)
{
    vfstree_node_t* node_del = vfstree_get_node_by_name(parent, (char*) name);
    if((parent == NULL) || (node_del == NULL))
    {
        return false;
    }

    //Hierbei darf nichts dazwischen kommen, da sonst der index verrutschen
    //könnte.
    p();
    
    int i = 0;
    vfstree_node_t* node;
    
    for (i = 0; (node = (vfstree_node_t*) list_get_element_at(parent->children, i))
        && (node != node_del); i++);
    
    //Abbrechen falls der Knoten nicht gefunden wurde
    if(node == NULL)
    {
        v();
        return false;
    }
    
    list_remove(parent->children, i);
    
    v();
    parent->size--;

    //TODO: Kinder-Knoten Loeschen
    free(node->name);
    free(node);
    
    return true;
}

/**
 * Einen Knoten anhand seines Pfades loeschen
 *
 * @param path Pointer auf den Pfad
 *
 * @return true, wenn der Knoten fehlerfrei geloescht wurde, sonst false.
 */
bool vfstree_delete_node(char* path)
{
    char* dirname = vfstree_dirname(path);
    char* basename = vfstree_basename(path);

    vfstree_node_t* parent = vfstree_get_node_by_path(dirname);
    free(dirname);

    if(parent == NULL)
    {
        return false;
    }
    
    return vfstree_delete_child(parent, basename);
}


void vfstree_clear_node(vfstree_node_t* node)
{
    if(node->size != 0)
    {
        vfstree_node_t* work_node;

        while((work_node = list_pop(node->children)))
        {
            vfstree_clear_node(work_node);
            free(work_node->data);
            free(work_node);
        }
        node->size = 0;
    }
}

/** @} */

