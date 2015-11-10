/*---------------------------------------------------------------------------*/
/* file name: sg_copy.c                                                      */
/*                                                                           */
/* 1. Please find attached a header file containing definitions              */
/* for a scatter-gather data type and a list of functions to implement:      */
/*                                                                           */
/* * sg_map maps a memory region into a scatter gather list                  */
/* * sg_destroy destroys a scatter gather list                               */
/* * sg_copy copies 'count' bytes from src to dest starting at 'offset'      */
/*                                                                           */
/* * ptr_to_phys and phys_to_ptr map a pointer to a physical address         */
/*   and vice-verse                                                          */
/*   and are implemented as inline functions.  Please use those to convert   */
/*   virtual                                                                 */
/*   memory addresses into physical addresses and vice verse.                */
/*---------------------------------------------------------------------------*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "sg_copy.h"


/*---------------------------------------------------------------------------*/
/* Defines:                                                                  */
/*---------------------------------------------------------------------------*/

#if defined (TRUE)
#  undef TRUE
#endif

#if defined (FALSE)
#  undef FALSE
#endif

typedef enum {
   FALSE  = 0,
   TRUE = 1
} BOOLEAN;


/*------------------------------------------------*/
/* S/G Setters/Getters:                           */
/*------------------------------------------------*/
#define SG_SET_PADDR(SG_PTR, PADDR) (SG_PTR)->paddr = (PADDR)
#define SG_SET_COUNT(SG_PTR, COUNT) (SG_PTR)->count = (COUNT)
#define SG_SET_NEXT(SG_PTR, NEXT_PTR) (SG_PTR)->next = (NEXT_PTR)

#define SG_GET_PADDR(SG_PTR) (SG_PTR)->paddr
#define SG_GET_COUNT(SG_PTR) (SG_PTR)->count
#define SG_GET_NEXT(SG_PTR) (SG_PTR)->next

#define SG_ISNULL(SG_PTR) ((SG_PTR) == NULL)


#define MIN(A,B) ((A) < (B) ? (A) : (B)) 
#define MAX(A,B) ((A) > (B) ? (A) : (B)) 
/*---------------------------------------------------------------------------*/
/* Variables:                                                                */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Functions:                                                                */
/*---------------------------------------------------------------------------*/
char *sg_genTestBlock(int blockSize, BOOLEAN fillBlock);
sg_entry_t *sg_alloc_rec(physaddr_t paddr, int count);


/*---------------------------------------------------------------------------*/
/* main:                                                                     */
/*---------------------------------------------------------------------------*/
#if 0
int main(int argc, char **argv)
{
   int src_len, dest_len, src_offset, count;
   int bytesCopied;
   
   char *srcBlockPtr;  /* Real/Physical address of the source test block */
   char *destBlockPtr; /* Real/Physical address of the dest test block */
   
   sg_entry_t *src_sg_list;
   sg_entry_t *dest_sg_list;
   
   printf("Welcome aboard\n");
   if (argc < 5)
   {
      printf("Usage: %s src_len dest_len src_offset count\n", argv[0]);
      exit(1);
   }
   src_len = atoi(argv[1]);
   dest_len = atoi(argv[2]);
   src_offset = atoi(argv[3]);
   count = atoi(argv[4]);
   
   srcBlockPtr = sg_genTestBlock(src_len, TRUE);
   destBlockPtr = sg_genTestBlock(dest_len, FALSE);
   
   /* Allocate a S/G map for the block: */
   src_sg_list = sg_map(srcBlockPtr, src_len);
   dest_sg_list = sg_map(destBlockPtr, dest_len);
   
   bytesCopied = sg_copy(src_sg_list, dest_sg_list, src_offset, count);
   printf ("Bytes copied by sg_copy: %d\n", bytesCopied);
      
   sg_destroy(src_sg_list);
   sg_destroy(dest_sg_list);
   free(srcBlockPtr);
   free(destBlockPtr);
   printf("Finished ....\n");
}
#endif


