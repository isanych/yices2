/*
 * CONTEXT
 *
 * Updated to work with the new term representation.
 *
 * Basic context: enough support for simplification
 * and flattening of assertion.
 */

#ifndef __CONTEXT_H
#define __CONTEXT_H

#include <stdint.h>
#include <stdbool.h>
#include <setjmp.h>

#include "yices_types.h"

#include "int_queues.h"
#include "int_stack.h"
#include "int_vectors.h"
#include "int_hash_sets.h"
#include "int_bv_sets.h"
#include "pair_hash_map2.h"
#include "tracer.h"

#include "terms.h"
#include "internalization_table.h"
#include "internalization_codes.h"
#include "pseudo_subst.h"
#include "mark_vectors.h"

#include "search_parameters.h"
#include "gates_manager.h"
#include "smt_core.h"
#include "models.h"
#include "yices_locks.h"



/***********************
 *  OPTIONAL FEATURES  *
 **********************/

/*
 * Bit mask for specifying which features are supported by a context.
 * These are set when the context is created.
 */
#define MULTICHECKS_OPTION_MASK 0x1
#define PUSHPOP_OPTION_MASK     0x2
#define CLEANINT_OPTION_MASK    0x4


/*
 * Possible modes: each mode defines which of the three above
 * bits are set.
 */
typedef enum {
  CTX_MODE_ONECHECK,
  CTX_MODE_MULTICHECKS,
  CTX_MODE_PUSHPOP,
  CTX_MODE_INTERACTIVE,
} context_mode_t;

#define NUM_MODES (CTX_MODE_INTERACTIVE+1)


/*
 * More bit masks for enabling/disabling simplification
 * - VARELIM eliminate variables (via substitution)
 * - FLATTENOR rewrites (or ... (or ...) ...) into a single (or ....)
 * - FLATTENDISEQ rewrite arithmetic disequality
 *      (not (p == 0)) into (or (not (p >= 0)) (not (-p >= 0)))
 *   FLATTENDISEQ requires FLATTENOR to be enabled
 * - EQABSTRACT enables the abstraction-based equality learning heuristic
 * - ARITHELIM enables variable elimination in arithmetic
 * - KEEP_ITE: keep if-then-else terms in the egraph
 * - BREAKSYM: enables symmetry breaking
 * - PSEUDO_INVERSE: enables elimination of unconstrained terms using
 *   pseudo-inverse tricks.
 *
 * BREAKSYM for QF_UF is based on the paper by Deharbe et al (CADE 2011)
 *
 * PSEUDO_INVERSE is based on Brummayer's thesis (Boolector stuff).
 *
 * Options passed to the simplex solver when it's created
 * - EAGER_LEMMAS
 * - ENABLE_ICHECK
 * - EQPROP
 *
 * Options for testing and debugging
 * - LAX_OPTION: try to keep going when the assertions contain unsupported
 *               constructs (e.g., quantifiers/bitvectors).
 * - DUMP_OPTION
 */
#define VARELIM_OPTION_MASK             0x10
#define FLATTENOR_OPTION_MASK           0x20
#define FLATTENDISEQ_OPTION_MASK        0x40
#define EQABSTRACT_OPTION_MASK          0x80
#define ARITHELIM_OPTION_MASK           0x100
#define KEEP_ITE_OPTION_MASK            0x200
#define BVARITHELIM_OPTION_MASK         0x400
#define BREAKSYM_OPTION_MASK            0x800
#define PSEUDO_INVERSE_OPTION_MASK      0x1000


#define PREPROCESSING_OPTIONS_MASK \
 (VARELIM_OPTION_MASK|FLATTENOR_OPTION_MASK|FLATTENDISEQ_OPTION_MASK|\
  EQABSTRACT_OPTION_MASK|ARITHELIM_OPTION_MASK|KEEP_ITE_OPTION_MASK|\
  BVARITHELIM_OPTION_MASK|BREAKSYM_OPTION_MASK|PSEUDO_INVERSE_OPTION_MASK)

// SIMPLEX OPTIONS
#define SPLX_EGRLMAS_OPTION_MASK  0x10000
#define SPLX_ICHECK_OPTION_MASK   0x20000
#define SPLX_EQPROP_OPTION_MASK   0x40000

// FOR TESTING
#define LAX_OPTION_MASK         0x40000000
#define DUMP_OPTION_MASK        0x80000000



/***************************************
 *  ARCHITECTURES/SOLVER COMBINATIONS  *
 **************************************/

/*
 * An architecture is specified by one of the following codes
 * - each architecture defines a set of solvers (and the supported theories)
 * - the special "auto" codes can be used if mode is CTX_MODE_ONECHECK
 */
