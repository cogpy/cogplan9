/*
 * plan9cog/pln.h — Probabilistic Logic Networks (PLN) portable C API
 *
 * Uncertain inference and reasoning over AtomSpace hypergraphs.
 */

#ifndef PLAN9COG_PLN_H
#define PLAN9COG_PLN_H

#include <plan9cog/atomspace.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── PLN formula types ──────────────────────────────────────────── */
typedef enum pln_formula {
    PLN_DEDUCTION   = 1,
    PLN_INDUCTION   = 2,
    PLN_ABDUCTION   = 3,
    PLN_REVISION    = 4,
    PLN_MODUS       = 5,
    PLN_AND         = 6,
    PLN_OR          = 7,
    PLN_NOT         = 8,
    PLN_INHERITANCE = 9,
    PLN_SIMILARITY  = 10
} pln_formula_t;

/* ── PLN engine ─────────────────────────────────────────────────── */
typedef struct pln_engine {
    atomspace_t *as;
    float        min_confidence;
    int          max_steps;
    unsigned long inferences;
    unsigned long rule_matches;
} pln_engine_t;

/* ── Truth value constructors ───────────────────────────────────── */

/* Allocate a truth value on the heap */
truth_value_t *pln_tv_create(double strength, double confidence);

/* Free a heap-allocated truth value */
void pln_tv_free(truth_value_t *tv);

/* ── PLN formulas ───────────────────────────────────────────────── */

/* Deduction: A → B, A ⊢ B  (strength = s(A→B)*s(A)) */
truth_value_t *pln_deduction(const truth_value_t *ab, const truth_value_t *a);

/* Induction: A → B, A → C ⊢ B → C */
truth_value_t *pln_induction(const truth_value_t *ab, const truth_value_t *ac);

/* Abduction: A → C, B → C ⊢ A → B */
truth_value_t *pln_abduction(const truth_value_t *ac, const truth_value_t *bc);

/* Revision: combine two independent evidence TVs */
truth_value_t *pln_revision(const truth_value_t *a, const truth_value_t *b);

/* And: P(A ∧ B) */
truth_value_t *pln_and(const truth_value_t *a, const truth_value_t *b);

/* Or: P(A ∨ B) */
truth_value_t *pln_or(const truth_value_t *a, const truth_value_t *b);

/* Not: P(¬A) */
truth_value_t *pln_not(const truth_value_t *a);

/* ── High-level inference ───────────────────────────────────────── */

/*
 * Forward modus ponens over an ImplicationLink atom.
 * implication must be a 2-outgoing link; premise is its first target.
 * Returns a newly allocated truth_value_t for the conclusion,
 * or NULL on error.
 */
truth_value_t *pln_forward_inference(const atom_t *implication,
                                     const atom_t *premise);

/* ── Engine lifecycle ───────────────────────────────────────────── */
pln_engine_t *pln_engine_create(atomspace_t *as);
void          pln_engine_free(pln_engine_t *engine);

/* Run forward chaining up to max_steps; returns inferred atoms */
atom_t **pln_forward_chain(pln_engine_t *engine, atom_t *target,
                           int max_steps, int *count_out);

/* Run backward chaining; returns atoms supporting goal */
atom_t **pln_backward_chain(pln_engine_t *engine, atom_t *goal,
                            int max_steps, int *count_out);

/* ── PLN statistics ─────────────────────────────────────────────── */
typedef struct pln_stats {
    unsigned long inferences;
    unsigned long forward_steps;
    unsigned long backward_steps;
    unsigned long rule_matches;
    unsigned long tv_computations;
} pln_stats_t;

void pln_get_stats(const pln_engine_t *engine, pln_stats_t *out);
void pln_reset_stats(pln_engine_t *engine);

#ifdef __cplusplus
}
#endif

#endif /* PLAN9COG_PLN_H */
