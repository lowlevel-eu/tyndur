#include "types.h"
#include "string.h"

#define PAGESIZE 4096
#define _PDCLIB_MINALLOC 1

extern void* mem_allocate(dword size, dword flags);

struct _PDCLIB_headnode_t
{
    struct _PDCLIB_memnode_t * first;
    struct _PDCLIB_memnode_t * last;
};
 
struct _PDCLIB_memnode_t
{
    size_t size;
    struct _PDCLIB_memnode_t * next;
};

struct _PDCLIB_headnode_t _PDCLIB_memlist = { NULL, NULL };


void * malloc( size_t size )
{
    if ( size == 0 )
    {
        return NULL;
    }
    if ( size < _PDCLIB_MINALLOC )
    {
        size = _PDCLIB_MINALLOC;
    }
    {
    struct _PDCLIB_memnode_t * current = _PDCLIB_memlist.first;
    struct _PDCLIB_memnode_t * previous = NULL;
    struct _PDCLIB_memnode_t * firstfit = NULL;
    struct _PDCLIB_memnode_t * firstfit_previous = NULL;
    /* Trying exact fit 
    while ( current != NULL )
    {
        if ( current->size == size )
        {
            /* Found exact fit, allocate node 
            if ( previous != NULL )
            {
                /* Node in the middle of the list 
                previous->next = current->next;
            }
            else
            {
                /* Node is first in list 
                _PDCLIB_memlist.first = current->next;
            }
            if ( _PDCLIB_memlist.last == current )
            {
                /* Node is last in list/
                _PDCLIB_memlist.last = previous;
            }
            return (char *)current + sizeof( struct _PDCLIB_memnode_t );
        }
        else if ( current->size > size && ( firstfit == NULL || current->size < firstfit->size ) )
        {
            /* Remember previous node in case we do not get an exact fit.
               Note that this is the node *pointing to* the first fit,
               as we need that for allocating (i.e., changing next pointer).
            
            firstfit_previous = previous;
            firstfit = current;
        }
        /* Skip to next node 
        previous = current;
        current = current->next;
    }
    /* No exact fit; go for first fit */
    if ( firstfit != NULL )
    {
        if ( ( firstfit->size - size ) > _PDCLIB_MINALLOC )
        {
            /* Oversized - split into two nodes */
            struct _PDCLIB_memnode_t * newnode = (struct _PDCLIB_memnode_t *)( (char *)firstfit + sizeof( struct _PDCLIB_memnode_t ) + size );
            newnode->size = firstfit->size - size - sizeof( struct _PDCLIB_memnode_t );
            newnode->next = firstfit->next;
            firstfit->next = newnode;
            firstfit->size = firstfit->size - newnode->size - sizeof( struct _PDCLIB_memnode_t );
        }
        if ( firstfit_previous != NULL )
        {
            /* Node in the middle of the list */
            firstfit_previous->next = firstfit->next;
        }
        else
        {
            /* Node is first in list */
            _PDCLIB_memlist.first = firstfit->next;
        }
        if ( _PDCLIB_memlist.last == firstfit )
        {
            /* Node is last in list */
            _PDCLIB_memlist.last = firstfit_previous;
        }
        return (char *)firstfit + sizeof( struct _PDCLIB_memnode_t );
    }
    }
    {
    /* No fit possible; how many additional pages do we need? */
    int pages = ( ( size + sizeof( struct _PDCLIB_memnode_t ) - 1 ) / PAGESIZE ) + 1;
    /* Allocate more pages */
    struct _PDCLIB_memnode_t * newnode = (struct _PDCLIB_memnode_t *)mem_allocate( pages * PAGESIZE, 0 );
    if ( newnode != NULL )
    {
        newnode->next = NULL;
        newnode->size = pages * PAGESIZE - sizeof( struct _PDCLIB_memnode_t );
        if ( ( newnode->size - size ) > ( _PDCLIB_MINALLOC + sizeof( struct _PDCLIB_memnode_t ) ) )
        {
            /* Oversized - split into two nodes */
            struct _PDCLIB_memnode_t * splitnode = (struct _PDCLIB_memnode_t *)( (char *)newnode + sizeof( struct _PDCLIB_memnode_t ) + size );
            splitnode->size = newnode->size - size - sizeof( struct _PDCLIB_memnode_t );
            newnode->size = size;
            /* Add splitted node as last element to free node list */
            if ( _PDCLIB_memlist.last == NULL )
            {
                _PDCLIB_memlist.first = splitnode;
            }
            else
            {
                _PDCLIB_memlist.last->next = splitnode;
            }
	    splitnode->next = NULL; /* TODO: This is bug #1514883, uncovered by testdriver yet. */
            _PDCLIB_memlist.last = splitnode;
        }
        return (char *)newnode + sizeof( struct _PDCLIB_memnode_t );
    }
    }
    /* No fit, heap extension not possible - out of memory */
    return NULL;
}





void free( void * ptr )
{
    ptr = (void *)( (char *)ptr - sizeof( struct _PDCLIB_memnode_t ) );
    ( (struct _PDCLIB_memnode_t *)ptr )->next = NULL;
    if ( _PDCLIB_memlist.last != NULL )
    {
        _PDCLIB_memlist.last->next = ptr;
    }
    else
    {
        _PDCLIB_memlist.first = ptr;
    }
    _PDCLIB_memlist.last = ptr;
}


void * realloc( void * ptr, size_t size )
{
    void * newptr = NULL;
    if ( ptr == NULL )
    {
        return malloc( size );
    }
    if ( size > 0 )
    {
        struct _PDCLIB_memnode_t * baseptr = (struct _PDCLIB_memnode_t *)( (char *)ptr - sizeof( struct _PDCLIB_memnode_t ) );
        if ( baseptr->size >= size )
        {
            /* Current memnode is large enough; nothing to do. */
            return ptr;
        }
        else
        {
            /* Get larger memnode and copy over contents. */
            if ( ( newptr = malloc( size ) ) == NULL )
            {
                return NULL;
            }
            memcpy( newptr, ptr, baseptr->size );
        }
    }
    free( ptr );
    return newptr;
}