typedef enum {
  CTX_ARCH_NOSOLVERS,    // core only
  CTX_ARCH_EG,           // egraph
  CTX_ARCH_SPLX,         // simplex
  CTX_ARCH_IFW,          // integer floyd-warshall
  CTX_ARCH_RFW,          // real floyd-warshall
  CTX_ARCH_BV,           // bitvector solver
  CTX_ARCH_EGFUN,        // egraph+array/function theory
  CTX_ARCH_EGSPLX,       // egraph+simplex
  CTX_ARCH_EGBV,         // egraph+bitvector solver
  CTX_ARCH_EGFUNSPLX,    // egraph+fun+simplex
  CTX_ARCH_EGFUNBV,      // egraph+fun+bitvector
  CTX_ARCH_EGSPLXBV,     // egraph+simplex+bitvector
  CTX_ARCH_EGFUNSPLXBV,  // all solvers (should be the default)

  CTX_ARCH_AUTO_IDL,     // either simplex or integer floyd-warshall
  CTX_ARCH_AUTO_RDL,     // either simplex or real floyd-warshall
} context_arch_t;


#define NUM_ARCH (CTX_ARCH_AUTO_RDL+1)


/*
 * Supported theories (including arithmetic and function theory variants)
 * - a 32bit theories word indicate what's supported
 * - each bit selects a theory
 * The theory word is derived from the architecture descriptor
 */
#define UF_MASK        0x1
#define BV_MASK        0x2
#define IDL_MASK       0x4
#define RDL_MASK       0x8
#define LIA_MASK       0x10
#define LRA_MASK       0x20
#define LIRA_MASK      0x40
#define NLIRA_MASK     0x80     // non-linear arithmetic
#define FUN_UPDT_MASK  0x100
#define FUN_EXT_MASK   0x200
#define QUANT_MASK     0x400

// arith means all variants of linear arithmetic are supported
#define ARITH_MASK (LIRA_MASK|LRA_MASK|LIA_MASK|RDL_MASK|IDL_MASK)

// nlarith_mask means non-linear arithmetic is supported too
#define NLARITH_MASK (NLIRA_MASK|ARITH_MASK)

// fun means both function theories are supported
#define FUN_MASK   (FUN_UPDT_MASK|FUN_EXT_MASK)

// all theories, except non-linear arithmetic
#define ALLTH_MASK (UF_MASK|BV_MASK|ARITH_MASK|FUN_MASK)




/***********************************
 *  PREPROCESSING/SIMPLIFICATION   *
 **********************************/

/*
 * Marks for detecting cycles in variable substitutions
 * - white: term not visited
 * - grey: term found but not fully explored yet
 * - black: fully explored term
 */
enum {
  WHITE = 0,
  GREY = 1,
  BLACK = 2,
};




/********************************
 *  BITVECTOR SOLVER INTERFACE  *
 *******************************/

/*
 * Term constructors
 * -----------------
 * 1) thvar_t create_var(void *solver, uint32_t n)
 *    - must return the index of a new bitvector variable (no eterm attached)
 *    - n = number of bits of that variable
 *
 * 2a) thvar_t create_const(void *solver, bvconst_term_t *const)
 * 2b) thvar_t create_const64(void *solver, bvconst64_term_t *const)
 *    - must return the index of a variable x equal to the constant const
 *    - const->nbits = number of bits
 *    - const->bits = array of uint32_t words (constant value)
 *
 * 3a) thvar_t create_poly(void *solver, bvpoly_t *p, thvar_t *map)
 * 3b) thvar_t create_poly64(void *solver, bvpoly64_t *p, thvar_t *map)
 *    - must return a theory variable that represents p with variables renamed as
 *      defined by map:
 *      p is a_0 t_0 + ... + a_n t_n and map[i] = variable x_i mapped to t_i
 *      with the exception that map[0] = null_thvar if x_0 is const_idx
 *
 * 4) thvar_t create_pprod(void *solver, pprod_t *r, thvar_t *map)
 *    - return a theory variable to represent the product (t_0 ^ d_0 ... t_n ^ d_n)
 *    - map is an array of n+1 theory variables x_0 ... x_n such that
 *      x_i is mapped to t_i in the internalization table.
 *
 * 5) thvar_t create_bvarray(void *solver, literal_t *a, uint32_t n)
 *    - must return a theory variable that represent the array a[0 ... n-1]
 *    - a[0 ... n-1] are all literals in the core
 *    - a[0] is the low order bit, a[n-1] is the high order bit
 *
 * 6) thvar_t create_bvite(void *solver, literal_t c, thvar_t x, thvar_t y)
 *    - create (ite c x y): x and y are two theory variables in solver,
 *      and c is a literal in the core.
 *
 * 7) binary operators
 *    thvar_t create_bvdiv(void *solver, thvar_t x, thvar_t y)
 *    thvar_t create_bvrem(void *solver, thvar_t x, thvar_t y)
 *    thvar_t create_bvsdiv(void *solver, thvar_t x, thvar_t y)
 *    thvar_t create_bvsrem(void *solver, thvar_t x, thvar_t y)
 *    thvar_t create_bvsmod(void *solver, thvar_t x, thvar_t y)
 *    thvar_t create_bvshl(void *solver, thvar_t x, thvar_t y)
 *    thvar_t create_bvlshr(void *solver, thvar_t x, thvar_t y)
 *    thvar_t create_bvashr(void *solver, thvar_t x, thvar_t y)
 *
 * 8) bit extraction
 *    literal_t select_bit(void *solver, thvar_t x, uint32_t i)
 *    - must return bit i of theory variable x as a literal in the core
 *
 * Atom creation
 * -------------
 * 9) literal_t create_eq_atom(void *solver, thvar_t x, thvar_t y)
 * 10) literal_t create_ge_atom(void *solver, thvar_t x, thvar_t y)
 * 11) literal_t create_sge_atom(void *solver, thvar_t x, thvar_t y)
 *
 * Axiom assertion
 * ---------------
 * assert axiom if tt is true, the negation of axiom otherwise
 * 12) void assert_eq_axiom(void *solver, thvar_t x, thvar_t y, bool tt)
 * 13) void assert_ge_axiom(void *solver, thvar_t x, thvar_t y, bool tt)
 * 14) void assert_sge_axiom(void *solver, thvar_t x, thvar_t y, bool tt)
 *
 * 15) void set_bit(void *solver, thvar_t x, uint32_t i, bool tt)
 *   - assign bit i of x to true or false (depending on tt)
 *
 * Egraph interface: NOT AVAILABLE HERE
 * ------------------------------------
 * 16) void attach_eterm(void *solver, thvar_t v, eterm_t t)
 *    - attach egraph term t to theory variable v of solver
 *
 * 17) eterm_t eterm_of_var(void *solver, thvar_t v)
 *    - return the egraph term attached to v in solver (or null_eterm
 *      if v has no egraph term attached).
 *
 * Model construction
 * ------------------
 * Same functions as for the arithmetic solvers
 *
 * 18) void build_model(void *solver)
 *     - build a model (that maps solver variables to bitvector constants)
 *
 * 19) void free_model(void *solver)
 *     - notify the solver that the model is no longer needed
 *
 * 20) bool value_in_model(void *solver, thvar_t x, bvconstant_t *v):
 *     - copy the value of x into v and return true.
 *     - if model construction is not supported or the value is not available, return false.
 */
