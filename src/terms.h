/*
 * Internal term representation
 * ----------------------------
 *
 * This module provides low-level functions for term construction
 * and management of a global term table.
 *
 * Changes
 * -------
 *
 * Feb. 20, 2007.  Added explicit variables for dealing with
 * quantifiers, rather than DeBruijn indices. DeBruijn indices
 * do not mix well with hash consing because different occurrences
 * of the same variable may have different indices.
 * Removed free_vars field since we can't represent free variables
 * via a bit vector anymore.
 *
 * March 07, 2007. Removed bvconstant as a separate representation.
 * They can be stored as bdd arrays. That's simpler and does not cause
 * much overhead.
 *
 * March 24, 2007. Removed mandatory names for uninterpreted constants.
 * Replaced by a function that creates a new uninterpreted constant (with
 * no name) of a given type. Also removed built-in names for the boolean
 * constants.
 *
 * April 20, 2007. Put back a separate representation for bvconstants.
 *
 * June 6, 2007. Added distinct as a built-in term kind.
 *
 * June 12, 2007. Added the bv_apply constructor to support bit-vector operations
 * that are overloaded but that we want to treat as uninterpreted terms (mostly).
 * This is a hack to support the overloaded operators from SMT-LIB 2007 (e.g., bvdiv,
 * bvlshr, etc.)
 *
 * December 11, 2008. Added arith_bineq constructor.
 *
 * January 27, 2009. Removed BDD representation of bvlogic_expressions
 *
 *
 * MAJOR REVISION: April 2010
 *
 * 1) Removed the arithmetic and bitvector variables in polynomials
 *    To replace them, we represent power-products directly as
 *    terms in the term table.
 *
 * 2) Removed the AIG-style data structures for bitvectors (these
 *    are replaced by arrays of boolean terms). Added an n-ary
 *    (xor ...) term constructor to help representing bv-xor.
 *
 * 3) Removed the term constructor 'not' for boolean negation.
 *    Used positive/negative polarity bits to replace that:
 *    For a boolean term t, we use a one bit tag to denote t+ and t-,
 *    where t- means (not t).
 *
 * 5) Added terms for converting between boolean and bitvector terms:
 *    Given a term u of type (bitvector n) then (bit u i) is
 *    a boolean term for i=0 to n-1. (bit u 0) is the low-order bit.
 *    Conversely, given n boolean terms b_0 ... b_{n-1}, the term
 *    (bitarray b0 ... b_{n-1}) is the bitvector formed by b0 ... b_{n-1}.
 *
 * 6) Added support for unit types: a unit type tau is a type of cardinality
 *    one. In that case, all terms of type tau are equal so we build a
 *    single representative term for type tau.
 *
 * 7) General cleanup to make things more consistent: use a generic
 *    structure to represent most composite terms.
 *
 * July 2012: Added lambda terms.
 *
 * October 2013: NEW BRANCH FOR BITVECTORS AND BOOLEANS ONLY
 * - removed arithmetic, tuples, and update constructors, lambda
 * - removed unit types stuff (no type is unit)
 */

/*
 * The internal terms include:
 * 1) constants:
 *    - constants of uninterpreted/scalar types
 *    - global uninterpreted constants
 * 2) generic terms
 *    - ite c t1 t2
 *    - eq t1 t2
 *    - apply f t1 ... t_n
 *    - distinct t1 ... t_n
 * 3) variables and quantifiers
 *    - variables are identified by their type and an integer index.
 *    - quantified formulas are of the form (forall v_1 ... v_n term)
 *      where each v_i is a variable
 *    - lambda terms are of the form (lambda v_1 ... v_n term) where
 *      each v_i is a variable
 * 4) boolean operators
 *    - or t1 ... t_n
 *    - xor t1 ... t_n
 *    - bit i u (extract bit i of a bitvector term u)
 * 4) bitvector terms and atoms
 *    - bitvector constants
 *    - power products
 *    - polynomials
 *    - bit arrays
 *    - other operations: divisions/shift
 *    - atoms: three binary predicates
 *      bv_eq t1 t2
 *      bv_ge t1 t2 (unsigned comparison: t1 >= t2)
 *      bv_sge t1 t2 (signed comparison: t1 >= t2)
 *
 * Every term is an index t in a global term table,
 * where 0 <= t <= 2^30. There are two term occurrences
 * t+ and t- associated with term t.  The two occurrences
 * are encoded in signed 32bit integers:
 * - bit[31] = sign bit = 0
 * - bits[30 ... 1] = t
 * - bit[0] = polarity bit: 0 for t+, 1 for t-
 *
 * For a boolean term t, the occurrence t+ means p
 * and t- means (not p). All occurrences of a
 * non-boolean term t are positive.
 *
 * For every term, we keep:
 * - type[t] (index in the type table)
 * - kind[t] (what kind of term it is)
 * - desc[t] = descriptor that depends on the kind
 *
 * It is possible to attach names to term occurrences (but not directly
 * to terms). This is required to deal properly with booleans. For example,
 * we want to allow the user to give different names to t and (not t).
 */

