/*
 * The Yices SMT Solver. Copyright 2015 SRI International.
 *
 * This program may only be used subject to the noncommercial end user
 * license agreement which is downloadable along with this program.
 */

/*
 * STAND-ALONE SAT SOLVER
 */

/*
 * This is a new implementation based on Hadrien Barral's work.
 * Hadrien's original code is in sat_solver.h/sat_sover.c.
 * This is a cleanup but similar implementation.
 */

#ifndef __NEW_SAT_SOLVER_H
#define __NEW_SAT_SOLVER_H

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>




/****************
 *  UTILITIES   *
 ***************/

/*
 * We use arrays of 32bit integers in several places.
 * The following macro is intended to define the maximal
 * number of elements in such an array (assuming we store the
 * array size as an unsigned 32bit integer).
 *
 * Not sure this definition is portable, but it should work
 * with gcc and clang.
 */
#if (SIZE_MAX/4) < UINT32_MAX
#define MAX_ARRAY32_SIZE (SIZE_MAX/4)
#else
#define MAX_ARRAY32_SIZE UINT32_MAX
#endif



/************************************
 *  BOOLEAN VARIABLES AND LITERALS  *
 ***********************************/

/*
 * Boolean variables: integers between 1 and nvars.
 * Literals: integers between 2 and 2nvar + 1.
 *
 * For a variable x, the positive literal is 2x, the negative
 * literal is 2x + 1.
 *
 * Variable index 0 is reserved. The corresponding literals
 * 0 and 1 denote true and false, respectively.
 */
typedef uint32_t bvar_t;
typedef uint32_t literal_t;


/*
 * Maximal number of boolean variables.
 * That's also the maximal size of any clause.
 */
#define MAX_VARIABLES (UINT32_MAX >> 2)
#define MAX_CLAUSE_SIZE MAX_VARIABLES

/*
 * Conversions from variables to literals
 */
static inline literal_t pos(bvar_t x) {
  return (x << 1);
}

static inline literal_t neg(bvar_t x) {
  return (x << 1) + 1;
}

static inline bvar_t var_of(literal_t l) {
  return l>>1;
}

// sign: 0 --> positive, 1 --> negative
static inline int32_t sign_of(literal_t l) {
  return l & 1;
}

// negation of literal l
static inline literal_t not(literal_t l) {
  return l ^ 1;
}

// check whether l1 and l2 are opposite
static inline bool opposite(literal_t l1, literal_t l2) {
  return (l1 ^ l2) == 1;
}

// true if l has positive polarity (i.e., l = pos_lit(x))
static inline bool is_pos(literal_t l) {
  return !(l & 1);
}

static inline bool is_neg(literal_t l) {
  return (l & 1);
}


/*
 * Assignment values for a variable:
 * - we use four values to encode the truth value of x
 *   when x is assigned and the preferred value when x is
 *   not assigned.
 * - value[x] is interpreted as follows
 *   val_undef_false = 0b00 --> x not assigned, preferred value = false
 *   val_undef_true  = 0b01 --> x not assigned, preferred value = true
 *   val_false = 0b10       --> x assigned false
 *   val_true =  0b11       --> x assigned true
 *
 * The preferred value is used when x is selected as a decision variable.
 * Then we assign x to true or false depending on the preferred value.
 * This is done by setting bit 1 in value[x].
 */
typedef enum bval {
  BVAL_UNDEF_FALSE = 0,
  BVAL_UNDEF_TRUE = 1,
  BVAL_FALSE = 2,
  BVAL_TRUE = 3,
} bval_t;


// check whether val is undef_true or undef_false
static inline bool is_unassigned_val(bval_t val) {
  return (val & 0x2) == 0;
}

// check whether val is val_undef_true of val_true
static inline bool true_preferred(bval_t val) {
  return (val & 0x1) != 0;
}



/*****************
 *  CLAUSE POOL  *
 ****************/

