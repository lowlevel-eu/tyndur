/*  
 * Copyright (c) 2006 The LOST Project. All rights reserved.
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
#include "syscall.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"

#define PAGESIZE 0x1000

struct memory_node
{
    size_t              size    : 32;   /*Groesse des Blocks*/
    dword               address : 32;   /*Startadresse des Blocks*/
    
    struct memory_node*  next;          /*Zeiger zum nächsten Block*/
};

struct memory_node* first_unused_memory_node;
struct memory_node* first_used_memory_node;
struct memory_node* first_free_memory_node;



struct memory_node* create_memory_node_table();



void init_memory_manager()
{
    first_unused_memory_node    = NULL;
    first_used_memory_node      = NULL;
    first_free_memory_node      = NULL;
}


/**
 * Haenge das uebergebene Element an die Liste an.
 * @param list Pointer zu dem ersten Element Liste
 * @param element Anzufuegendes Element
 */
void append_memory_node(struct memory_node** list, struct memory_node* element)
{
    if(*list == NULL)
    {
        *list = element;
        element->next = NULL;
    }
    else
    {
        element->next = *list;
        *list = element;
    }
}


/**
 * Hole eine Seite vom Kernel und erstelle darauf jede Menge memory_nodes.
 * @return Pointer auf das erste erstellte Node
 */
struct memory_node* create_memory_node_table()
{
    puts("create node table");
    struct memory_node* new_nodes = mem_allocate(PAGESIZE - 1, 0);
    memset(new_nodes, 0, PAGESIZE);
    
    /*die neuen Nodes werden der unused liste angehaengt, ein */
    int i = 1;
    for(;i < (PAGESIZE / sizeof(struct memory_node)); i++)
        append_memory_node(&first_unused_memory_node, &(new_nodes[i]));
    
    return new_nodes;
}



/**
 * Hole ein neues memory_node aus der Liste der unbenutzen, weise ihm die 
 * angegeben Eigenschaften zu und haenge es bei der richtigen Liste an.
 * @param size Groesse des Blocks
 * @param address Startadresse des Blocks
 * @param free Gibt an, ob ein freier, oder ein belegter Block erfasst werden 
 *              soll (1 => frei).
 * @return Pointer auf das memory_node
 */
struct memory_node* create_memory_node(size_t size, dword address, byte free)
{
    struct memory_node* new_memory_node = first_unused_memory_node;
    
    /*wenn die Liste der unbenutzen Elemente leer ist, dann erstelle welche*/
    if(new_memory_node == NULL)
        new_memory_node = create_memory_node_table();
    else
        first_unused_memory_node = first_unused_memory_node->next;
    
    new_memory_node->size = size;
    new_memory_node->address = address;
    
    /*Den Block der ensprechenden Liste anhaengen*/
    if(free == 0)
        append_memory_node(&first_used_memory_node, new_memory_node);
    else
        append_memory_node(&first_free_memory_node, new_memory_node);
    
    return new_memory_node;
}


/**
 * Reserviert einen Speicherbereich
 * @param size Groesse des Bereichs
 * @return Pointer auf den Bereich
 */
void* malloc(size_t size)
{
    if(size < 1) return NULL;
    
    /*Wird benutzt um einen eintrag aus der liste loeschen zu koennen*/
    struct memory_node* previous_memory_node = NULL;
    struct memory_node* current_memory_node = first_free_memory_node;
    
    /*schaue zuerst in der Liste, ob was passendes da ist*/
    while(current_memory_node != NULL)
    {
        if(current_memory_node->size >= size)
            break;
        
        previous_memory_node = current_memory_node;
        current_memory_node = current_memory_node->next;
    }
    
    /*wenn kein passendes gefunden wurde, dann wird nun eines erstellt*/
    if(current_memory_node == NULL)
    {
        puts("null");
        /*berechne die Anzahl der Seiten*/
        size_t page_count = size / PAGESIZE;
        if((size % PAGESIZE) != 0)
            page_count++;
        
        /*speicher beim kernel holen*/
        char *ptr = mem_allocate(page_count * PAGESIZE, 0);
        
        current_memory_node = create_memory_node(size, ((dword)ptr), 0);
        
        /*wenn noch freier Speicher auf der Seite bleibt, dann legen wir noch eine Block-Struktur dafuer an*/
        if((page_count * PAGESIZE) > size)
        {
            struct memory_node* free_node = create_memory_node((page_count * PAGESIZE) - size, ((dword)ptr) + size, 1);
            append_memory_node(&first_used_memory_node, free_node);
        }
    }
    else
    {
        /*Ueberpruefen, ob noch ein block fuer den freien Rest erstellt werden muss*/
        if(current_memory_node->size > size)
        {
            create_memory_node(current_memory_node->size - size, current_memory_node->address + size, 1);
        }
        
        /*den current_memory_node aus der alten liste löeschen*/
        if(previous_memory_node == NULL)
            first_free_memory_node = current_memory_node->next;
        else
            previous_memory_node->next = current_memory_node->next;
        
        append_memory_node(&first_used_memory_node, current_memory_node);
    }
    
    return (void*) current_memory_node->address;
}


/**
 * Gibt einen reservierten Speicherbereich wieder frei.
 * @param address Adresse des Speicherbereichs
 */
void free(void* address)
{
    struct memory_node* previous_memory_node = NULL;
    struct memory_node* current_memory_node = first_used_memory_node;
    
    /*schaue zuerst in der Liste, ob was passendes da ist*/
    while(current_memory_node != NULL)
    {
        if(current_memory_node->address == (dword)address)
        {
            /*den current_memory_node aus der alten liste löeschen*/
            if(previous_memory_node == NULL)
                first_used_memory_node = current_memory_node->next;
            else
                previous_memory_node->next = current_memory_node->next;
        
            append_memory_node(&first_free_memory_node, current_memory_node);
            
            return;
        }
        previous_memory_node = current_memory_node;
        current_memory_node = current_memory_node->next;
    }
}