#ifndef __TERMS_H
#define __TERMS_H

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "bitvectors.h"
#include "int_vectors.h"
#include "ptr_vectors.h"
#include "int_hash_tables.h"
#include "ptr_hash_map.h"
#include "symbol_tables.h"

#include "types.h"
#include "pprod_table.h"
#include "bvarith_buffers.h"
#include "bvarith64_buffers.h"


/*
 * TERM INDICES
 */

/*
 * type_t and term_t are aliases for int32_t, defined in yices_types.h.
 *
 * We use the type term_t to denote term occurrences (i.e., a pair
 * term index + polarity bit packed into a signed 32bit integer as
 * defined in term_occurrences.h).
 *
 * NULL_TERM and NULL_TYPE are also defined in yices_types.h (used to
 * report errors).
 *
 * Limits defined in yices_limits.h are relevant here:
 *
 * 1) YICES_MAX_TERMS = bound on the number of terms
 * 2) YICES_MAX_TYPES = bound on the number of types
 * 3) YICES_MAX_ARITY = bound on the term arity
 * 4) YICES_MAX_VARS  = bound on n in (FORALL (x_1.... x_n) P)
 * 5) YICES_MAX_DEGREE = bound on the total degree of polynomials and power-products
 * 6) YICES_MAX_BVSIZE = bound on the size of bitvector
 *
 */
#include "yices_types.h"
#include "yices_limits.h"



/*
 * TERM KINDS
 */
/*
 * The enumeration order is significant. We can cheaply check whether
 * a term is constant, variable or composite.
 */
typedef enum {
  /*
   * Special marks
   */
  UNUSED_TERM,    // deleted term
  RESERVED_TERM,  // mark for term indices that can't be used

  /*
   * Constants
   */
  CONSTANT_TERM,    // constant of uninterpreted/scalar/boolean types
  BV64_CONSTANT,    // compact bitvector constant (64 bits at most)
  BV_CONSTANT,      // generic bitvector constant (more than 64 bits)

  /*
   * Non-constant, atomic terms
   */
  UNINTERPRETED_TERM,  // (i.e., global variables, can't be bound).

  /*
   * Composites
   */
  ITE_TERM,         // if-then-else
  EQ_TERM,          // equality
  DISTINCT_TERM,    // distinct t_1 ... t_n
  OR_TERM,          // n-ary OR
  XOR_TERM,         // n-ary XOR

  BV_ARRAY,         // array of boolean terms
  BV_DIV,           // unsigned division
  BV_REM,           // unsigned remainder
  BV_SDIV,          // signed division
  BV_SREM,          // remainder in signed division (rounding to 0)
  BV_SMOD,          // remainder in signed division (rounding to -infinity)
  BV_SHL,           // shift left (padding with 0)
  BV_LSHR,          // logical shift right (padding with 0)
  BV_ASHR,          // arithmetic shift right (padding with sign bit)
  BV_EQ_ATOM,       // equality: (t1 == t2)
  BV_GE_ATOM,       // unsigned comparison: (t1 >= t2)
  BV_SGE_ATOM,      // signed comparison (t1 >= t2)

  BIT_TERM,         // bit-select

  // Polynomials
  POWER_PRODUCT,    // power products: (t1^d1 * ... * t_n^d_n)
  BV64_POLY,        // polynomial with 64bit coefficients
  BV_POLY,          // polynomial with generic bitvector coefficients
} term_kind_t;

#define NUM_TERM_KINDS (BV_POLY+1)



/*
 * PREDEFINED TERMS
 */

/*
 * Term index 0 is reserved to make sure there's no possibility
 * that a real term has index equal to const_idx (= 0) used in
 * polynomials.
 *
 * The boolean constant true is built-in and always has index 1.
 * This gives two terms:
 * - true_term = pos_occ(bool_const) = 2
 * - false_term = neg_occ(bool_const) = 3
 */
enum {
  // indices
  bool_const = 1,
  // terms
  true_term = 2,
  false_term = 3,
};


/*
 * TERM DESCRIPTORS
 */

/*
 * Composite: array of n terms
 */
typedef struct composite_term_s {
  uint32_t arity;  // number of subterms
  term_t arg[0];  // real size = arity
} composite_term_t;


/*
 * Tuple projection and bit-extraction:
 * - an integer index + a term occurrence
 */
