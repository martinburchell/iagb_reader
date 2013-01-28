#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "memlib.h"

/* TODO #if DEBUG */
#if 1

typedef struct block *BLOCK;

struct block
{
    void *ptr;
    size_t size;
    char *file;
    int line;
    BLOCK next;
};

static BLOCK g_mem_list = NULL;

/* this function must not return NULL */

static void mem_list_blocks(void)
{
    BLOCK this_block = g_mem_list;

    fprintf(stderr,
            "Currently allocated memory\n\n");

    while (this_block)
    {
        fprintf(stderr,
                "Block %p size %d allocated from file %s, line %d\n",
                this_block->ptr,
                this_block->size,
                this_block->file,
                this_block->line);

        this_block = this_block->next;
    };

    fprintf(stderr,"\n\n");
}

extern void mem_report_leaks()
{
    if (g_mem_list)
    {
        fprintf(stderr,"\n\nThere were leaks:\n");
        
        mem_list_blocks();
    }
}

extern void *mem_malloc_debug(size_t size,
                               char *file,
                               int line)
{
   void  *ptr;

   BLOCK new_block;

   ptr = malloc(size);
   
   if (ptr != NULL)
   {
       char  *i,*end;

       i = ptr;
       end = i + size;
   
       do
       {
           *i = '?';
   
           i++;
       } while (i<end);
   }

   if (!ptr)
   {
       /* TODO do something more intelligent here */
       fprintf(stderr,"Ran out of memory\n");

       exit(EXIT_FAILURE);
   }

   new_block = malloc(sizeof(struct block));
   if (!new_block)
   {
       /* TODO do something more intelligent here */
       fprintf(stderr,"Ran out of memory\n");

       exit(EXIT_FAILURE);
   }

   new_block->ptr = ptr;
   new_block->size = size;
   new_block->file = malloc(strlen(file)+1);
   if (!new_block->file)
   {
       /* TODO do something more intelligent here */
       fprintf(stderr,"Ran out of memory\n");

       exit(EXIT_FAILURE);
   }

   strcpy(new_block->file,file);
   new_block->line = line;

   new_block->next = g_mem_list;
   g_mem_list = new_block;

#if 0
   mem_list_blocks();
#endif

   return ptr;
}

extern void mem_free_debug(void *ptr,char *file,int line)
{
    if (ptr)
    {
        BLOCK prev_block = NULL;
        BLOCK this_block = g_mem_list;

        while (this_block && (this_block->ptr != ptr))
        {
            prev_block = this_block;
            this_block = this_block->next;
        };

        if (this_block)
        {
            if (prev_block)
            {
                /* wasn't first in the list */
                prev_block->next = this_block->next;
            }
            else
            {
                /* first in the list */
                g_mem_list = this_block->next;
            }

            free(this_block->file);
            free(this_block);
        }
        else
        {
            /* got to the end of the list */
            fprintf(stderr,
                    "Attempt to free unknown block %p with mem_free():\n%s, line %d\n",
                    ptr,file,line);

            assert(0);
        }

        free(ptr);

#if 0
        mem_list_blocks();
#endif

    }
}

#else

/* No debug equivalents */

extern void *mem_malloc(size_t size)
{
    void  *ptr;

    ptr = malloc(size);

    if (!ptr)
    {
        /* do something more intelligent here */
        fprintf(stderr,"Ran out of memory\n");

        exit(EXIT_FAILURE);
    }

    return ptr;
}

extern void mem_free(void *ptr)
{
    if (ptr)
    {
        free(ptr);
    }
}

#endif
