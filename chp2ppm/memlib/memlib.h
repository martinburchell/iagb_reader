#ifndef MEM_H
#define MEM_H

#ifdef __cplusplus
extern "C"
{
#endif

/* TODO #if DEBUG */
#if 1

extern void mem_report_leaks(void);

extern void *mem_malloc_debug(size_t size,
                              char *file,
                              int line);

extern void mem_free_debug(void *ptr,char *file,int line);

#define mem_malloc(x) mem_malloc_debug(x,(char *)__FILE__,__LINE__)
#define mem_free(x) mem_free_debug(x,(char *)__FILE__,__LINE__)

#else

#define mem_report_leaks(x)

extern void *mem_malloc(size_t size);
extern void mem_free(void *ptr);

#endif

#ifdef __cplusplus
    }
#endif

#endif /* MEM_H */