/*
 * Clauses are stored in a big array of integers.
 *
 * Each clause consists of
 * - header: clause length + auxiliary data
 * - for a learned clause, the auxiliary data is the clause's activity.
 * - for a problem clause, the auxiliary data is a bitmask to accelerate
 *   subsumption checks.
 * - the rest is an array of literals
 * - the first two elements of this array are the watched literals.
 *
 * The pool is divided into three regions:
 * - data[0 ... learned-1]           contains the problem clauses
 * - data[learned_base ... size - 1] contains the learned clauses
 * - data[size ... capacity-1]       unused
 *
 * Each clause is identified by an index i:
 * - data[i]   is the clause length
 * - data[i+1] is the auxiliary data
 * - data[i+2] is the first watched literal
 * - data[i+3] is the second watched literal
 * - data[i+4 ... i+n+2] = rest of the clause = array of n-2 literals
 *   where n = data[i] = clause length.
 *
 * Each clause starts at an index that's a multiple of 4. This ensures
 * that header + two watched literals are in the same cache line.
 *
 * If a clause starts at index i, the next clause starts
 * at index j = ((i + data[i] + 2 + 3) & ~3). That's i + length of the
 * clause + size of the header rounded up to the next multiple of 4.
 *
 * Simplification/in-processing may delete or shrink a clause. This
 * introduces gaps in the data array.  To deal with these gaps, we add
 * padding blocks. A padding block at index i is a block of unused
 * elements in the array.  Its length is a multiple of four.  The
 * first two elements of a padding block are as follows: 
 * - data[i] = 0
 * - data[i+1] = length of the padding block.
 * This distinguishes padding blocks from clauses since a clause starts with 
 * data[i] >= 2.
 */

// clause structure
typedef struct clause_s {
  uint32_t len;
  union { 
    uint32_t d; 
    float f;
  } aux;
  literal_t c[0]; // real size is equal to len
} clause_t;

/*
 * Pool structure:
 * - capacity = length of the array data
 * - invariants:
 *     learned <= size <= capacity
 *     available = capacity - size
 *     learned, size, capacity, available are all mutiple of four.
 * - counters: number of clauses/literals
 */
typedef struct clause_pool_s {
  uint32_t *data;
  uint32_t learned;
  uint32_t size;
  uint32_t capacity;
  uint32_t available;
  //  statistics
  uint32_t num_prob_clauses;      // number of problem clauses
  uint32_t num_prob_literals;     // sum of the length of these clauses
  uint32_t num_learned_clauses;   // number of learned clauses
  uint32_t num_learned_literals;  // sum of the length of these clauses
} clause_pool_t;


/*
 * Initial and maximal capacity of a pool
 * - initial size = 1Mb
 */
#define DEF_CLAUSE_POOL_CAPACITY 262144
#define MAX_CLAUSE_POOL_CAPACITY (MAX_ARRAY32_SIZE & ~3)


// clause index
typedef uint32_t cidx_t;


#ifndef NDEBUG
static inline bool good_clause_idx(clause_pool_t *pool, cidx_t idx) {
  return ((idx & 3) == 0) && idx < pool->size;
}
#endif

static inline bool is_learned_clause_idx(clause_pool_t *pool, cidx_t idx) {
  assert(good_clause_idx(pool, idx));
  return  idx >= pool->learned;
}

static inline bool is_problem_clause_idx(clause_pool_t *pool, cidx_t idx) {
  assert(good_clause_idx(pool, idx));
  return  idx < pool->learned;  
}

static inline clause_t *clause_of_idx(clause_pool_t *pool, cidx_t idx) {
  assert(good_clause_idx(pool, idx));
  return (clause_t *) ((char *) (pool->data + idx));
}

/*
 * Number of literals in clause
 */
static inline uint32_t clause_length(clause_pool_t *pool, cidx_t idx) {
  assert(good_clause_idx(pool, idx));
  return pool->data[idx];
}



/*******************
 *  WATCH VECTORS  *
 ******************/

/*
 * For a literal l, watch[l] stores index/clauses in which l is a watched
 * literal. The information is stored as a sequence of records, in an
 * integer array.
 *
 * VERSION 1:
 * - if l is watched literal in a clause cidx of length >= 3, then
 *   the record is cidx.
 * - if l occurs in a binary clause { l, l1 } then the record is the
 *   literal l1
 *
 * To tell the difference, use the lower-order bit of the integer:
 * - [cidx] is stored as is. The two low-order bits of cidx are 00.
 * - [l1] is stored as (l1 << 1)|1  (lower-order bit = 1)
 *
 * The watch structure is a vector:
 * - capacity = full length of the data array
 * - size = number of array elements actually used
 */
typedef struct watch_s {
  uint32_t capacity;
  uint32_t size;
  uint32_t data[0]; // real length = capacity
} watch_t;


/*
 * Initial capacity: smallish.
 */
#define DEF_WATCH_CAPACITY 6
#define MAX_WATCH_CAPACITY (MAX_ARRAY32_SIZE - 2)




/****************************
 *  SOLVER DATA STRUCTURES  *
 ***************************/

/*
 * Assignment stack/propagation queue
 * - an array of literals (the literals assigned to true)
 * - two pointers: top of the stack, beginning of the propagation queue
 * - for each decision level, an index into the stack points
 *   to the literal decided at that level (for backtracking)
 */
typedef struct {
  literal_t *lit;
  uint32_t top;
  uint32_t prop_ptr;
  uint32_t *level_index;
  uint32_t nlevels; // size of level_index array
} sol_stack_t;


