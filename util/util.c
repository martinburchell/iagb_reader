/*****************************************************************************/
/* File: util.c                                                              */
/* Part of library: utillib                                                  */
/*****************************************************************************/

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "util.h"

#if DEBUG
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

static void util_list_blocks(void)
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

extern void util_report_leaks()
{
    if (g_mem_list)
    {
        fprintf(stderr,"There were leaks:\n");
        
        util_list_blocks();
    }
    else
    {
        fprintf(stderr,"No leaks reported\n");
    }
}

extern void *util_malloc_debug(size_t size,
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
       /* do something more intelligent here */
       fprintf(stderr,"Ran out of memory\n");

       exit(EXIT_FAILURE);
   }

   new_block = malloc(sizeof(struct block));
   if (!new_block)
   {
       /* do something more intelligent here */
       fprintf(stderr,"Ran out of memory\n");

       exit(EXIT_FAILURE);
   }

   new_block->ptr = ptr;
   new_block->size = size;
   new_block->file = malloc(strlen(file)+1);
   if (!new_block->file)
   {
       /* do something more intelligent here */
       fprintf(stderr,"Ran out of memory\n");

       exit(EXIT_FAILURE);
   }

   strcpy(new_block->file,file);
   new_block->line = line;

   new_block->next = g_mem_list;
   g_mem_list = new_block;

#if 0
   util_list_blocks();
#endif

   return ptr;
}

extern void util_free_debug(void *ptr,char *file,int line)
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
                    "Attempt to free unknown block %p with util_free():\n%s, line %d\n",
                    ptr,file,line);

            assert(FALSE);
        }

        free(ptr);

#if 0
        util_list_blocks();
#endif

    }
}

#else

/* No debug equivalents */

extern void *util_malloc(size_t size)
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

extern void util_free(void *ptr)
{
    if (ptr)
    {
        free(ptr);
    }
}

#endif

extern void util_set_string(char **out_string,const char* in_string)
{
   char *ret_string;

   ret_string = util_malloc(strlen(in_string)+1);

   strcpy(ret_string,in_string);

   *out_string = ret_string;
}

extern void util_set_string_if_null(char **ret_string,char* in_string)
{
   if (*ret_string == NULL)
   {
      util_set_string(ret_string,in_string);
   }
}

extern void util_downcase_string(char **out_string,char *in_string)
{
   char *ret_string;
   char *in_ptr,*out_ptr;

   ret_string = util_malloc(strlen(in_string)+1);

   out_ptr = ret_string;
   for (in_ptr = in_string; *in_ptr != '\0'; in_ptr++)
      *out_ptr++ = (char) tolower((int) *in_ptr);

   *out_ptr = '\0';

   *out_string = ret_string;
}

extern void util_upcase_string(char **out_string,char *in_string)
{
   char *ret_string;
   char *in_ptr,*out_ptr;

   ret_string = util_malloc(strlen(in_string)+1);

   out_ptr = ret_string;
   for (in_ptr = in_string; *in_ptr != '\0'; in_ptr++)
      *out_ptr++ = (char) toupper((int) *in_ptr);

   *out_ptr = '\0';

   *out_string = ret_string;
}

extern void util_str_is_print(char *in_string,int *is_print)
{
   int i;
   int len;

   *is_print = 0;

   if (in_string == NULL)
      return;

   len = strlen(in_string);

   for (i=0; (!isprint((unsigned char)in_string[i]) && i<len); i++);

   if (i<len)
      *is_print=isprint((unsigned char)in_string[i]);
}
