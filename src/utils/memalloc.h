/*
 * This file is part of the Yices SMT Solver.
 * Copyright (C) 2017 SRI International.
 *
 * Yices is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Yices is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Yices.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * WRAPPERS FOR MALLOC/REALLOC/FREE
 *
 * safe_malloc and safe_realloc abort if we run out of memory.
 */

#ifndef __MEMALLOC_H
#define __MEMALLOC_H

#include <stdlib.h>
#include <yices_config.h>

/*
 * Pointer to a callback function:
 * - when we run out-of-memory, then function __out_of_mem_callback is
 *   called first, if the pointer is not null.
 * - if the function returns, then we kill the process with a
 *   call to exit(YICES_EXIT_OUT_OF_MEMORY)
 *
 * If there's no callback function registered (i.e., __out_of_mem_callback)
 * is NULL, then we print an error message on stderr then call
 * exit(YICES_EXIT_OUT_OF_MEMORY).
 *   
 */
typedef void (*out_of_mem_callback_t)(void);
YICES_EXTERN out_of_mem_callback_t __out_of_mem_callback;

/*
 * Either calls longjmp(out_of_mem, -1) or prints an error message then
 * calls exit(YICES_EXIT_OUT_OF_MEMORY)
 * - this exit code is defined in yices_exit_codes.h
 */
YICES_EXTERN ATTRIBUTE_NORETURN void out_of_memory(void);

/*
 * Wrappers for malloc/realloc.
 */
YICES_EXTERN ATTRIBUTE_MALLOC void *safe_malloc(size_t size);
YICES_EXTERN ATTRIBUTE_MALLOC void *safe_realloc(void *ptr, size_t size);


/*
 * Safer free: used to check whether ptr is NULL before calling free.
 *
 * Updated to just call free. The check is redundant on most
 * systems. The C99 standard specifies that free shall have no effect
 * if ptr is NULL.
 *
 */
static inline void safe_free(void *ptr)  {
  //  if (ptr != NULL) free(ptr);
  free(ptr);
}


/*
 * Wrapper for strdup
 */
YICES_EXTERN char *safe_strdup(const char *s);

#endif