typedef thvar_t (*create_bv_var_fun_t)(void *solver, uint32_t nbits);
typedef thvar_t (*create_bv_const_fun_t)(void *solver, bvconst_term_t *c);
typedef thvar_t (*create_bv64_const_fun_t)(void *solver, bvconst64_term_t *c);
typedef thvar_t (*create_bv_poly_fun_t)(void *solver, bvpoly_t *p, thvar_t *map);
typedef thvar_t (*create_bv64_poly_fun_t)(void *solver, bvpoly64_t *p, thvar_t *map);
typedef thvar_t (*create_bv_pprod_fun_t)(void *solver, pprod_t *p, thvar_t *map);
typedef thvar_t (*create_bv_array_fun_t)(void *solver, literal_t *a, uint32_t n);
typedef thvar_t (*create_bv_ite_fun_t)(void *solver, literal_t c, thvar_t x, thvar_t y);
typedef thvar_t (*create_bv_binop_fun_t)(void *solver, thvar_t x, thvar_t y);
typedef literal_t (*create_bv_atom_fun_t)(void *solver, thvar_t x, thvar_t y);
typedef literal_t (*select_bit_fun_t)(void *solver, thvar_t x, uint32_t i);
typedef void (*assert_bv_axiom_fun_t)(void *solver, thvar_t x, thvar_t y, bool tt);
typedef void (*set_bit_fun_t)(void *solver, thvar_t x, uint32_t i, bool tt);
typedef void (*build_model_fun_t)(void *solver);
typedef void (*free_model_fun_t)(void *solver);
typedef bool (*bv_val_in_model_fun_t)(void *solver, thvar_t x, bvconstant_t *v);


typedef struct bv_interface_s {
  create_bv_var_fun_t create_var;
  create_bv_const_fun_t create_const;
  create_bv64_const_fun_t create_const64;
  create_bv_poly_fun_t create_poly;
  create_bv64_poly_fun_t create_poly64;
  create_bv_pprod_fun_t create_pprod;
  create_bv_array_fun_t create_bvarray;
  create_bv_ite_fun_t create_bvite;
  create_bv_binop_fun_t create_bvdiv;
  create_bv_binop_fun_t create_bvrem;
  create_bv_binop_fun_t create_bvsdiv;
  create_bv_binop_fun_t create_bvsrem;
  create_bv_binop_fun_t create_bvsmod;
  create_bv_binop_fun_t create_bvshl;
  create_bv_binop_fun_t create_bvlshr;
  create_bv_binop_fun_t create_bvashr;

  select_bit_fun_t select_bit;
  create_bv_atom_fun_t create_eq_atom;
  create_bv_atom_fun_t create_ge_atom;
  create_bv_atom_fun_t create_sge_atom;

  assert_bv_axiom_fun_t assert_eq_axiom;
  assert_bv_axiom_fun_t assert_ge_axiom;
  assert_bv_axiom_fun_t assert_sge_axiom;
  set_bit_fun_t set_bit;

  build_model_fun_t build_model;
  free_model_fun_t free_model;
  bv_val_in_model_fun_t value_in_model;
} bv_interface_t;



/**************
 *  CONTEXT   *
 *************/

struct context_s {

  //lock to synchronize multithreaded access
  yices_lock_t      lock;

  // mode + architecture
  context_mode_t mode;
  context_arch_t arch;

  // theories flag
  uint32_t theories;

  // options flag
  uint32_t options;