/*
 * Initial size of level_index
 */
#define DEFAULT_NLEVELS 100


/*
 * Heap and variable activities for variable selection heuristic
 * - activity[x]: for every variable x between 1 and nvars - 1
 * - indices 0 and -1 are used as markers:
 *    activity[0] = DBL_MAX (higher than any activity)
 *   activity[-1] = -1.0 (lower than any activity);
 * - heap_index[x]: for every variable x,
 *      heap_index[x] = i if x is in the heap and heap[i] = x
 *   or heap_index[x] = -1 if x is not in the heap
 * - heap: array of nvars variables
 * - heap_last: index of last element in the heap
 *   heap[0] = 0,
 *   for i=1 to heap_last, heap[i] = x for some variable x
 * - size = number of variable (nvars)
 * - vmax = variable index (last variable not in the heap)
 * - act_inc: variable activity increment
 * - inv_act_decay: inverse of variable activity decay (e.g., 1/0.95)
 *
 * The set of variables is split into two segments:
 * - [1 ... vmax-1] = variables that are in the heap or have been in the heap
 * - [vmax ... size-1] = variables that may not have been in the heap
 *
 * To pick a decision variable:
 * - search for the most active unassigned variable in the heap
 * - if the heap is empty or all its variables are already assigned,
 *   search for the first unassigned variables in [vmax ... size-1]
 *
 * Initially: we set vmax to 1 (nothing in the heap yet) so decision
 * variables are picked in increasing order, starting from 1.
 */
typedef struct var_heap_s {
  double *activity;
  int32_t *heap_index;
  bvar_t *heap;
  uint32_t heap_last;
  uint32_t size;
  uint32_t vmax;
  double act_increment;
  double inv_act_decay;
} var_heap_t;



/*
 * STATISTICS
 */
typedef struct solver_stats_s {
  uint32_t starts;             // 1 + number of restarts
  uint32_t simplify_calls;     // number of calls to simplify_clause_database
  uint32_t reduce_calls;       // number of calls to reduce_learned_clause_set

  uint64_t decisions;          // number of decisions
  uint64_t random_decisions;   // number of random decisions
  uint64_t propagations;       // number of boolean propagations
  uint64_t conflicts;          // number of conflicts/backtracking

  uint64_t prob_clauses_deleted;     // number of problem clauses deleted
  uint64_t learned_clauses_deleted;  // number of learned clauses deleted

  uint64_t literals_before_simpl;
  uint64_t subsumed_literals;
} solver_stats_t;



/*
 * ANTECEDENT TAGS
 */

/*
 * When a variable is assigned, we store a tag to identify the reason
 * for the assignment. There are four cases:
 * - unit clause
 * - decision literal
 * - propagated from a binary clause
 * - propagated from a non-binary clause
 * + another one for variables not assigned
 */
typedef enum antecedent_tag {
  ATAG_NONE,
  ATAG_UNIT,
  ATAG_DECISION,
  ATAG_BINARY,
  ATAG_CLAUSE,
} antecedent_tag_t;


/*
 * SOLVER STATUS
 */
typedef enum solver_status {
  STAT_UNKNOWN,
  STAT_SAT,
  STAT_UNSAT,
} solver_status_t;


/*
 * For each variable x, we store
 * - ante_tag[x]  = tag for assigned variables + marks
 * - ante_data[x] = antecedent index
 * - value[x]     = assigned value
 * - level[x]     = assingment level
 *
 * For each literal l, we keep
 * - watch[l] = watch vector for l
 */
typedef struct sat_solver_s {
  solver_status_t status;

  uint32_t decision_level;
  uint32_t backtrack_level;
  
  uint32_t prng;              // State of the pseudo-random number generator
  
  /*
   * Variables and literals
   */
  uint32_t nvars;              // Number of variables
  uint32_t nliterals;          // Number of literals = twice nvars
  uint32_t vsize;              // Size of the variable-indexed arrays (>= nvars)
  uint32_t lsize;              // Size of the watch array (>= nlits)

  uint8_t *value;
  uint8_t *ante_tag;
  uint32_t *ante_data;
  uint32_t *level;
  watch_t **watch;

  var_heap_t heap;            // Variable heap
  sol_stack_t stack;          // Assignment/propagation queue

  /*
   * Clause database and related stuff
   * - cla_inc and inv_cla_decay are used for deletion of learned clauses
   * - unit clauses are stored implicitly in the assignment stack
   * - binary clauses are stored implicitly in the watch vectors
   * - all other clauses are in the pool
   */
  float cla_inc;              // Clause activity increment
  float inv_cla_decay;        // Inverse of clause decay (1/0.999)
  bool has_empty_clause;      // Whether the empty clause was added
  uint32_t units;             // Number of unit clauses
  uint32_t binaries;          // Number of binary clauses
  clause_pool_t pool;         // Pool for non-binary/non-unit clauses

  /*
   * Statistics record
   */
  solver_stats_t stats;

} sat_solver_t;