/*---------------------------------------------------------------------------*/
/* Allocate a Test block and fill it with ASCII values                       */
/*                                                                           */
/* Parameters:                                                               */
/*    int blockSize - the size of the allocated block size.                  */
/*    BOOLEAN fillBlock - if TRUE - fill the allocated block.                */ // BUG missing '/'
/*                                                                           */
/* Returns: Pointer to the block.                                            */
/*---------------------------------------------------------------------------*/
char *sg_genTestBlock(int blockSize, BOOLEAN fillBlock)
{
   char *retBlockPtr;
   int i;

   /* Allocate the block: */
   if ((retBlockPtr = malloc(blockSize)) == NULL)
   {
      printf("Failed to allocate %d bytes for test block - cannot continue.\n", blockSize);
      exit(1);
   }
   /* Fill the block with a repeatative value: */
   if (fillBlock)
   {
      for (i = 0; i < blockSize; i++)
         retBlockPtr[i] = (char) (i % 255);
   }   
   return(retBlockPtr);
}



/*---------------------------------------------------------------------------*/
/* Allocate a scatter/gather record:                                         */
/*                                                                           */
/* Parameters:                                                               */
/*  physaddr_t paddr -  physical address                                     */
/*  int count - numer of bytes:                                              */
/*                                                                           */
/* Returns: A sg record.                                                     */
/*---------------------------------------------------------------------------*/
sg_entry_t *sg_alloc_rec(physaddr_t paddr, int count)
{
   sg_entry_t *retPtr = malloc(sizeof(sg_entry_t));
   
   if (retPtr == NULL)
   {
      printf("Failed to allocate S/G Record - cannot continue.\n");
      exit(1);
   }
   retPtr->paddr = paddr;
   retPtr->count = count;
   retPtr->next = NULL;
   return(retPtr);
}

/*---------------------------------------------------------------------------*/
/* Implementation of code:                                                   */
/*---------------------------------------------------------------------------*/


/*
 * sg_map        Map a memory buffer using a scatter-gather list
 *
 * @in buf       Pointer to buffer
 * @in length    Buffer length in bytes
 *
 * @ret          A list of sg_entry elements mapping the input buffer
 *
 * @note         Make a scatter-gather list mapping a buffer into
 *               a list of chunks mapping up to PAGE_SIZE bytes each.
 *               All entries except the first one must be aligned on a
 *               PAGE_SIZE address;
 */
sg_entry_t *sg_map(void *buf, int length)
{
   sg_entry_t *retMapPtr = NULL; /* points to the S/G map, which is created by the routine */

   sg_entry_t *newPtr; /* used for pointing to the new S/G record */
   sg_entry_t *lastPtr; 
   physaddr_t currAddr;
   int currBlockSize;
   
   int currBuffOffset = 0; /* Current offset within the input buffer */
   BOOLEAN cont = TRUE;

   // BUG: must do for ever page
#ifdef DEBUG 
   currAddr = (physaddr_t) buf;
#else
   currAddr = ptr_to_phys(buf);
#endif

   while (TRUE)
   {
	   // BUG: need to take alignment into account
      if (currBuffOffset + PAGE_SIZE - (currAddr % PAGE_SIZE) < length)
         currBlockSize = PAGE_SIZE - (currAddr % PAGE_SIZE);
      else
      {
         currBlockSize = length - currBuffOffset;
         cont = FALSE;
      }
      
      /* add a new S/G reord and chain it: */      
      newPtr = sg_alloc_rec(currAddr, currBlockSize);
      
      if (SG_ISNULL(retMapPtr))
         retMapPtr = newPtr;
      else
         SG_SET_NEXT(lastPtr, newPtr);
      
      if (!cont)
         break;
      
      lastPtr = newPtr;

      currBuffOffset += currBlockSize;
      currAddr += currBlockSize;
   }
   return(retMapPtr);
}

/*
 * sg_destroy    Destroy a scatter-gather list
 *
 * @in sg_list   A scatter-gather list
 *
 * NOTE: once the routine is called, variable, passed as sg_list parameter should be
 * reset.
 */
void sg_destroy(sg_entry_t *sg_list)
{
   sg_entry_t *currPtr; 
      
   while (sg_list != NULL)
   {
      currPtr = sg_list;
      sg_list = SG_GET_NEXT(sg_list);
      free(currPtr);
   }
}