  // base_level == number of calls to push
  uint32_t base_level;

  // core and theory solvers
  smt_core_t *core;
  void *bv_solver;

  // solver internalization interfaces
  bv_interface_t bv;

  // input are all from the following tables (from yices_globals.h)
  type_table_t *types;
  term_table_t *terms;

  // hash table for Boolean gates
  gate_manager_t gate_manager;

  // internalization table
  intern_tbl_t intern;

  // result of flattening and simplification
  ivector_t top_eqs;
  ivector_t top_atoms;
  ivector_t top_formulas;
  ivector_t top_interns;

  // auxiliary buffers and structures for internalization
  ivector_t subst_eqs;
  ivector_t aux_eqs;
  int_queue_t queue;
  ivector_t aux_vector;
  int_stack_t istack;

  // optional components: allocated if needed
  pseudo_subst_t *subst;
  mark_vector_t *marks;
  int_bvset_t *cache;
  int_hset_t *small_cache;
  pmap2_t *eq_cache;

  // auxiliary buffers for model construction
  bvconstant_t bv_buffer;

  // for exception handling
  jmp_buf env;

  // for verbose output (default NULL)
  tracer_t *trace;
};



/*
 * Default initial size of auxiliary buffers and vectors
 */
#define CTX_DEFAULT_VECTOR_SIZE 10


/*
 * Default initial size for the solvers
 */
#define CTX_DEFAULT_CORE_SIZE 100


/*
 * Error and return codes used by internalization procedures:
 * - negative codes indicate an error
 * - these codes can also be used by the theory solvers to report exceptions.
 */
enum {
  TRIVIALLY_UNSAT = 1,   // simplifies to false
  CTX_NO_ERROR = 0,      // internalization succeeds/not solved
  // bugs
  INTERNAL_ERROR = -1,
  TYPE_ERROR = -2,
  // general errors
  FREE_VARIABLE_IN_FORMULA = -3,
  LOGIC_NOT_SUPPORTED = -4,
  UF_NOT_SUPPORTED = -5,
  ARITH_NOT_SUPPORTED = -6,
  BV_NOT_SUPPORTED = -7,
  FUN_NOT_SUPPORTED = -8,
  QUANTIFIERS_NOT_SUPPORTED = -9,
  LAMBDAS_NOT_SUPPORTED = -10,
  // arithmetic solver errors
  FORMULA_NOT_IDL = -11,
  FORMULA_NOT_RDL = -12,
  FORMULA_NOT_LINEAR = -13,
  TOO_MANY_ARITH_VARS = -14,
  TOO_MANY_ARITH_ATOMS = -15,
  ARITHSOLVER_EXCEPTION = -16,
  // bv solver errors
  BVSOLVER_EXCEPTION = -17,
};


/*
 * NUM_INTERNALIZATION_ERRORS: must be (1 + number of negative codes)
 */
#define NUM_INTERNALIZATION_ERRORS 18





/********************************
 *  INITIALIZATION AND CONTROL  *
 *******************************/

/*
 * Initialize ctx for the given mode and architecture
 * - terms = term table for this context
 * - qflag = false means quantifier-free variant
 * - qflag = true means quantifiers allowed
 * If arch is one of the ARCH_AUTO_... variants, then mode must be ONECHECK
 */
extern void init_context(context_t *ctx, term_table_t *terms,
                         context_mode_t mode, context_arch_t arch, bool qflag);


/*
 * Deletion
 */
extern void delete_context(context_t *ctx);

/*
 * Reset: remove all assertions
 */
extern void reset_context(context_t *ctx);

/*
 * Set the trace:
 * - the current tracer must be NULL.
 * - the tracer is also attached to the context's smt_core
 *   (so all solvers can use it to print stuff).
 */
extern void context_set_trace(context_t *ctx, tracer_t *trace);

/*
 * Push and pop
 * - should not be used if the push_pop option is disabled
 */
extern void context_push(context_t *ctx);
extern void context_pop(context_t *ctx);




/**********************************
 *  SIMPLIFICATION AND UTILITIES  *
 *********************************/

/*
 * The following functions are implemented in context_simplifier.c.
 * They can be used by any procedure that processes assertions but does
 * not depend on the solvers instantiated in a conterxt.
 */

/*
 * INTERNAL CACHES AND AUXILIARY STRUCTURES
 */

/*
 * There are two internal caches for visiting terms.
 * - the 'cache' uses a bitvector implementation and should be
 *   better for operations that visit many terms.
 * - the 'small_cache' uses a hash table and should be better
 *   for operations that visit a small number of terms.
 *
 * There are three buffers for internal construction of polynomials
 * - arith_buffer is more expensive (requires more memory) but
 *   it supports more operations (e.g., term constructors in yices_api.c
 *   take arith_buffers as arguments).
 * - poly_buffer is a cheaper data structure, but it does not support
 *   all the operations
 * - aux_poly is even cheaper, but it's for direct construction only
 */

/*
 * Allocate/reset/free the small cache
 * - the cache is allocated and initialized on the first call to get_small_cache
 * - reset and free do nothing if the cache is not allocated
 */
