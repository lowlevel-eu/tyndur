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

#include "config.h"
#if CONFIG_MALLOC == MALLOC_LOST

#include "types.h"
#include "string.h"
#include "stdlib.h"
#define PAGESIZE 0x1000

struct memory_node
{
    size_t              size    : 32;   // Groesse des Blocks
    dword               address : 32;   // Startadresse des Blocks
    
    struct memory_node*  next;          // Zeiger zum naechsten Block
};

struct memory_node* first_unused_memory_node    = NULL;
struct memory_node* first_used_memory_node      = NULL;
struct memory_node* first_free_memory_node      = NULL;

// FIXME: Dafuer sollte man wohl eher ne header-Datei hernehmen
extern void* mem_allocate(dword size, dword flags);


void append_memory_node(struct memory_node** list, struct memory_node* element);
struct memory_node* create_memory_node_table();
struct memory_node* create_memory_node(size_t size, dword address, byte free);



/**
 * Initialisiert die Speicherverwaltung
 */
// FIXME: Muss automatisch aufgerufen werden!
void init_memory_manager()
{
    first_unused_memory_node    = NULL;
    first_used_memory_node      = NULL;
    first_free_memory_node      = NULL;
}


/**
 * Haengt das uebergebene Element an die Liste an.
 * @param list Pointer zu dem ersten Element Liste
 * @param element Anzufuegendes Element
 */
void append_memory_node(struct memory_node** list, struct memory_node* element)
{
        element->next = *list;
        *list = element;
}


/**
 * Holt eine Seite vom Kernel und erstellt darauf jede Menge memory_nodes.
 * @return Pointer auf das erste erstellte Node
 */
struct memory_node* create_memory_node_table()
{
    struct memory_node* new_nodes = mem_allocate(PAGESIZE - 1, 0);
    memset(new_nodes, 0, PAGESIZE);
    
    // die neuen Nodes werden der unused liste angehaengt, ein
    int i = 1;
    for(;i < (PAGESIZE / sizeof(struct memory_node)); i++)
        append_memory_node(&first_unused_memory_node, &(new_nodes[i]));
    
    return new_nodes;
}



/**
 * Holt ein neues memory_node aus der Liste der unbenutzen, weist ihm die 
 * angegeben Eigenschaften zu und haengt es bei der richtigen Liste an.
 * @param size Groesse des Blocks
 * @param address Startadresse des Blocks
 * @param free Gibt an, ob ein freier, oder ein belegter Block erfasst werden 
 *              soll (1 => frei).
 * @return Pointer auf das memory_node
 */
struct memory_node* create_memory_node(size_t size, dword address, byte free)
{
    struct memory_node* new_memory_node = first_unused_memory_node;
    
    // wenn die Liste der unbenutzen Elemente leer ist, dann erstelle welche
    if(new_memory_node == NULL)
        new_memory_node = create_memory_node_table();
    else
        first_unused_memory_node = first_unused_memory_node->next;
    
    new_memory_node->size = size;
    new_memory_node->address = address;
    
    // Den Block der ensprechenden Liste anhaengen
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
    
    // Wird benutzt um einen eintrag aus der liste loeschen zu koennen
    struct memory_node* previous_memory_node = NULL;
    struct memory_node* current_memory_node = first_free_memory_node;
    
    // schaue zuerst in der Liste, ob was passendes da ist
    while(current_memory_node != NULL)
    {
        if(current_memory_node->size >= size)
            break;
        
        previous_memory_node = current_memory_node;
        current_memory_node = current_memory_node->next;
    }
    
    
    // wenn kein passendes gefunden wurde, dann wird nun eines erstellt
    if(current_memory_node == NULL)
    {
        // berechne die Anzahl der Seiten
        size_t page_count = size / PAGESIZE;
        if((size % PAGESIZE) != 0)
            page_count++;
        
        // speicher beim kernel holen
        char *ptr = mem_allocate(page_count * PAGESIZE, 0);
        
        current_memory_node = create_memory_node(size, ((dword)ptr), 0);
        
        // wenn noch freier Speicher auf der Seite bleibt, dann legen wir noch eine Block-Struktur dafuer an
        if((page_count * PAGESIZE) > size)
        {
            create_memory_node((page_count * PAGESIZE) - size, ((dword)ptr) + size, 1);
        }
    }
    else
    {
         // den current_memory_node aus der alten liste loeschen
        if(previous_memory_node == NULL)
            first_free_memory_node = current_memory_node->next;
        else
            previous_memory_node->next = current_memory_node->next;


        //In die neue Liste rein
        append_memory_node(&first_used_memory_node, current_memory_node);

        // Ueberpruefen, ob noch ein block fuer den freien Rest erstellt werden muss
        if(current_memory_node->size > size)
        {
            create_memory_node(current_memory_node->size - size, current_memory_node->address + size, 1);
            current_memory_node->size = size;
        }

    }

    //printf("malloc(%d): 0x%08x\n", size, current_memory_node->address);
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
    
    // schaue zuerst in der Liste, ob was passendes da ist
    while(current_memory_node != NULL)
    {
        if(current_memory_node->address == (dword)address)
        {
            // den current_memory_node aus der alten liste loeschen
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


/**
 *
 */
void* realloc(void* address, size_t size)
{
    void* result;
    if((address == NULL) && (size == 0))
    {
        result = FALSE;
    }
    if(address == NULL)
    {
        result = malloc(size);
    }
    else if(size == 0)
    {
        free(address);
        result = NULL;
    }
    else
    {
        size_t old_size = 0;
        
        struct memory_node* current_memory_node = first_used_memory_node;
        
        //Den Memory-Node mit der zugehoerigen Adresse suchen
        while(current_memory_node != NULL)
        {
            if(current_memory_node->address == (dword)address)
            {
                old_size = current_memory_node->size;
                break;
            }
            current_memory_node = current_memory_node->next;
        }
        
        //Wenn der Memory-Node nicht gefunden wurde => abbrechn
        if(old_size == 0)
        {
            return NULL;
        }
        
        result = malloc(size);

        if(size > old_size)
        {
            memcpy(result, address, old_size);
        }
        else
        {
            memcpy(result, address, size);
        }

        free(address);
    }
    return (result);
}

#endif /* CONFIG_MALLOC == LOST */