/*
 * sg_copy       Copy bytes using scatter-gather lists
 *
 * @in src       Source sg list
 * @in dest      Destination sg list
 * @in src_offset Offset into source
 * @in count     Number of bytes to copy
 *
 * @ret          Actual number of bytes copied
 *
 * @note         The function copies "count" bytes from "src",
 *               starting from "src_offset" into the beginning of "dest".
 *               The scatter gather list can be of arbitrary length so it is
 *               possible that fewer bytes can be copied.
 *               The function returns the actual number of bytes copied
 */
int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
   int totalBytesCopied; /* The return value - # of bytes copied */
   sg_entry_t *srcPtr, *destPtr; /* pointers to members of src & dest S/G lists */
   
   int curSrcSgSize;    /* Size of the current src S/G entry */
   int currDestSgSize; /* Size of the current dest S/G entry */

   int currSrcSgOffset;  /* Offset within current S/G to start the copy from */   
   int bytesCopiedToCurrDest;
   int bytesLeftToCopyFromSrc;
   
   int bytesToCopy;
   BOOLEAN cont = TRUE;
   
   
   /* First, locate S/G entry within src, which holds the the src_offset: */
   srcPtr = src;
   while (TRUE)
   {
      curSrcSgSize = SG_GET_COUNT(srcPtr);

      if (src_offset < curSrcSgSize)
      {
         break;
      }  
      src_offset -= curSrcSgSize;
      
      srcPtr = SG_GET_NEXT(srcPtr);
      /* Have we reached the end of the Source list? */
      if (SG_ISNULL(srcPtr))
      {
         return(0);
      }
   }
   
   /* Prepare to copy: */
   totalBytesCopied = 0;
   currSrcSgOffset = src_offset;
   bytesLeftToCopyFromSrc = curSrcSgSize - src_offset;
   
   /* Next, set the pointer to the beginning */
   /* of dest and start the copying process: */
   destPtr = dest;
   while (destPtr != NULL)
   {
      currDestSgSize = SG_GET_COUNT(destPtr);
      
      /* Scan src's entries until the current dest S/G entry is filled: */
      bytesCopiedToCurrDest = 0; 
      while (cont)
      {
         /* Get how many bytes need to be copied to the current dest S/G */
         bytesToCopy = MIN((currDestSgSize - bytesCopiedToCurrDest) , bytesLeftToCopyFromSrc);
         /*bytesToCopy = MIN(currDestSgSize, bytesLeftToCopyFromSrc);*/
         
         /* Verify that's number is less than what's left to copy: */
         if (totalBytesCopied + bytesToCopy > count)
            bytesToCopy = count - totalBytesCopied; 
         
         /* Copy ... */
         memcpy((void *) (SG_GET_PADDR(destPtr) + bytesCopiedToCurrDest),
                (void *) (SG_GET_PADDR(srcPtr) + currSrcSgOffset),
                bytesToCopy);
                
         /*----------------------------------------------*/
         /* Update counters and check the current state: */
         /*----------------------------------------------*/
         totalBytesCopied += bytesToCopy;
         /* Was all data copied: */
         if (totalBytesCopied == count)
         {
            /* PLEASE DON'T PANIC - everyone has to goto sometime ... */
            goto FINISH;
         }  

         /* Was all data from src S/G copied?   */
         /* If so, skip to the next S/G record: */
         bytesLeftToCopyFromSrc -= bytesToCopy;
         if (bytesLeftToCopyFromSrc == 0)
         {
            srcPtr = SG_GET_NEXT(srcPtr);
            if (SG_ISNULL(srcPtr))
            {
               /* PLEASE DON'T PANIC - everyone has to goto sometime ... */
               goto FINISH;
            }
            
            bytesLeftToCopyFromSrc = SG_GET_COUNT(srcPtr);
            currSrcSgOffset = 0;
         }
         else /* Else - set next copy location from src S/G: */
            currSrcSgOffset += bytesToCopy;
         
         /* Finished filling current dest S/G? */
         bytesCopiedToCurrDest += bytesToCopy;
         if (bytesCopiedToCurrDest == currDestSgSize)
         {
            break;
         }
      }
      
      /* Move to the next dest S/G: */         
      destPtr = SG_GET_NEXT(destPtr);
   }
   
FINISH:   
   return(totalBytesCopied);
}