extern int_hset_t *context_get_small_cache(context_t *ctx);
extern void context_reset_small_cache(context_t *ctx);
extern void context_free_small_cache(context_t *ctx);


/*
 * Allocate/free the cache
 * - same conventions as for the small_cache
 */
extern int_bvset_t *context_get_cache(context_t *ctx);
extern void context_free_cache(context_t *ctx);


/*
 * EQUALITY CACHE
 */

/*
 * If lift-if is enabled then arithmetic equalities
 *  (eq (ite c t1 t2) u) are rewritten to (ite c (eq t1 u) (eq t2 u))
 * We don't create new terms (eq t1 u) or (eq t2 u). Instead, we store
 * the internalization of equalities (eq t1 u) in the eq_cache:
 * This cache maps pairs of terms <t, u> to a literal l (such that
 * l is the internalization of (t == u)).
 *
 * The following functions operate on this cache: reset/free/push/pop
 * do nothing if the cache does not exist.
 */
extern pmap2_t *context_get_eq_cache(context_t *ctx);
extern void context_free_eq_cache(context_t *ctx);
extern void context_eq_cache_push(context_t *ctx);
extern void context_eq_cache_pop(context_t *ctx);

static inline void context_reset_eq_cache(context_t *ctx) {
  context_free_eq_cache(ctx);
}


/*
 * Check what's mapped to (t1, t2) in the internal eq_cache.
 * - return null_literal if nothing is mapped to (t1, t2) (or if the cache does not exit)
 */
extern literal_t find_in_eq_cache(context_t *ctx, term_t t1, term_t t2);


/*
 * Add the mapping (t1, t2) --> l to the equality cache.
 * - allocate and initialize the cache if needed.
 * - the pair (t1, t2) must not be in the cache already.
 * - l must be different from null_literal
 */
extern void add_to_eq_cache(context_t *ctx, term_t t1, term_t t2, literal_t l);



/*
 * SIMPLIFICATION
 */

/*
 * Check whether t is true or false (i.e., mapped to 'true_occ' or 'false_occ'
 * in the internalization table.
 * - t must be a root in the internalization table
 */
extern bool term_is_true(context_t *ctx, term_t t);
extern bool term_is_false(context_t *ctx, term_t t);


/*
 * Check whether (t1 == t2) can be simplified to an existing term
 * (including true_term or false_term).
 * - t1 and t2 must be Boolean terms
 * - return NULL_TERM if no simplifcation is found
 */
extern term_t simplify_bool_eq(context_t *ctx, term_t t1, term_t t2);


/*
 * Same thing for bitvector terms
 * - both t1 and t2 must be root terms in the internalization table
 */
extern term_t simplify_bitvector_eq(context_t *ctx, term_t t1, term_t t2);



/*
 * FLATTENING AND VARIABLE ELIMINATION
 */

/*
 * Flattening of disjunctions
 * - rewrite nested OR terms to flat OR terms
 * - if option FLATTENDISEQ is enabled, also replace arithmetic
 *   disequalities by disjunctions of strict inequalities:
 *    (i.e., rewrite x!= 0 to (or (< x 0) (> x 0))
 *
 * The function applies flattenning to composite term or:
 * - or must be of the form (or t1 .... tn)
 * - v must be empty
 * - flattening is applied recursively to t1 ... t_n
 * - the result is stored in v: it 's an array of Boolean terms
 *   u_1 .... u_m such that (or t1 ... t_n)  is equivalent to (or u_1 ... u_m).
 *
 * Side effect: use ctx's small_cache then reset it
 */
extern void flatten_or_term(context_t *ctx, ivector_t *v, composite_term_t *or);


/*
 * If t is (ite c a b), we can try to rewrite (= t k) into a conjunction
 * of terms using the two rules:
 *   (= (ite c a b) k) --> c and (= a k)        if k != b holds
 *   (= (ite c a b) k) --> (not c) and (= b k)  if k != a holds
 *
 * This works best for the NEC benchmarks in SMT LIB, where many terms
 * are deeply nested if-then-else terms with constant leaves.
 *
 * The function below does that: it rewrites (eq t k) to (and c_0 ... c_n (eq t' k))
 * - the boolean terms c_0 ... c_n are added to vector v
 * - the term t' is returned
 *
 * So the simplification worked it the returned term t' is different from t
 * (and then v->size is not 0).
 */
extern term_t flatten_ite_equality(context_t *ctx, ivector_t *v, term_t t, term_t k);


/*
 * Simplify and flatten assertion f.
 *
 * This function performs top-down Boolean propagation and collects
 * all subterms of f that can't be flattened into four vectors:
 *
 * 1) ctx->top_eqs = top-level equalities.
 *    Every t in top_eqs is (eq t1 * t2) (or a variant) asserted true.
 *    t is mapped to true in the internalization table.
 *
 * 2) ctx->top_atoms = top-level atoms.
 *    Every t in top_atoms is an atom or the negation of an atom (that
 *    can't go into top_eqs).
 *    t is mapped to true in the internalization table.
 *
 * 3) ctx->top_formulas = non-atomic terms.
 *    Every t in top_formulas is either an (OR ...) or (ITE ...) or (XOR ...)
 *    or the negation of such a term.
 *    t is mapped to true in the internalization table.
 *
 * 4) ctx->top_interns = already internalized terms.
 *    Every t in top_interns is a term that's been internalized before
 *    and is mapped to a literal l or an egraph occurrence g in
 *    the internalization table.
 *    l or g must be asserted true in later stages.
 *
 *
 * If variable elimination is enabled, then equalities of the form (= x t)
 * where x is a variable are converted to substitutions if possible:
 *
 * 1) if t is a constant or variable: then [x := t] is added as a substitution
 *    to ctx->intern_tbl (if possible)
 *
 * 2) other equalities of the form (= x t) are added to ctx->subst_eqs. to
 *    be processed later by process_candidate_subst
 *
 * This function raises an exception via longjmp if there's an error
 * or if a contradiction is detected. So ctx->env must be set.
 */