typedef struct select_term_s {
  uint32_t idx;
  term_t arg;
} select_term_t;


/*
 * Bitvector constants of arbitrary size:
 * - bitsize = number of bits
 * - data = array of 32bit words (of size equal to ceil(nbits/32))
 */
typedef struct bvconst_term_s {
  uint32_t bitsize;
  uint32_t data[0];
} bvconst_term_t;


/*
 * Bitvector constants of no more than 64bits
 */
typedef struct bvconst64_term_s {
  uint32_t bitsize; // between 1 and 64
  uint64_t value;   // normalized value: high-order bits are 0
} bvconst64_term_t;


/*
 * Descriptor: one of
 * - integer index for constant terms and variables
 * - pair  (idx, arg) for select term
 * - ptr to a composite, polynomial, power-product, or bvconst
 */
typedef union {
  int32_t integer;
  void *ptr;
  select_term_t select;
} term_desc_t;



/*
 * Term table: valid terms have indices between 0 and nelems - 1
 *
 * For each i between 0 and nelems - 1
 * - kind[i] = term kind
 * - type[i] = type
 * - desc[i] = term descriptor
 * - mark[i] = one bit used during garbage collection
 * - size = size of these arrays.
 *
 * After deletion, term indices are recycled into a free list.
 * - free_idx = start of the free list (-1 if the list is empty)
 * - if i is in the free list then kind[i] is UNUSED and
 *   desc[i].integer is the index of the next term in the free list
 *   (or -1 if i is the last element in the free list).
 *
 * - live_terms = number of actual terms = nelems - size of the free list
 *
 * Symbol table and name table:
 * - stbl is a symbol table that maps names (strings) to term occurrences.
 * - the name table is the reverse. If maps term occurrence to a name.
 * The base name of a term occurrence t, is what's mapped to t in ntbl.
 * It's used to display t in pretty printing. The symbol table is
 * more important.
 *
 * Other components:
 * - types = pointer to an associated type table
 * - pprods = pointer to an associated power product table
 * - htbl = hash table for hash consing
 *
 * Auxiliary vectors
 * - ibuffer: to store an array of integers
 * - pbuffer: to store an array of pprods
 */
typedef struct term_table_s {
  uint8_t *kind;
  term_desc_t *desc;
  type_t *type;
  byte_t *mark;

  uint32_t size;
  uint32_t nelems;
  int32_t free_idx;
  uint32_t live_terms;

  type_table_t *types;
  pprod_table_t *pprods;

  int_htbl_t htbl;
  stbl_t stbl;
  ptr_hmap_t ntbl;

  ivector_t ibuffer;
  pvector_t pbuffer;
} term_table_t;




/*
 * INITIALIZATION
 */

/*
 * Initialize table:
 * - n = initial size
 * - ttbl = attached type table
 * - ptbl = attached power-product table
 */
extern void init_term_table(term_table_t *table, uint32_t n, type_table_t *ttbl, pprod_table_t *ptbl);


/*
 * Delete all terms and descriptors, symbol table, hash table, etc.
 */
extern void delete_term_table(term_table_t *table);



/*
 * TERM CONSTRUCTORS
 */

/*
 * All term constructors return a term occurrence and all the arguments
 * the constructors must be term occurrences (term index + polarity
 * bit). The constructors do not check type correctness or attempt any
 * simplification. They just do hash consing.
 */

/*
 * Constant of the given type and index.
 * - tau must be uninterpreted or scalar
 * - if tau is scalar of cardinality n, then index must be between 0 and n-1
 */
extern term_t constant_term(term_table_t *table, type_t tau, int32_t index);


/*
 * Declare a new uninterpreted constant of type tau.
 * - this always creates a fresh term
 */
extern term_t new_uninterpreted_term(term_table_t *table, type_t tau);


/*
 * Negation: just flip the polarity bit
 * - p must be boolean
 */
extern term_t not_term(term_table_t *table, term_t p);


/*
 * If-then-else term (if cond then left else right)
 * - tau must be the super type of left/right.
 */
extern term_t ite_term(term_table_t *table, type_t tau, term_t cond, term_t left, term_t right);


/*
 * Other constructors compute the result type
 * - for variable-arity constructor:
 *   arg must be an array of n term occurrences
 *   and n must be no more than YICES_MAX_ARITY.
 */
extern term_t eq_term(term_table_t *table, term_t left, term_t right);
extern term_t distinct_term(term_table_t *table, uint32_t n, term_t arg[]);
extern term_t or_term(term_table_t *table, uint32_t n, term_t arg[]);
extern term_t xor_term(term_table_t *table, uint32_t n, term_t arg[]);
extern term_t bit_term(term_table_t *table, uint32_t index, term_t bv);



/*
 * POWER PRODUCT
 */

