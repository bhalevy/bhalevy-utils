#include <stdio.h>
#include <string.h>
#include "sg_copy.h"

/* Return: 0 = equal, -1 = not the name length, 1 = not the same value */
int bufferTest(byte_t* buffer, sg_entry_t* src, int size)
{
    int offset = 0;
    int i;
    for (i = 0; i < size - 1; i++)
    {
        /* Compare bytes */
        if (buffer[i] != ((byte_t*)phys_to_ptr(src->paddr))[offset++])
            return 1;

        /* Next page */
        if (offset == src->count)
        {
            src = src->next;
            if (src == NULL)
                return -1;
            offset = 0;
        }
    }

    /* Compare last byte */
    if (buffer[i] != ((byte_t*)phys_to_ptr(src->paddr))[offset++])
        return 1;

    if (offset != src->count)
        return -1;

    return 0;
}

int main(int argc, char* argv[])
{
    byte_t      buffer1[PAGE_SIZE * 3];
    byte_t      buffer1_copy[sizeof(buffer1)];
    byte_t      buffer2[sizeof(buffer1)];
    byte_t      buffer3[sizeof(buffer1)];
    byte_t      buffer4[sizeof(buffer1)];

    sg_entry_t *list1, *list2, *list3, *list4;
    int         i_bufferOffset, i_copyOffset, bufferNewSize, i_copySize, copySizeMax, ret;

    /* Prepare for test */
    for (i_bufferOffset = 0; i_bufferOffset < sizeof(buffer1); i_bufferOffset++)
        buffer1[i_bufferOffset] = (byte_t)i_bufferOffset;

    memcpy(buffer1_copy, buffer1, sizeof(buffer1_copy));

    memset(buffer2, 0, sizeof(buffer2));
    list2 = sg_map(buffer2, sizeof(buffer2));
    if (list2 == NULL)
    {
        printf("Error: Line=%u - stop test!\n", __LINE__);
        return 1;
    }

    if ((ret = bufferTest(buffer2, list2, sizeof(buffer2))) != 0)
        printf("Error: Line=%u, ret=%d, expected=%d\n", __LINE__, ret, 0);

    memset(buffer3, 0, sizeof(buffer3));
    list3 = sg_map(buffer3, sizeof(buffer3));
    if (list3 == NULL)
    {
        printf("Error: Line=%u - stop test!\n", __LINE__);
        return 1;
    }

    if ((ret = bufferTest(buffer3, list3, sizeof(buffer3))) != 0)
        printf("Error: Line=%u, ret=%d, expected=%d\n", __LINE__, ret, 0);

    /* Test sg_copy */
    for (i_bufferOffset = 0; i_bufferOffset < sizeof(buffer1); i_bufferOffset++)
    {
        bufferNewSize = sizeof(buffer1) - i_bufferOffset;

        list1 = sg_map(&(buffer1[i_bufferOffset]), bufferNewSize);
        if (list1 == NULL)
        {
            printf("Error: Line=%u - stop test!\n", __LINE__);
            return 1;
        }

        list4 = sg_map(&(buffer4[i_bufferOffset]), bufferNewSize);
        if (list4 == NULL)
        {
            printf("Error: Line=%u - stop test!\n", __LINE__);
            return 1;
        }

        if ((ret = bufferTest(&(buffer1_copy[i_bufferOffset]), list1, bufferNewSize)) != 0)
            printf("Error: Line=%u, Index=%d, ret=%d, expected=%d\n", __LINE__, i_bufferOffset, ret, 0);

        for (i_copyOffset = 0; i_copyOffset < bufferNewSize; i_copyOffset++)
        {
            /* Content aspect */
            copySizeMax = bufferNewSize - i_copyOffset;
            for (i_copySize = 0; i_copySize <= copySizeMax; i_copySize++)
            {
                if ((ret = sg_copy(list1, list2, i_copyOffset, i_copySize)) != i_copySize)
                    printf("Error: Line=%u, Index=%d.%d.%d, ret=%d, expected=%d\n", __LINE__, i_bufferOffset, i_copyOffset, i_copySize, ret, i_copySize);
                if ((ret = memcmp(&(buffer1_copy[i_bufferOffset + i_copyOffset]), buffer2, i_copySize)) != 0)
                    printf("Error: Line=%u, Index=%d.%d.%d, ret=%d, expected=%d\n", __LINE__, i_bufferOffset, i_copyOffset, i_copySize, ret, 0);
            }

            /* Size aspect */
            if ((ret = sg_copy(list4, list3, i_copyOffset, sizeof(buffer1))) != copySizeMax)
                printf("Error: Line=%u, Index=%d.%d, ret=%d, expected=%d\n", __LINE__, i_bufferOffset, i_copyOffset, ret, copySizeMax);
            if ((ret = sg_copy(list3, list4, 0, sizeof(buffer1))) != bufferNewSize)
                printf("Error: Line=%u, Index=%d.%d, ret=%d, expected=%d\n", __LINE__, i_bufferOffset, i_copyOffset, ret, bufferNewSize);
        }

        sg_destroy(list1);
        sg_destroy(list4);
    }

    sg_destroy(list2);
    sg_destroy(list3);

    /* Verify bufferTest */
    list1 = sg_map(buffer1, sizeof(buffer1));
    memcpy(buffer2, buffer1, sizeof(buffer2));

    if ((ret = bufferTest(&(buffer2[1]), list1, sizeof(buffer2))) != 1)
        printf("Error: Line=%u, ret=%d, expected=%d\n", __LINE__, ret, 1);
    if ((ret = bufferTest(buffer2, list1, sizeof(buffer2) -1)) != -1)
        printf("Error: Line=%u, ret=%d, expected=%d\n", __LINE__, ret, -1);

    for (i_bufferOffset = 0; i_bufferOffset < sizeof(buffer1); i_bufferOffset++)
    {
        buffer2[i_bufferOffset]++;
        if ((ret = bufferTest(buffer2, list1, sizeof(buffer2))) != 1)
            printf("Error: Line=%u, ret=%d, expected=%d\n", __LINE__, ret, 1);
        buffer2[i_bufferOffset]--;
    }

    if ((ret = bufferTest(buffer2, list1, sizeof(buffer2))) != 0)
        printf("Error: Line=%u, ret=%d, expected=%d\n", __LINE__, ret, 0);

    sg_destroy(list1);

    printf("Done\n");

    return 0;
}

