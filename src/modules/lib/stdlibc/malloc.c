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

struct memoryNode
{
    size_t              size    : 31;   /*Groesse des Blocks*/
    byte                free    : 1;    /*Gibt an. ob der Block freier oder benutzen Speicher verwaltet*/
    dword               address : 32;   /*Startadresse des Blocks*/
    
    struct memoryNode*  next ;          /*Zeiger zum nächsten Block*/
};


struct memoryNode* firstMemoryNode;
struct memoryNode* lastMemoryNode;



struct memoryNode* createMemoryNodeTable();



void initMemoryManager()
{
    lastMemoryNode = NULL;
    firstMemoryNode = createMemoryNodeTable();
}


/*erstelle neue Tabelle mit Speicherbloecken und haenge sie den bisherigen an*/
struct memoryNode* createMemoryNodeTable()
{
    struct memoryNode* newNodes = mem_allocate(PAGESIZE - 1, 0);
    memset(newNodes, 0, PAGESIZE);
    
    int i = 1;
    for(;i < (PAGESIZE / sizeof(struct memoryNode)); i++)
        newNodes[i - 1].next = &(newNodes[i]);
    
    /*die neue Tabelle den anderen anhaengen, aber nur wenn es schon andere gibt*/
    if(lastMemoryNode != NULL)
        lastMemoryNode->next = newNodes;
    
    /*zeiger auf letzes Element setzen*/
    lastMemoryNode = &(newNodes[PAGESIZE / sizeof(struct memoryNode) - 1]);
    lastMemoryNode->next = NULL;
    return newNodes;
}


/*Erstelt eine neue Blockstruktur*/
struct memoryNode* createMemoryNode(size_t size, dword address, byte free)
{
    struct memoryNode* newMemoryNode;
    
    /*Durchsuche vorhandene Blocks auf solche mit Groesse 0(=unbenutzt)*/
    while((newMemoryNode != NULL) && (newMemoryNode->size != 0))
        newMemoryNode = newMemoryNode->next;
    
    /*wenn kein unbenutzt Element gefunden wurde, muss eine neue Tabelle erstellt werden*/
    if(newMemoryNode == NULL)
        newMemoryNode = createMemoryNodeTable();
    
    newMemoryNode->size = size;
    newMemoryNode->free = free;
    newMemoryNode->address = address;
    return newMemoryNode;
}


/*finde das erste freie MemoryNode welches gross genug ist*/
struct memoryNode* findFirstMemoryNode(size_t size)
{
    struct memoryNode* currentMemoryNode = firstMemoryNode;
    
    /*schaue zuerst in der Liste, ob was passendes da ist*/
    while(currentMemoryNode != NULL)
    {
        if((currentMemoryNode->size >= size) && (currentMemoryNode->free == 1))
            break;
        
        currentMemoryNode = currentMemoryNode->next;
    }
    /*wenn kein passendes gefunden wurde, dann wird nun eines erstellt*/
    if(currentMemoryNode == NULL)
    {
        /*berechne die Anzahl der Seiten*/
        size_t pageCount = size / PAGESIZE;
        if((size % PAGESIZE) != 0)
            pageCount++;
        
        /*speicher beim kernel holen*/
        char *ptr = mem_allocate(pageCount * PAGESIZE - 1, 0);
        
        currentMemoryNode = createMemoryNode(size, ((dword)ptr), 0);
        
        /*wenn noch freier Speicher auf der Seite bleibt, dann legen wir noch eine Block-Struktur dafuer an*/
        if((pageCount * PAGESIZE) > size)
            createMemoryNode((pageCount * PAGESIZE) - size, ((dword)ptr) + size, 1);
    }
    else
    {
        /*Ueberpruefen, ob noch ein block fuer den freien Rest erstellt werden muss*/
        if(currentMemoryNode->size > size)
        {
            createMemoryNode(currentMemoryNode->size - size, currentMemoryNode->address + size, 1);
        }
        
        currentMemoryNode->free = 0;
    }
    
    return currentMemoryNode;
}

struct memoryNode* findMemoryNodeByAddress(dword address)
{
    struct memoryNode* currentMemoryNode = firstMemoryNode;
    
    /*schaue zuerst in der Liste, ob was passendes da ist*/
    while(currentMemoryNode != NULL)
    {
        if((currentMemoryNode->address == address) && (currentMemoryNode->free == 0))
        {
            return currentMemoryNode;
        }
        currentMemoryNode = currentMemoryNode->next;
    }
    return NULL;
}


void *malloc(size_t size)
{
    if(firstMemoryNode == NULL) initMemoryManager();
    if(size < 1) return NULL;
    
    struct memoryNode* currentMemoryNode = findFirstMemoryNode(size);
    return (void*)currentMemoryNode->address;
}


void free(void* address)
{
    if(firstMemoryNode == NULL) initMemoryManager();
    
    struct memoryNode* currentMemoryNode = findMemoryNodeByAddress((dword)address);
    if(currentMemoryNode != NULL)
        currentMemoryNode->free = 1;
}