/*
 * Power product: r must be valid in table->ptbl, and must not be a tagged
 * variable or empty_pp.
 * - each variable index x_i in r must be a term defined in table
 * - the x_i's must have compatible types: they must all be bit-vector
 *   terms of the  same type.
 * The type of the result is determined from the x_i's type:
 * - if all x_i's have type (bitvector k), the result has type (bitvector k)
 */
extern term_t pprod_term(term_table_t *table, pprod_t *r);


/*
 * BITVECTOR TERMS
 */

/*
 * Small bitvector constant
 * - n = bitsize (must be between 1 and 64)
 * - bv = value (must be normalized modulo 2^n)
 */
extern term_t bv64_constant(term_table_t *table, uint32_t n, uint64_t bv);

/*
 * Bitvector constant:
 * - n = bitsize
 * - bv = array of k words (where k = ceil(n/32))
 * The constant must be normalized (modulo 2^n)
 * This constructor should be used only for n > 64.
 */
extern term_t bvconst_term(term_table_t *table, uint32_t n, uint32_t *bv);


/*
 * Bitvector polynomials are constructed from a buffer b
 * - all variables of b must be bitvector terms defined in table
 * - b must be normalized and b->ptbl must be the same as table->ptbl
 * - if b contains non-linear terms, then the power products that
 *   occur in b are converted to terms (using pprod_term) then
 *   a polynomial object is created.
 *
 * SIDE EFFECT: b is reset to zero.
 */
extern term_t bv64_poly(term_table_t *table, bvarith64_buffer_t *b);
extern term_t bv_poly(term_table_t *table, bvarith_buffer_t *b);


/*
 * Bitvector formed of arg[0] ... arg[n-1]
 * - n must be positive and no more than YICES_MAX_BVSIZE
 * - arg[0] ... arg[n-1] must be boolean terms
 */
extern term_t bvarray_term(term_table_t *table, uint32_t n, term_t arg[]);


/*
 * Division and shift operators
 * - the two arguments must be bitvector terms of the same type
 * - in the division/remainder operators, b is the divisor
 * - in the shift operator: a is the bitvector to be shifted
 *   and b is the shift amount
 */
extern term_t bvdiv_term(term_table_t *table, term_t a, term_t b);
extern term_t bvrem_term(term_table_t *table, term_t a, term_t b);
extern term_t bvsdiv_term(term_table_t *table, term_t a, term_t b);
extern term_t bvsrem_term(term_table_t *table, term_t a, term_t b);
extern term_t bvsmod_term(term_table_t *table, term_t a, term_t b);

extern term_t bvshl_term(term_table_t *table, term_t a, term_t b);
extern term_t bvlshr_term(term_table_t *table, term_t a, term_t b);
extern term_t bvashr_term(term_table_t *table, term_t a, term_t b);


/*
 * Bitvector atoms: l and r must be bitvector terms of the same type
 *  (bveq l r): l == r
 *  (bvge l r): l >= r unsigned
 *  (bvsge l r): l >= r signed
 */
extern term_t bveq_atom(term_table_t *table, term_t l, term_t r);
extern term_t bvge_atom(term_table_t *table, term_t l, term_t r);
extern term_t bvsge_atom(term_table_t *table, term_t l, term_t r);





/*
 * NAMES
 */

/*
 * IMPORTANT: we use reference counting on character strings as
 * implemented in refcount_strings.h.
 *
 * Parameter "name" in set_term_name and set_term_basename
 * must be constructed via the clone_string function.
 * That's not necessary for get_term_by_name or remove_term_name.
 * When name is added to the term table, its reference counter
 * is increased by 1 or 2.  When remove_term_name is
 * called for an existing symbol, the symbol's reference counter is
 * decremented.  When the table is deleted (via delete_term_table),
 * the reference counters of all symbols present in table are also
 * decremented.
 */

/*
 * Assign name to term occurrence t.
 *
 * If name is already mapped to another term t' then the previous mapping
 * is hidden. The next calls to get_term_by_name will return t. After a
 * call to remove_term_name, the mapping [name --> t] is removed and
 * the previous mapping [name --> t'] is revealed.
 *
 * If t does not have a base name already, then 'name' is stored as the
 * base name for t. That's what's printed for t by the pretty printer.
 *
 * Warning: name is stored as a pointer, no copy is made; name must be
 * created via the clone_string function.
 */
extern void set_term_name(term_table_t *table, term_t t, char *name);


/*
 * Assign name as the base name for term t
 * - if t already has a base name, then it's replaced by 'name'
 *   and the previous name's reference counter is decremented
 */
extern void set_term_base_name(term_table_t *table, term_t t, char *name);


/*
 * Get term occurrence with the given name (or NULL_TERM)
 */
