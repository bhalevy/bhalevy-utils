// BH
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sg_copy.h"

#define MIN(a, b) ( (a < b) ? a : b )
#define MIN3(a,b,c) ( (a < MIN(b,c)) ? a : MIN(b,c) )

static sg_entry_t * new_entry()
{
    sg_entry_t * p = malloc(sizeof(sg_entry_t));

// BH: what's wrong with calloc?
    if(p) memset(p, sizeof(sg_entry_t), 0);
    return(p);
}

/* I use "inert_node" [O(1)]  and "reverse_list" [O(N)] */

static int insert_node(sg_entry_t ** root, sg_entry_t **node)
{
    sg_entry_t * tmp = *root;
    (*node)->next = *root;
    *root = *node;
    *node = tmp;

    return(0);
}

static sg_entry_t * extract_node(sg_entry_t ** root)
{
    sg_entry_t *tmp = *root;
// BH: questionable style
    if(*root) (*root)  = (*root)->next;
    return(tmp);
}

// BH: why?
static void reverse_list(sg_entry_t ** list)
{
    sg_entry_t * tmp = NULL;
    sg_entry_t * n = NULL;
    while ( (*list) )
    {
        n = extract_node(list);
        insert_node(&tmp, &n);
    }

    *list = tmp;
}


void sg_destroy(sg_entry_t * list)
{
    sg_entry_t * n;

    do {
        n = extract_node(&list);
        if(n) free(n);
    } while(n);
}

// BH: what the hack?
#define BUFEND() (( (char *) buf) + length)

sg_entry_t * sg_map(void *buf, int length)
{
    sg_entry_t * list = NULL;
    sg_entry_t * node;

    /* I suppose that sizeof(char) == 1; 
     * If this code going to be used in a system where it is > 1, 
     * the type should be ajusted */

    char * p = buf;
    
// BH: debug print
//    printf("Got buf: %p, length: %i\n", buf, length);

    if (!p || length <= 0)
        return NULL;

    /* Every next chunk must be aligned by the PAGE_SIZE */
    while ( p < BUFEND() )
    {
        node = new_entry();
        if(!node)
        {
            sg_destroy(list);
            return(NULL);
        }

        node->paddr = ptr_to_phys(p);
        node->count = MIN( (PAGE_SIZE - ( (int) p % PAGE_SIZE )), ( (int) (BUFEND() - p))  ); 
     
        p += node->count;
        
        insert_node(&list, &node);
    }

// why not just append new entries at the first place???
    reverse_list(&list);

    return(list);
}

#undef BUFEND()


int sg_copy(sg_entry_t *s, sg_entry_t * d, int offset, int count)
{
    int src_off = 0;
    int dst_off = 0;

    char * sc;
    char * dc;

    int i = 0;
    int j = 0;

// BH: BUG: no input validation
if (offset < 0 || count <= 0 || !s || !d) return 0;

    /* Scroll the offset */
    while(i < offset)
    {
// BH: overly complicated
        if (s->count < (offset - i) )
        {
            i+= s->count;
            s = s->next;
        }
        else
        {
            src_off = (offset - i);
            i = offset;
        }

        if (!s)
            return(0);
    }

    i = 0;

    while (i < count)
    {
        /* What is the block size to copy? */
        j = MIN3( (s->count - src_off), (d->count - dst_off), (count - i) );

        sc = (char *) phys_to_ptr(s->paddr) ;
        dc = (char *) phys_to_ptr(d->paddr) ;

        memcpy(dc + dst_off , sc + src_off, j);

        i       += j;
        src_off += j;
        dst_off += j;

        if(i == count) return(i);

        if (src_off == s->count)
        {
// style?
            if (s->next) s = s->next;
            else return (i);
            src_off = 0;
        }

        if (dst_off == d->count)
        {
            if (d->next) d = d->next;
            else return(i);
            dst_off = 0;
        }
    }

    return(i);
}