// Default size for the variable array
#define SAT_SOLVER_DEFAULT_VSIZE 1024


/********************
 *  MAIN FUNCTIONS  *
 *******************/

/*
 * Initialization:
 * - sz = initial size of the variable-indexed arrays.
 * - if sz is zero, the default size is used.
 * - the solver is initialized with one variable (the reserved variable 0).
 */
extern void init_nsat_solver(sat_solver_t *solver, uint32_t sz);

/*
 * Set the prng seed
 */
extern void nsat_solver_set_seed(sat_solver_t *solver, uint32_t seed);

/*
 * Deletion: free memory
 */
extern void delete_nsat_solver(sat_solver_t *solver);

/*
 * Reset: remove all variables and clauses
 */
extern void reset_nsat_solver(sat_solver_t *solver);


/*
 * Add n fresh variables:
 * - they are indexed from nv, ..., nv + n-1 where nv = number of 
 *   variables in solver (on entry to this function).
 */
extern void nsat_solver_add_vars(sat_solver_t *solver, uint32_t n);

/*
 * Allocate a fresh Boolean variable and return its index.
 */
extern bvar_t nsat_solver_new_var(sat_solver_t *solver);



/*********************
 *  CLAUSE ADDITION  *
 ********************/

/*
 * A clause is an array of literals (integers between 0 and nlits - 1)
 * - a clause is simplified if it satisfies the following conidtions:
 *   1) it doesn't contain assigned literals (including the reserved 
 *      literals 0 and 1)
 *   2) it doesn't include duplicates or complementary literals
 */

/*
 * The following function take a simplified clause as input
 */
extern void nsat_solver_add_empty_clause(sat_solver_t *solver);
extern void nsat_solver_add_unit_clause(sat_solver_t *solver, literal_t l1);
extern void nsat_solver_add_binary_clause(sat_solver_t *solver, literal_t l1, literal_t l2);
extern void nsat_solver_add_ternary_clause(sat_solver_t *solver, literal_t l1, literal_t l2, literal_t l3);

// n = size of the clause, l = array of n literals
extern void nsat_solver_add_clause(sat_solver_t *solver, uint32_t n, const literal_t *l);

/*
 * This function simplifies the clause then adds it
 * - n = number of literals
 * - l = array of n literals
 * - the array is modified
 */
extern void nsat_solver_simplify_and_add_clause(sat_solver_t *solver, uint32_t n, literal_t *l);



/**************************
 *  VARIABLE ASSIGNMENTS  *
 *************************/

static inline bval_t var_value(sat_solver_t *solver, bvar_t x) {
  assert(x < solver->nvars);
  return solver->value[x];
}

static inline bool var_is_unassigned(sat_solver_t *solver, bvar_t x) {
  return is_unassigned_val(var_value(solver, x));
}

static inline bool var_is_assigned(sat_solver_t *solver, bvar_t x) {
  return ! var_is_unassigned(solver, x);
}

static inline bool var_prefers_true(sat_solver_t *solver, bvar_t x) {
  return true_preferred(var_value(solver, x));
}

static inline bool var_is_true(sat_solver_t *solver, bvar_t x) {
  return var_value(solver, x) == BVAL_TRUE;
}

static inline bool var_is_false(sat_solver_t *solver, bvar_t x) {
  return var_value(solver, x) == BVAL_FALSE;
}


static inline bval_t lit_value(sat_solver_t *solver, literal_t l) {
  assert(l < solver->nliterals);
  return solver->value[var_of(l)] ^ sign_of(l);
}

static inline bool lit_is_unassigned(sat_solver_t *solver, literal_t l) {
  return is_unassigned_val(lit_value(solver, l));
}

static inline bool lit_is_assigned(sat_solver_t *solver, literal_t l) {
  return ! lit_is_unassigned(solver, l);
}

static inline bool lit_prefers_true(sat_solver_t *solver, literal_t l) {
  return true_preferred(lit_value(solver, l));
}

static inline bool lit_is_true(sat_solver_t *solver, literal_t l) {
  return lit_value(solver, l) == BVAL_TRUE;
}

static inline bool lit_is_false(sat_solver_t *solver, literal_t l) {
  return lit_value(solver, l) == BVAL_FALSE;
}



#endif /* __NEW_SAT_SOLVER_H */