/*****************************************************************************/
/* File: util.h                                                              */
/* Part of library: utillib                                                  */
/*****************************************************************************/

#ifndef UTIL_H
#define UTIL_H

#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif

#if DEBUG

extern void util_report_leaks(void);

extern void *util_malloc_debug(size_t size,
                               char *file,
                               int line);

extern void util_free_debug(void *ptr,char *file,int line);

#define util_malloc(x) util_malloc_debug(x,(char *)__FILE__,__LINE__)
#define util_free(x) util_free_debug(x,(char *)__FILE__,__LINE__)

#else

#define util_report_leaks(x)

extern void *util_malloc(size_t size);
extern void util_free(void *ptr);

#endif

extern void util_set_string(char **out_string,const char* in_string);

extern void util_set_string_if_null(char **ret_string,char* in_string);

extern void util_downcase_string(char **out_string,char *in_string);

extern void util_upcase_string(char **out_string,char *in_string);

extern void util_str_is_print(char *in_string,int *is_print);

#ifdef __cplusplus
    }
#endif

#endif /* UTIL_H */