extern void flatten_assertion(context_t *ctx, term_t f);


/*
 * Auxiliary equalities:
 * - add a new equality (x == y) in the aux_eq vector.
 * - this is useful for simplification procedures that are executed after
 *   assertion flattening (e.g., symmetry breaking).
 * - the auxiliary equalities can then be processed by process_aux_eqs
 */
extern void add_aux_eq(context_t *ctx, term_t x, term_t y);


/*
 * Process the auxiliary equalities:
 * - if substitution is not enabled, then all aux equalities are added to top_eqs
 * - otherwise, cheap substitutions are performand and candidate substitutions
 *   are added to subst_eqs.
 *
 * This function raises an exception via longjmp if a contradiction os detected.
 */
extern void process_aux_eqs(context_t *ctx);


/*
 * Process all candidate substitutions after flattening and processing of
 * auxiliary equalities.
 * - the candidate substitutions are in ctx->subst_eqs
 * - all elemenst of subst_eqs must be equality terms asserted true
 *   and of the form (= x t) for some variable x.
 * - converts these equalities into substitutions, as long as this
 *   can be done without creating substitution cycles.
 * - candidate substitution  that can't be converted are moved to
 *   ctx->top_eqs.
 */
extern void context_process_candidate_subst(context_t *ctx);



/*
 * TYPES AFTER VARIABLE ELIMINATION
 */

/*
 * Get the type of r's class
 * - r must be a root in the internalization table
 */
static inline type_t type_of_root(context_t *ctx, term_t r) {
  return intern_tbl_type_of_root(&ctx->intern, r);
}

/*
 * PREPROCESSING/ANALYSIS AFTER FLATTENING
 */


/*
 * CLEANUP
 */

/*
 * Subst/mark are used by flattening if variable elimination is
 * enabled. The dl_profile is allocated in analyze_diff_logic. The
 * following functions must be called to delete these internal
 * structures. They do nothing if the structures haven't been
 * allocated.
 */
extern void context_free_subst(context_t *ctx);
extern void context_free_marks(context_t *ctx);




/****************************
 *   ASSERTIONS AND CHECK   *
 ***************************/

/*
 * Assert a boolean formula f.
 *
 * The context status must be IDLE.
 *
 * Return code:
 * - TRIVIALLY_UNSAT means that an inconsistency is detected
 *   (in that case the context status is set to UNSAT)
 * - CTX_NO_ERROR means no internalization error and status not
 *   determined
 * - otherwise, the code is negative. The assertion could
 *   not be processed.
 */
extern int32_t assert_formula(context_t *ctx, term_t f);


/*
 * Assert all formulas f[0] ... f[n-1]
 * same return code as above.
 */
extern int32_t assert_formulas(context_t *ctx, uint32_t n, term_t *f);


/*
 * Convert boolean term t to a literal l in context ctx
 * - return a negative code if there's an error
 * - return a literal (l >= 0) otherwise.
 */
extern int32_t context_internalize(context_t *ctx, term_t t);


/*
 * Add the blocking clause to ctx
 * - ctx->status must be either SAT or UNKNOWN
 * - this collects all decision literals in the current truth assignment
 *   (say l_1, ..., l_k) then clears the current assignment and adds the
 *  clause ((not l_1) \/ ... \/ (not l_k)).
 *
 * Return code:
 * - TRIVIALLY_UNSAT: means that the blocking clause is empty (i.e., k = 0)
 *   (in that case, the context status is set to UNSAT)
 * - CTX_NO_ERROR: means that the blocking clause is not empty (i.e., k > 0)
 *   (In this case, the context status is set to IDLE)
 */
extern int32_t assert_blocking_clause(context_t *ctx);


/*
 * Check whether the context is consistent
 * - parameters = search and heuristic parameters to use
 * - if parameters is NULL, the default values are used
 *
 * return status: either STATUS_UNSAT, STATUS_SAT, STATUS_UNKNOWN,
 * STATUS_INTERRUPTED (these codes are defined in smt_core.h)
 */
extern smt_status_t check_context(context_t *ctx, const param_t *parameters);


/*
 * Build a model: the context's status must be STATUS_SAT or STATUS_UNKNOWN
 * - model must be initialized (and empty)
 * - the model maps a value to every uninterpreted terms present in ctx's
 *   internalization tables
 * - if model->has_alias is true, the term substitution defined by ctx->intern_tbl
 *   is copied into the model
 */
