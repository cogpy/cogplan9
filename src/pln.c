/*
 * src/pln.c — Probabilistic Logic Networks portable C implementation
 */

#include <plan9cog/pln.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Clamp a double to [0, 1] */
static double clamp01(double v)
{
    if (v < 0.0) return 0.0;
    if (v > 1.0) return 1.0;
    return v;
}

/* ── Truth value constructors ───────────────────────────────────── */

truth_value_t *pln_tv_create(double strength, double confidence)
{
    return tv_create(clamp01(strength), clamp01(confidence));
}

void pln_tv_free(truth_value_t *tv)
{
    tv_free(tv);
}

/* ── PLN formulas ───────────────────────────────────────────────── */

/*
 * Deduction formula (simplified):
 *   strength(B)    = s(A→B) * s(A)
 *   confidence(B)  = c(A)   * c(A→B)
 */
truth_value_t *pln_deduction(const truth_value_t *ab, const truth_value_t *a)
{
    double s, c;
    if (!ab || !a) return NULL;
    s = clamp01(ab->strength * a->strength);
    c = clamp01(ab->confidence * a->confidence);
    return pln_tv_create(s, c);
}

/*
 * Induction formula:
 *   strength = s(AC) / (s(AB) + 1e-9)   clamped to [0,1]
 *   confidence = min(c_ab, c_ac) * 0.9
 */
truth_value_t *pln_induction(const truth_value_t *ab, const truth_value_t *ac)
{
    double s, c, cmin;
    if (!ab || !ac) return NULL;
    s    = clamp01(ac->strength / (ab->strength + 1e-9));
    cmin = ab->confidence < ac->confidence ? ab->confidence : ac->confidence;
    c    = clamp01(cmin * 0.9);
    return pln_tv_create(s, c);
}

/*
 * Abduction formula:
 *   strength = s(AC) / (s(BC) + 1e-9)   clamped to [0,1]
 *   confidence = min(c_ac, c_bc) * 0.9
 */
truth_value_t *pln_abduction(const truth_value_t *ac, const truth_value_t *bc)
{
    double s, c, cmin;
    if (!ac || !bc) return NULL;
    s    = clamp01(ac->strength / (bc->strength + 1e-9));
    cmin = ac->confidence < bc->confidence ? ac->confidence : bc->confidence;
    c    = clamp01(cmin * 0.9);
    return pln_tv_create(s, c);
}

/*
 * Revision formula — combine two independent evidence sources.
 * Uses the OR-combination for confidence (maximally compatible with
 * Bayesian update): c_out = c_a + c_b - c_a*c_b
 * Weighted average for strength:
 *   s_out = (s_a*c_a + s_b*c_b) / (c_a + c_b + 1e-9)
 */
truth_value_t *pln_revision(const truth_value_t *a, const truth_value_t *b)
{
    double s, c, denom;
    if (!a || !b) return NULL;
    denom = a->confidence + b->confidence + 1e-9;
    s     = clamp01((a->strength * a->confidence + b->strength * b->confidence) / denom);
    /* confidence increases when combining independent evidence */
    c     = clamp01(a->confidence + b->confidence - a->confidence * b->confidence);
    return pln_tv_create(s, c);
}

/* P(A ∧ B) = P(A) * P(B) (assuming independence) */
truth_value_t *pln_and(const truth_value_t *a, const truth_value_t *b)
{
    double cmin;
    if (!a || !b) return NULL;
    cmin = a->confidence < b->confidence ? a->confidence : b->confidence;
    return pln_tv_create(a->strength * b->strength, cmin);
}

/* P(A ∨ B) = P(A) + P(B) - P(A)*P(B) */
truth_value_t *pln_or(const truth_value_t *a, const truth_value_t *b)
{
    double cmin;
    if (!a || !b) return NULL;
    cmin = a->confidence < b->confidence ? a->confidence : b->confidence;
    return pln_tv_create(
        clamp01(a->strength + b->strength - a->strength * b->strength),
        cmin);
}

/* P(¬A) = 1 - P(A) */
truth_value_t *pln_not(const truth_value_t *a)
{
    if (!a) return NULL;
    return pln_tv_create(1.0 - a->strength, a->confidence);
}

/* ── High-level inference ───────────────────────────────────────── */

truth_value_t *pln_forward_inference(const atom_t *implication,
                                     const atom_t *premise)
{
    const truth_value_t *tv_imp;
    const truth_value_t *tv_pre;
    if (!implication || !premise) return NULL;
    tv_imp = implication->tv;
    tv_pre = premise->tv;
    if (!tv_imp || !tv_pre) return NULL;
    return pln_deduction(tv_imp, tv_pre);
}

/* ── Engine lifecycle ───────────────────────────────────────────── */

pln_engine_t *pln_engine_create(atomspace_t *as)
{
    pln_engine_t *e = (pln_engine_t *)calloc(1, sizeof(pln_engine_t));
    if (!e) return NULL;
    e->as             = as;
    e->min_confidence = 0.01f;
    e->max_steps      = 100;
    return e;
}

void pln_engine_free(pln_engine_t *engine)
{
    free(engine);
}

atom_t **pln_forward_chain(pln_engine_t *engine, atom_t *target,
                           int max_steps, int *count_out)
{
    /* Stub implementation: no results yet */
    (void)engine; (void)target; (void)max_steps;
    if (count_out) *count_out = 0;
    return NULL;
}

atom_t **pln_backward_chain(pln_engine_t *engine, atom_t *goal,
                            int max_steps, int *count_out)
{
    (void)engine; (void)goal; (void)max_steps;
    if (count_out) *count_out = 0;
    return NULL;
}

/* ── Statistics ─────────────────────────────────────────────────── */

void pln_get_stats(const pln_engine_t *engine, pln_stats_t *out)
{
    if (!engine || !out) return;
    out->inferences      = engine->inferences;
    out->forward_steps   = 0;
    out->backward_steps  = 0;
    out->rule_matches    = engine->rule_matches;
    out->tv_computations = 0;
}

void pln_reset_stats(pln_engine_t *engine)
{
    if (!engine) return;
    engine->inferences  = 0;
    engine->rule_matches = 0;
}
