
#include <stdlib.h>
#include <string.h>
#include "sg_copy.h"

sg_entry_t *sg_map(void *buf, int length)
{
    sg_entry_t *head = NULL;
    sg_entry_t **entry = &head;
    char *p = (char *)buf;

    for (entry = &head; length; entry = &(*entry)->next)
    {
        *entry = malloc(sizeof(sg_entry_t));
        if (*entry == NULL)
        {
            if (head)
                sg_destroy(head);
            return NULL;
        }

        (*entry)->paddr = ptr_to_phys(p);
        (*entry)->count = PAGE_SIZE - (((unsigned long)p) & (PAGE_SIZE - 1));
        (*entry)->next = NULL;

        if (length <= (*entry)->count)
        {
            /* this can only happen for the last entry */
            (*entry)->count = length;
            break;
        }

        /* prepare for the next entry... */
        p += (*entry)->count;
        length -= (*entry)->count;
    }

    return head;
}

void sg_destroy(sg_entry_t *sg_list)
{
    sg_entry_t *next;

    while (sg_list)
    {
        next = sg_list->next;
        free(sg_list);
        sg_list = next;
    }
}

int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
    int copied = 0;
    int s_len;
    int d_len = 0;
    char *s;
    char *d;

    /* skip offset */
    for (; src && src_offset > src->count; src = src->next)
    {
        src_offset -= src->count;
    }

    if (!src)
       return 0;

    s_len = -src_offset;

    /* start walking src */
    for (; src && dest && count; src = src->next)
    {
        /* prepare source information */
        s = ((char *)phys_to_ptr(src->paddr)) - s_len;
        s_len += src->count;
        if (s_len > count)
            s_len = count;
        count -= s_len;

        /* copy to dest */
        while (dest)
        {
            d = ((char *)phys_to_ptr(dest->paddr)) + d_len;
            d_len = dest->count - d_len;

            if (d_len >= s_len)
                d_len = s_len;

            memcpy(d, s, d_len);

            copied += d_len;
            s_len -= d_len;
            if (!s_len)
                break;

            s += d_len;
            dest = dest->next;
            d_len = 0;
        }
    }

    return copied;
}