extern void context_build_model(model_t *model, context_t *ctx);


/*
 * Interrupt the search
 * - this can be called after check_context from a signal handler
 * - this interrupts the current search
 * - if clean_interrupt is enabled, calling context_cleanup will
 *   restore the solver to a good state, equivalent to the state
 *   before the call to check_context
 * - otherwise, the solver is in a bad state from which new assertions
 *   can't be processed. Cleanup is possible via pop (if push/pop is supported)
 *   or reset.
 *
 * NOTE: DON'T TRY TO ACQUIRE THE LOCK ON CTX (since it is an interrupter)
 */
extern void context_stop_search(context_t *ctx);


/*
 * Cleanup after check is interrupted
 * - must not be called if the clean_interrupt option is disabled
 * - restore the context to a good state (status = IDLE)
 */
extern void context_cleanup(context_t *ctx);


/*
 * Clear boolean assignment and return to the IDLE state.
 * - this can be called after check returns UNKNOWN or SEARCHING
 *   provided the context's mode isn't ONECHECK
 * - after this call, additional formulas can be asserted and
 *   another call to check_context is allowed. Model construction
 *   is no longer possible until the next call to check_context.
 */
extern void context_clear(context_t *ctx);


/*
 * Cleanup after the search returned UNSAT
 * - if the clean_interrupt option is enabled, this restore
 *   the state to what it was at the start of search
 * - otherwise, this does nothing.
 *
 * NOTE: Call this before context_pop(ctx) if the context status
 * is unsat.
 */
extern void context_clear_unsat(context_t *ctx);





/*****************************
 *  INTERNALIZATION OPTIONS  *
 ****************************/

/*
 * Set or clear preprocessing options
 */
static inline void enable_variable_elimination(context_t *ctx) {
  ctx->options |= VARELIM_OPTION_MASK;
}

static inline void disable_variable_elimination(context_t *ctx) {
  ctx->options &= ~VARELIM_OPTION_MASK;
}

static inline void enable_or_flattening(context_t *ctx) {
  ctx->options |= FLATTENOR_OPTION_MASK;
}

static inline void disable_or_flattening(context_t *ctx) {
  ctx->options &= ~FLATTENOR_OPTION_MASK;
}

static inline void enable_diseq_and_or_flattening(context_t *ctx) {
  ctx->options |= FLATTENOR_OPTION_MASK|FLATTENDISEQ_OPTION_MASK;
}

static inline void disable_diseq_and_or_flattening(context_t *ctx) {
  ctx->options &= ~(FLATTENOR_OPTION_MASK|FLATTENDISEQ_OPTION_MASK);
}

static inline void enable_eq_abstraction(context_t *ctx) {
  ctx->options |= EQABSTRACT_OPTION_MASK;
}

static inline void disable_eq_abstraction(context_t *ctx) {
  ctx->options &= ~EQABSTRACT_OPTION_MASK;
}

static inline void enable_arith_elimination(context_t *ctx) {
  ctx->options |= ARITHELIM_OPTION_MASK;
}

static inline void disable_arith_elimination(context_t *ctx) {
  ctx->options &= ~ARITHELIM_OPTION_MASK;
}

static inline void enable_keep_ite(context_t *ctx) {
  ctx->options |= KEEP_ITE_OPTION_MASK;
}

static inline void disable_keep_ite(context_t *ctx) {
  ctx->options &= ~KEEP_ITE_OPTION_MASK;
}

static inline void enable_bvarith_elimination(context_t *ctx) {
  ctx->options |= BVARITHELIM_OPTION_MASK;
}

static inline void disable_bvarith_elimination(context_t *ctx) {
  ctx->options &= ~BVARITHELIM_OPTION_MASK;
}

static inline void enable_symmetry_breaking(context_t *ctx) {
  ctx->options |= BREAKSYM_OPTION_MASK;
}

static inline void disable_symmetry_breaking(context_t *ctx) {
  ctx->options &= ~BREAKSYM_OPTION_MASK;
}

static inline void enable_pseudo_inverse_simplification(context_t *ctx) {
  ctx->options |= PSEUDO_INVERSE_OPTION_MASK;
}

static inline void disable_pseudo_inverse_simplification(context_t *ctx) {
  ctx->options &= ~PSEUDO_INVERSE_OPTION_MASK;
}


/*
 * Check which options are enabled
 */
static inline bool context_var_elim_enabled(context_t *ctx) {
  return (ctx->options & VARELIM_OPTION_MASK) != 0;
}

static inline bool context_flatten_or_enabled(context_t *ctx) {
  return (ctx->options & FLATTENOR_OPTION_MASK) != 0;
}

static inline bool context_flatten_diseq_enabled(context_t *ctx) {
  return (ctx->options & FLATTENDISEQ_OPTION_MASK) != 0;
}

static inline bool context_eq_abstraction_enabled(context_t *ctx) {
  return (ctx->options & EQABSTRACT_OPTION_MASK) != 0;
}

static inline bool context_arith_elim_enabled(context_t *ctx) {
  return (ctx->options & ARITHELIM_OPTION_MASK) != 0;
}