extern term_t get_term_by_name(term_table_t *table, const char *name);


/*
 * Remove a name from the symbol table
 * - if name is not in the symbol table, nothing is done
 * - if name is mapped to a term t, then the mapping [name -> t]
 *   is removed. If name was mapped to a previous term t' then
 *   that mapping is restored.
 *
 * If name is the base name of a term t, then that remains unchanged.
 */
extern void remove_term_name(term_table_t *table, const char *name);


/*
 * Get the base name of term occurrence t
 * - return NULL if t has no base name
 */
extern char *term_name(term_table_t *table, term_t t);


/*
 * Clear name: remove t's base name if any.
 * - If t has name 'xxx' then 'xxx' is first removed from the symbol
 *   table (using remove_term_name) then t's base name is erased.
 *   The reference counter for 'xxx' is decremented twice.
 * - If t doesn't have a base name, nothing is done.
 */
extern void clear_term_name(term_table_t *table, term_t t);





/*
 * TERM INDICES/POLARITY
 */

/*
 * Conversion between indices and terms
 * - the polarity bit is the low-order bit of
 *   0 means positive polarity
 *   1 means negative polarity
 */
static inline term_t pos_term(int32_t i) {
  return (i << 1);
}

static inline term_t neg_term(int32_t i) {
  return (i << 1) | 1;
}


/*
 * Term of index i and polarity tt
 * - true means positive polarity
 * - false means negative polarity
 */
static inline term_t mk_term(int32_t i, bool tt) {
  return (i << 1) | (((int32_t) tt) ^ 1);
}


/*
 * Extract term and polarity bit
 */
static inline int32_t index_of(term_t x) {
  return x>>1;
}

static inline uint32_t polarity_of(term_t x) {
  return ((uint32_t) x) & 1;
}


/*
 * Check polarity
 */
static inline bool is_pos_term(term_t x) {
  return polarity_of(x) == 0;
}

static inline bool is_neg_term(term_t x) {
  return polarity_of(x) != 0;
}


/*
 * Complement of x = same term, opposite polarity
 * - this can be used instead of not_term(table, x)
 *   if x is known to be a valid boolean term.
 */
static inline term_t opposite_term(term_t x) {
  return x ^ 1;
}


/*
 * Remove the sign of x:
 * - if x has positive polarity: return x
 * - if x has negative polarity: return (not x)
 */
static inline term_t unsigned_term(term_t x) {
  return x & ~1; // clear polarity bit
}


/*
 * Add polarity tt to term x:
 * - if tt is true: return x
 * - if tt is false: return (not x)
 */
static inline term_t signed_term(term_t x, bool tt) {
  return x ^ (((int32_t) tt) ^ 1);
}


/*
 * Check whether x and y are opposite terms
 */
static inline bool opposite_bool_terms(term_t x, term_t y) {
  return (x ^ y) == 1;
}


/*
 * Conversion of boolean to true_term or false_term
 */
static inline term_t bool2term(bool tt) {
  return mk_term(bool_const, tt);
}



/*
 * SUPPORT FOR POLYNOMIAL/BUFFER OPERATIONS
 */

/*
 * The term table store polynomials in the form
 *      a_0 t_0 + ... + a_n t_n
 * where a_i is a coefficient and t_i is a term.
 *
 * For operations that involve buffers and terms, we must convert the
 * integer indices t_0 ... t_n to the power products r_0 ... r_n that
 * buffers require.
 *
 * The translation is defined by:
 * 1) if t_i is const_idx --> r_i is empty_pp
 * 2) if t_i is a power product --> r_i = descriptor for t_i
 * 3) otherwise --> r_i = var_pp(t_i)
 */

/*
 * Convert term t to a power product:
 * - t must be a term (not a term index) present in the table
 * - t must have arithmetic or bitvector type
 */
extern pprod_t *pprod_for_term(term_table_t *table, term_t t);


/*
 * Degree of term t
 * - t must be a good term of arithmetic or bitvector type
 *
 * - if t is a constant --> 0
 * - if t is a power product --> that product degree
 * - if t is a polynomial --> degree of that polynomial
 * - otherwise --> 1
 */
extern uint32_t term_degree(term_table_t *table, term_t t);


/*
 * Convert all indices in bitvector polynomial p to power products
 * - all variable indices of p must be either const_idx or
 *   bitvector terms of bitsize <= 64 present in table.
 * - the result is stored in table's internal pbuffer.
 * - the function returns pbuffer->data.
 *
 * The number of elements in pbuffer is equal to p->nterms + 1.
 * - pbuffer->data[i] = pprod_for_term(table, p->mono[i].var)
 * - the last element of buffer->data is the end marker end_pp.
 */
