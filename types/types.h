#ifndef TYPES_H
#define TYPES_H

#ifdef NDEBUG
#define DEBUG 0
#else
#define DEBUG 1
#endif

#ifndef BOOL
typedef int BOOL;
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#  define TRUE (!FALSE)
#endif

#ifndef NOT_USED
#define NOT_USED(x) x=x
#endif

#endif /* TYPES_H */