static inline bool context_keep_ite_enabled(context_t *ctx) {
  return (ctx->options & KEEP_ITE_OPTION_MASK) != 0;
}

static inline bool context_bvarith_elim_enabled(context_t *ctx) {
  return (ctx->options & BVARITHELIM_OPTION_MASK) != 0;
}

static inline bool context_breaksym_enabled(context_t *ctx) {
  return (ctx->options & BREAKSYM_OPTION_MASK) != 0;
}

static inline bool context_pseudo_inverse_enabled(context_t *ctx) {
  return (ctx->options & PSEUDO_INVERSE_OPTION_MASK) != 0;
}

static inline bool context_has_preprocess_options(context_t *ctx) {
  return (ctx->options & PREPROCESSING_OPTIONS_MASK) != 0;
}

static inline bool context_dump_enabled(context_t *ctx) {
  return (ctx->options & DUMP_OPTION_MASK) != 0;
}

static inline bool splx_eager_lemmas_enabled(context_t *ctx) {
  return (ctx->options & SPLX_EGRLMAS_OPTION_MASK) != 0;
}

static inline bool splx_periodic_icheck_enabled(context_t *ctx) {
  return (ctx->options & SPLX_ICHECK_OPTION_MASK) != 0;
}

static inline bool splx_eqprop_enabled(context_t *ctx) {
  return (ctx->options & SPLX_EQPROP_OPTION_MASK) != 0;
}


/*
 * Provisional: set/clear/test dump mode
 */
static inline void enable_dump(context_t *ctx) {
  ctx->options |= DUMP_OPTION_MASK;
}

static inline void disable_dump(context_t *ctx) {
  ctx->options &= ~DUMP_OPTION_MASK;
}

// Lax mode
static inline void enable_lax_mode(context_t *ctx) {
  ctx->options |= LAX_OPTION_MASK;
}

static inline void disable_lax_mode(context_t *ctx) {
  ctx->options &= ~LAX_OPTION_MASK;
}

static inline bool context_in_strict_mode(context_t *ctx) {
  return (ctx->options & LAX_OPTION_MASK) == 0;
}


/********************************
 *  CHECK THEORIES AND SOLVERS  *
 *******************************/

/*
 * Supported theories
 */
static inline bool context_allows_uf(context_t *ctx) {
  return (ctx->theories & UF_MASK) != 0;
}

static inline bool context_allows_bv(context_t *ctx) {
  return (ctx->theories & BV_MASK) != 0;
}

static inline bool context_allows_idl(context_t *ctx) {
  return (ctx->theories & IDL_MASK) != 0;
}

static inline bool context_allows_rdl(context_t *ctx) {
  return (ctx->theories & RDL_MASK) != 0;
}

static inline bool context_allows_lia(context_t *ctx) {
  return (ctx->theories & LIA_MASK) != 0;
}

static inline bool context_allows_lra(context_t *ctx) {
  return (ctx->theories & LRA_MASK) != 0;
}

static inline bool context_allows_lira(context_t *ctx) {
  return (ctx->theories & LIRA_MASK) != 0;
}

static inline bool context_allows_nlarith(context_t *ctx) {
  return (ctx->theories & NLIRA_MASK) != 0;
}

static inline bool context_allows_fun_updates(context_t *ctx) {
  return (ctx->theories & FUN_UPDT_MASK) != 0;
}

static inline bool context_allows_extensionality(context_t *ctx) {
  return (ctx->theories & FUN_EXT_MASK) != 0;
}

static inline bool context_allows_quantifiers(context_t *ctx) {
  return (ctx->theories & QUANT_MASK) != 0;
}


/*
 * Check which solvers are present
 */
static inline bool context_has_bv_solver(context_t *ctx) {
  return ctx->bv_solver != NULL;
}


/*
 * Optional features
 */
static inline bool context_supports_multichecks(context_t *ctx) {
  return (ctx->options & MULTICHECKS_OPTION_MASK) != 0;
}

static inline bool context_supports_pushpop(context_t *ctx) {
  return (ctx->options & PUSHPOP_OPTION_MASK) != 0;
}

static inline bool context_supports_cleaninterrupt(context_t *ctx) {
  return (ctx->options & CLEANINT_OPTION_MASK) != 0;
}


/*
 * Read the mode flag
 */
static inline context_mode_t context_get_mode(context_t *ctx) {
  return ctx->mode;
}



/***************
 *  UTILITIES  *
 **************/

/*
 * Read the status: returns one of
 *  STATUS_IDLE        (before check_context)
 *  STATUS_SEARCHING   (during check_context)
 *  STATUS_UNKNOWN
 *  STATUS_SAT
 *  STATUS_UNSAT
 *  STATUS_INTERRUPTED
 */
static inline smt_status_t context_status(context_t *ctx) {
  return smt_status(ctx->core);
}


/*
 * Read the base_level (= number of calls to push)
 */
static inline uint32_t context_base_level(context_t *ctx) {
  return ctx->base_level;
}



/*
 * GARBAGE-COLLECTION SUPPORT
 */

/*
 * Mark all terms present in ctx (to make sure they survive the
 * next call to term_table_gc).
 */
extern void context_gc_mark(context_t *ctx);


#endif /* __CONTEXT_H */