extern pprod_t **pprods_for_bvpoly64(term_table_t *table, bvpoly64_t *p);


/*
 * Convert all indices in bitvector polynomial p to power products
 * - all variable indices of p must be either const_idx or
 *   arithmetic terms present in table.
 * - the result is stored in table's internal pbuffer.
 * - the function returns pbuffer->data.
 */
extern pprod_t **pprods_for_bvpoly(term_table_t *table, bvpoly_t *p);


/*
 * Reset the internal pbuffer
 */
static inline void term_table_reset_pbuffer(term_table_t *table) {
  pvector_reset(&table->pbuffer);
}








/*
 * ACCESS TO TERMS
 */

/*
 * From a term index i
 */
static inline bool valid_term_idx(term_table_t *table, int32_t i) {
  return 0 <= i && i < table->nelems;
}

static inline bool live_term_idx(term_table_t *table, int32_t i) {
  return valid_term_idx(table, i) && table->kind[i] != UNUSED_TERM;
}

static inline bool good_term_idx(term_table_t *table, int32_t i) {
  return valid_term_idx(table, i) && table->kind[i] > RESERVED_TERM;
}

static inline type_t type_for_idx(term_table_t *table, int32_t i) {
  assert(good_term_idx(table, i));
  return table->type[i];
}

static inline term_kind_t kind_for_idx(term_table_t *table, int32_t i) {
  assert(good_term_idx(table, i));
  return table->kind[i];
}

// descriptor converted to an appropriate type
static inline int32_t integer_value_for_idx(term_table_t *table, int32_t i) {
  assert(good_term_idx(table, i));
  return table->desc[i].integer;
}

static inline composite_term_t *composite_for_idx(term_table_t *table, int32_t i) {
  assert(good_term_idx(table, i));
  return (composite_term_t *) table->desc[i].ptr;
}

static inline select_term_t *select_for_idx(term_table_t *table, int32_t i) {
  assert(good_term_idx(table, i));
  return &table->desc[i].select;
}

static inline pprod_t *pprod_for_idx(term_table_t *table, int32_t i) {
  assert(good_term_idx(table, i));
  return (pprod_t *) table->desc[i].ptr;
}

static inline bvconst64_term_t *bvconst64_for_idx(term_table_t *table, int32_t i) {
  assert(good_term_idx(table, i));
  return (bvconst64_term_t *) table->desc[i].ptr;
}

static inline bvconst_term_t *bvconst_for_idx(term_table_t *table, int32_t i) {
  assert(good_term_idx(table, i));
  return (bvconst_term_t *) table->desc[i].ptr;
}

static inline bvpoly64_t *bvpoly64_for_idx(term_table_t *table, int32_t i) {
  assert(good_term_idx(table, i));
  return (bvpoly64_t *) table->desc[i].ptr;
}

static inline bvpoly_t *bvpoly_for_idx(term_table_t *table, int32_t i) {
  assert(good_term_idx(table, i));
  return (bvpoly_t *) table->desc[i].ptr;
}


// bitsize of bitvector terms
static inline uint32_t bitsize_for_idx(term_table_t *table, int32_t i) {
  assert(good_term_idx(table, i));
  return bv_type_size(table->types, table->type[i]);
}



/*
 * Access components using term occurrence t
 */
static inline bool live_term(term_table_t *table, term_t t) {
  return live_term_idx(table, index_of(t));
}

// good_term means good_term_index
// and polarity = 0 (unless t is Boolean)
extern bool good_term(term_table_t *table, term_t t);

static inline bool bad_term(term_table_t *table, term_t t) {
  return ! good_term(table, t);
}

static inline term_kind_t term_kind(term_table_t *table, term_t t) {
  return kind_for_idx(table, index_of(t));
}

static inline type_t term_type(term_table_t *table, term_t t) {
  return type_for_idx(table, index_of(t));
}

static inline type_kind_t term_type_kind(term_table_t *table, term_t t) {
  return type_kind(table->types, term_type(table, t));
}


// Checks on the type of t
static inline bool is_boolean_term(term_table_t *table, term_t t) {
  return is_boolean_type(term_type(table, t));
}

static inline bool is_bitvector_term(term_table_t *table, term_t t) {
  return term_type_kind(table, t) == BITVECTOR_TYPE;
}

// Bitsize of term t
static inline uint32_t term_bitsize(term_table_t *table, term_t t) {
  return bitsize_for_idx(table, index_of(t));
}


// Check whether t is if-then-else
static inline bool is_ite_kind(term_kind_t tag) {
  return tag == ITE_TERM;
}

static inline bool is_ite_term(term_table_t *table, term_t t) {
  return term_kind(table, t) == ITE_TERM;
}


