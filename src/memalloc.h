/*
 * Wrappers for malloc/realloc/free:
 * abort if out of memory.
 */

#ifndef __MEMALLOC_H
#define __MEMALLOC_H

#include <stddef.h>
#include <stdlib.h>


/*
 * Print an error message then call exit(YICES_EXIT_OUT_OF_MEMORY)
 * - this exit code is defined in yices_exit_codes.h
 */
extern void out_of_memory(void) __attribute__ ((noreturn));

/*
 * Wrappers for malloc/realloc.
 */
extern void *safe_malloc(size_t size) __attribute__ ((malloc));
extern void *safe_realloc(void *ptr, size_t size) __attribute__ ((malloc));


/*
 * Safer free: used to check whether ptr is NULL before calling free.
 *
 * Updated to just call free. The check is redundant on most
 * systes, The C99 standarrd specifies that free shall have no effect
 * if ptr is NULL.
 *
 */
static inline void safe_free(void *ptr)  {
  //  if (ptr != NULL) free(ptr);
  free(ptr);
}


#endif