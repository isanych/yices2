/*
 * PRETTY PRINTER FOR YICES TYPES AND TERMS
 */

#ifndef __YICES_PP_H
#define __YICES_PP_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <gmp.h>

#include "object_stores.h"
#include "pretty_printer.h"
#include "rationals.h"
#include "string_buffers.h"


/*
 * ATOMIC OBJECTS
 */

/*
 * Each atomic tokens stores a basic object to be printed as
 * a single string. It consists of a pp_atomic_token prefix +
 * extra data that describes the actual object to be printed.
 * The user_tag field in the prefix stores the object type.
 */
typedef enum pp_atom_type {
  PP_CHAR_ATOM,    // content = a single char
  PP_STRING_ATOM,  // content = string terminated by '\0'
  PP_ID_ATOM,      // identifier = concatenation of a string and an index
  PP_VARID_ATOM,   // variant id = concatenation of a string, '!', and an index
  PP_TRUE_ATOM,    // true
  PP_FALSE_ATOM,   // false
  PP_INT32_ATOM,   // signed integer
  PP_UINT32_ATOM,  // unsigned integer
  PP_RATIONAL_ATOM, // rational
  PP_BV64_ATOM,    // bitvector constant stored in a 64bit unsigned integer
  PP_BV_ATOM,      // bitvector constant stores in an array of words
} pp_atom_type_t;

#define NUM_PP_ATOMS ((uint32_t) (PP_BV_ATOM+1))


/*
 * Descriptors of ID, BV, BV64 atoms
 */
typedef struct pp_id_s {
  const char *prefix;
  int32_t index;
} pp_id_t;

typedef struct pp_bv64_s {
  uint64_t bv;
  uint32_t nbits;
} pp_bv64_t;

typedef struct pp_bv_s {
  uint32_t *bv;
  uint32_t nbits;
} pp_bv_t;



/*
 * Full atomic token
 */
typedef struct pp_atom_s {
  pp_atomic_token_t tk; // prefix defined in pretty_printer.h
  union {
    char c;
    const char *string;
    pp_id_t id;
    int32_t i32;
    uint32_t u32;
    rational_t rat;
    pp_bv64_t bv64;
    pp_bv_t bv;
  } data;
} pp_atom_t;




/*
 * OPEN-BLOCK TOKENS
 */

/*
 * Each open-block token is defined by an identifier.
 * For each identifier, the module maintains the following
 * information in internal tables:
 * - string label
 * - label size
 * - preferred format for that block
 * - indentation and short indentation
 * - two boolean flags (sep allowed + parenthesis for that block)
 */

// list of open-block identifiers
typedef enum {
  PP_OPEN,               // empty label, no parenthesis
  PP_OPEN_PAR,           // empty label, open parenthesis

  PP_OPEN_BV_TYPE,
  PP_OPEN_FUN_TYPE,
  PP_OPEN_TUPLE_TYPE,

  PP_OPEN_ITE,
  PP_OPEN_UPDATE,
  PP_OPEN_TUPLE,
  PP_OPEN_SELECT,
  PP_OPEN_EQ,
  PP_OPEN_NEQ,
  PP_OPEN_DISTINCT,
  PP_OPEN_FORALL,
  PP_OPEN_EXISTS,
  PP_OPEN_LAMBDA,
  PP_OPEN_NOT,
  PP_OPEN_OR,
  PP_OPEN_AND,
  PP_OPEN_XOR,
  PP_OPEN_IMPLIES,
  PP_OPEN_BIT,
  PP_OPEN_PROD,
  PP_OPEN_POWER,
  PP_OPEN_SUM,
  PP_OPEN_GE,
  PP_OPEN_LT,

  PP_OPEN_BV_ARRAY,
  PP_OPEN_BV_SUM,
  PP_OPEN_BV_PROD,
  PP_OPEN_BV_POWER,
  PP_OPEN_BV_DIV,
  PP_OPEN_BV_REM,
  PP_OPEN_BV_SDIV,
  PP_OPEN_BV_SREM,
  PP_OPEN_BV_SMOD,
  PP_OPEN_BV_SHL,
  PP_OPEN_BV_LSHR,
  PP_OPEN_BV_ASHR,
  PP_OPEN_BV_GE,
  PP_OPEN_BV_LT,
  PP_OPEN_BV_SGE,
  PP_OPEN_BV_SLT,

  // blocks used in pp_model
  PP_OPEN_FUNCTION,   // (function ...)
  PP_OPEN_TYPE,       // (type ..)
  PP_OPEN_DEFAULT,    // (default x)

  PP_OPEN_CONST_DEF,  // (constant i of <type>)
  PP_OPEN_UNINT_DEF,  // (unint i of <type>)
  PP_OPEN_VAR_DEF,    // (var i of <type>)

} pp_open_type_t;