// Check whether t is atomic
static inline bool is_atomic_kind(term_kind_t tag) {
  return CONSTANT_TERM <= tag && tag <= UNINTERPRETED_TERM;
}

static inline bool is_atomic_term(term_table_t *table, term_t t) {
  return is_atomic_kind(term_kind(table, t));
}


// Check whether t is atomic and constant
static inline bool is_const_kind(term_kind_t tag) {
  return CONSTANT_TERM <= tag && tag <= BV_CONSTANT;
}

static inline bool is_const_term(term_table_t *table, term_t t) {
  return is_const_kind(term_kind(table, t));
}


/*
 * CONSTANT TERMS
 */

/*
 * Check whether t is a constant tuple
 * - t must be a tuple term
 */
extern bool is_constant_tuple(term_table_t *table, term_t t);


/*
 * Generic version: return true if t is an atomic constant
 * or a constant tuple.
 */
extern bool is_constant_term(term_table_t *table, term_t t);


/*
 * Check whether the table contains a constant term of type tau and the given index
 * - tau must be uninterpreted or scalar
 * - if tau is scalar, then index must be between 0 and cardinality of tau - 1
 * - return NULL_TERM if there's no such term in table
 */
extern term_t find_constant_term(term_table_t *table, type_t tau, int32_t index);




/*
 * Descriptor of term t.
 *
 * NOTE: bit_term_desc should be used with care. It returns a pointer
 * that may become invalid if new terms are added to the table.
 */
static inline int32_t constant_term_index(term_table_t *table, term_t t) {
  assert(term_kind(table, t) == CONSTANT_TERM);
  return integer_value_for_idx(table, index_of(t));
}

static inline composite_term_t *composite_term_desc(term_table_t *table, term_t t) {
  return composite_for_idx(table, index_of(t));
}

static inline select_term_t *bit_term_desc(term_table_t *table, term_t t) {
  assert(term_kind(table, t) == BIT_TERM);
  return select_for_idx(table, index_of(t));
}

static inline pprod_t *pprod_term_desc(term_table_t *table, term_t t) {
  assert(term_kind(table, t) == POWER_PRODUCT);
  return pprod_for_idx(table, index_of(t));
}

static inline bvconst64_term_t *bvconst64_term_desc(term_table_t *table, term_t t) {
  assert(term_kind(table, t) == BV64_CONSTANT);
  return bvconst64_for_idx(table, index_of(t));
}

static inline bvconst_term_t *bvconst_term_desc(term_table_t *table, term_t t) {
  assert(term_kind(table, t) == BV_CONSTANT);
  return bvconst_for_idx(table, index_of(t));
}

static inline bvpoly64_t *bvpoly64_term_desc(term_table_t *table, term_t t) {
  assert(term_kind(table, t) == BV64_POLY);
  return bvpoly64_for_idx(table, index_of(t));
}

static inline bvpoly_t *bvpoly_term_desc(term_table_t *table, term_t t) {
  assert(term_kind(table, t) == BV_POLY);
  return bvpoly_for_idx(table, index_of(t));
}


/*
 * Subcomponents of a term t
 */
// arity of a composite term
static inline uint32_t composite_term_arity(term_table_t *table, term_t t) {
  return composite_term_desc(table, t)->arity;
}

// i-th argument of term t (t must be a composite term)
static inline term_t composite_term_arg(term_table_t *table, term_t t, uint32_t i) {
  assert(i < composite_term_arity(table, t));
  return composite_term_desc(table, t)->arg[i];
}

// index of a bit select term t
static inline uint32_t bit_term_index(term_table_t *table, term_t t) {
  return bit_term_desc(table, t)->idx;
}

// argument of select term t
static inline term_t bit_term_arg(term_table_t *table, term_t t) {
  return bit_term_desc(table, t)->arg;
}

/*
 * All the following functions are equivalent to composite_term_desc, but,
 * when debugging is enabled, they also check that the term kind is consistent.
 */
static inline composite_term_t *ite_term_desc(term_table_t *table, term_t t) {
  assert(is_ite_kind(term_kind(table, t)));
  return composite_for_idx(table, index_of(t));
}

static inline composite_term_t *eq_term_desc(term_table_t *table, term_t t) {
  assert(term_kind(table, t) == EQ_TERM);
  return composite_for_idx(table, index_of(t));
}

static inline composite_term_t *distinct_term_desc(term_table_t *table, term_t t) {
  assert(term_kind(table, t) == DISTINCT_TERM);
  return composite_for_idx(table, index_of(t));
}

static inline composite_term_t *or_term_desc(term_table_t *table, term_t t) {
  assert(term_kind(table, t) == OR_TERM);
  return composite_for_idx(table, index_of(t));
}