#define NUM_PP_OPENS ((uint32_t) (PP_OPEN_VAR_DEF + 1))



/*
 * CLOSE-BLOCK TOKENS
 */

/*
 * Two versions: close with a parenthesis or close with nothing
 */
typedef enum {
  PP_CLOSE,
  PP_CLOSE_PAR,
} pp_close_type_t;




/*
 * FULL PRETTY PRINTER
 */

/*
 * - pp: pretty printer object
 * - open_store: for allocation of open-block tokens
 * - atom_store: for allocation of atomic tokens
 * - two statically allocated close tokens
 * - a string buffer for conversion of atoms to strings
 */
typedef struct yices_pp_s {
  pp_t pp;
  object_store_t open_store;
  object_store_t atom_store;
  pp_close_token_t close_nopar;
  pp_close_token_t close_par;
  void *close[2];  // close[0] = nopar, close[1] = par
  string_buffer_t buffer;
} yices_pp_t;




/*
 * Initialize the internal table of open-token descriptors
 * - this must be called first.
 */
extern void init_yices_pp_tables(void);

/*
 * Initialize a pretty printer
 * - file = output file (must be open for write)
 * - area = display area (cf. pretty_printer.h)
 * - mode = initial print mode (cf. pretty printer.h)
 * - indent = initial indentation
 * If area is NULL, then the default is used (cf. pretty_printer.h)
 */
extern void init_yices_pp(yices_pp_t *printer, FILE *file, const pp_area_t *area,
                          pp_print_mode_t mode, uint32_t indent);


/*
 * Variant: use default mode and indent
 */
static inline void init_default_yices_pp(yices_pp_t *printer, FILE *file, const pp_area_t *area) {
  init_yices_pp(printer, file, area, PP_VMODE, 0);
}


/*
 * Flush: print everything pending + a newline
 * - then reset the line counter to 0
 */
extern void flush_yices_pp(yices_pp_t *printer);


/*
 * Delete a pretty printer
 * - if flush is true, print everything pending + a newline
 * - then free all memory used
 */
extern void delete_yices_pp(yices_pp_t *printer, bool flush);


/*
 * Check for saturation: when this is true, we should stop sending tokens
 */
static inline bool yices_pp_is_full(yices_pp_t *printer) {
  return pp_is_full(&printer->pp);
}


/*
 * Get the print depth = number of open blocks sent to the printer
 */
static inline uint32_t yices_pp_depth(yices_pp_t *printer) {
  return pp_depth(&printer->pp);
}


/*
 * Check for print error and error code
 */
static inline bool yices_pp_print_failed(yices_pp_t *printer) {
  return printer->pp.printer.print_failed;
}

static inline int yices_pp_errno(yices_pp_t *printer) {
  return printer->pp.printer.pp_errno;
}

static inline void yices_pp_clear_error(yices_pp_t *printer) {
  printer->pp.printer.print_failed = false;
  printer->pp.printer.pp_errno = 0;
}


/*
 * PRINT ATOMS
 */

/*
 * - pp_id(printer, prefix, id): prints <prefix><id>
 *   (example, pp_id(printer, "tau_", 23) prints "tau_23")
 * - pp_varid(printer, prefix, id): prints <prefix>!<id>
 * - for pp_bv64 and pp_bv, n is the number of bits (n must be positive)
 *
 * Function pp_string does not make a copy of the string s so s must
 * remain valid until the printer is deleted. Same thing for prefix
 * function in pp_id. Function pp_bv does not make a copy
 * of the word array *bv either.
 */
extern void pp_char(yices_pp_t *printer, char c);
extern void pp_string(yices_pp_t *printer, const char *s);
extern void pp_id(yices_pp_t *printer, const char *prefix, int32_t id);
extern void pp_varid(yices_pp_t *printer, const char *prefix, int32_t id);
extern void pp_bool(yices_pp_t *printer, bool tt);
extern void pp_int32(yices_pp_t *printer, int32_t x);
extern void pp_uint32(yices_pp_t *printer, uint32_t x);
extern void pp_mpz(yices_pp_t *printer, mpz_t z);
extern void pp_mpq(yices_pp_t *printer, mpq_t q);
extern void pp_rational(yices_pp_t *printer, rational_t *q);
extern void pp_bv64(yices_pp_t *printer, uint64_t bv, uint32_t n);
extern void pp_bv(yices_pp_t *printer, uint32_t *bv, uint32_t n);



/*
 * OPEN AND CLOSE BLOCK
 */

/*
 * Start an block given the open-block id
 */
extern void pp_open_block(yices_pp_t *printer, pp_open_type_t id);

/*
 * Close a block
 * - par: true if a parenthesis is required
 *        false to close and print nothing
 */
extern void pp_close_block(yices_pp_t *printer, bool par);


#endif /* __YICES_PP_H */