static inline composite_term_t *xor_term_desc(term_table_t *table, term_t t) {
  assert(term_kind(table, t) == XOR_TERM);
  return composite_for_idx(table, index_of(t));
}

static inline composite_term_t *bvarray_term_desc(term_table_t *table, term_t t) {
  assert(term_kind(table, t) == BV_ARRAY);
  return composite_for_idx(table, index_of(t));
}

static inline composite_term_t *bvdiv_term_desc(term_table_t *table, term_t t) {
  assert(term_kind(table, t) == BV_DIV);
  return composite_for_idx(table, index_of(t));
}

static inline composite_term_t *bvrem_term_desc(term_table_t *table, term_t t) {
  assert(term_kind(table, t) == BV_REM);
  return composite_for_idx(table, index_of(t));
}

static inline composite_term_t *bvsdiv_term_desc(term_table_t *table, term_t t) {
  assert(term_kind(table, t) == BV_SDIV);
  return composite_for_idx(table, index_of(t));
}

static inline composite_term_t *bvsrem_term_desc(term_table_t *table, term_t t) {
  assert(term_kind(table, t) == BV_SREM);
  return composite_for_idx(table, index_of(t));
}

static inline composite_term_t *bvsmod_term_desc(term_table_t *table, term_t t) {
  assert(term_kind(table, t) == BV_SMOD);
  return composite_for_idx(table, index_of(t));
}

static inline composite_term_t *bvshl_term_desc(term_table_t *table, term_t t) {
  assert(term_kind(table, t) == BV_SHL);
  return composite_for_idx(table, index_of(t));
}

static inline composite_term_t *bvlshr_term_desc(term_table_t *table, term_t t) {
  assert(term_kind(table, t) == BV_LSHR);
  return composite_for_idx(table, index_of(t));
}

static inline composite_term_t *bvashr_term_desc(term_table_t *table, term_t t) {
  assert(term_kind(table, t) == BV_ASHR);
  return composite_for_idx(table, index_of(t));
}

static inline composite_term_t *bveq_atom_desc(term_table_t *table, term_t t) {
  assert(term_kind(table, t) == BV_EQ_ATOM);
  return composite_for_idx(table, index_of(t));
}

static inline composite_term_t *bvge_atom_desc(term_table_t *table, term_t t) {
  assert(term_kind(table, t) == BV_GE_ATOM);
  return composite_for_idx(table, index_of(t));
}

static inline composite_term_t *bvsge_atom_desc(term_table_t *table, term_t t) {
  assert(term_kind(table, t) == BV_SGE_ATOM);
  return composite_for_idx(table, index_of(t));
}




/*
 * GARBAGE COLLECTION
 */

/*
 * Mark and sweep mechanism:
 * - nothing gets deleted until an explicit call to term_table_gc
 * - before calling the garbage collector, the root terms must be
 *   marked by calling set_gc_mark.
 * - all the terms that can be accessed by a name (i.e., all the terms
 *   that are present in the symbol table) are also considered root terms.
 *
 * Garbage collection process:
 * - The predefined terms (bool_const, zero_const, const_idx ) are marked
 * - If keep_named is true, all terms accessible from the symbol table are marked too
 * - The marks are propagated to subterms, types, and power products.
 * - Every term that's not marked is deleted.
 * - If keep_named is false, all references to dead terms are removed from the
 *   symbol table.
 * - The type and power-product tables' own garbage collectors are called.
 * - Finally all the marks are cleared.
 */

/*
 * Set or clear the mark on a term i. If i is marked, it is preserved
 * on the next call to the garbage collector (and all terms reachable
 * from i are preserved too).  If the mark is cleared, i may be deleted.
 */
static inline void term_table_set_gc_mark(term_table_t *table, int32_t i) {
  assert(good_term_idx(table, i));
  set_bit(table->mark, i);
}

static inline void term_table_clr_gc_mark(term_table_t *table, int32_t i) {
  assert(valid_term_idx(table, i));
  clr_bit(table->mark, i);
}


/*
 * Test whether i is marked
 */
static inline bool term_idx_is_marked(term_table_t *table, int32_t i) {
  assert(valid_term_idx(table, i));
  return tst_bit(table->mark, i);
}


/*
 * Call the garbage collector:
 * - every term reachable from a marked term is preserved.
 * - recursively calls type_table_gc and pprod_table_gc
 * - if keep_named is true, all named terms (reachable from the symbol table)
 *   are preserved. Otherwise,  all references to dead terms are removed
 *   from the symbol table.
 * - then all the marks are cleared.
 *
 * NOTE: type_table_gc is called with the same keep_named flag.
 */
extern void term_table_gc(term_table_t *table, bool keep_named);


#endif /* __TERMS_H */