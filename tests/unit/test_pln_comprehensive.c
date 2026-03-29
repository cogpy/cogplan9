/*
 * tests/unit/test_pln_comprehensive.c
 *
 * Exhaustive unit tests for the PLN (Probabilistic Logic Networks) API:
 *   - All formula types (deduction, induction, abduction, revision,
 *     and, or, not)
 *   - Boundary and extremal truth values (0, 1, near-0, near-1)
 *   - NULL safety for all API functions
 *   - forward inference chaining
 *   - Engine lifecycle
 *   - Statistics reset and accumulation
 *   - Numerical accuracy checks
 */

#include "../test_macros.h"
#include <plan9cog/pln.h>
#include <stdlib.h>
#include <math.h>

/* Tolerance for double comparisons */
#define EPS 1e-9

/* ── Helpers ─────────────────────────────────────────────────────── */
static int approx_eq(double a, double b) { return fabs(a - b) < EPS; }
static int in_range(double v, double lo, double hi) { return v >= lo && v <= hi; }

/* ── pln_tv_create / pln_tv_free ─────────────────────────────────── */

void test_tv_create_basic(void) {
    truth_value_t *tv;
    TEST_CASE("pln_tv_create: basic values");
    tv = pln_tv_create(0.7, 0.3);
    ASSERT(tv != NULL);
    ASSERT(approx_eq(tv->strength, 0.7));
    ASSERT(approx_eq(tv->confidence, 0.3));
    pln_tv_free(tv);
}

void test_tv_create_zero(void) {
    truth_value_t *tv;
    TEST_CASE("pln_tv_create: (0.0, 0.0)");
    tv = pln_tv_create(0.0, 0.0);
    ASSERT(tv != NULL);
    ASSERT(approx_eq(tv->strength, 0.0));
    ASSERT(approx_eq(tv->confidence, 0.0));
    pln_tv_free(tv);
}

void test_tv_create_one(void) {
    truth_value_t *tv;
    TEST_CASE("pln_tv_create: (1.0, 1.0)");
    tv = pln_tv_create(1.0, 1.0);
    ASSERT(tv != NULL);
    ASSERT(approx_eq(tv->strength, 1.0));
    ASSERT(approx_eq(tv->confidence, 1.0));
    pln_tv_free(tv);
}

void test_tv_create_clamps_negative(void) {
    truth_value_t *tv;
    TEST_CASE("pln_tv_create: negative values clamped to 0");
    tv = pln_tv_create(-0.5, -1.0);
    ASSERT(tv != NULL);
    ASSERT(approx_eq(tv->strength, 0.0));
    ASSERT(approx_eq(tv->confidence, 0.0));
    pln_tv_free(tv);
}

void test_tv_create_clamps_overflow(void) {
    truth_value_t *tv;
    TEST_CASE("pln_tv_create: values > 1 clamped to 1");
    tv = pln_tv_create(1.5, 2.0);
    ASSERT(tv != NULL);
    ASSERT(approx_eq(tv->strength, 1.0));
    ASSERT(approx_eq(tv->confidence, 1.0));
    pln_tv_free(tv);
}

void test_tv_free_null(void) {
    TEST_CASE("pln_tv_free: NULL is safe");
    pln_tv_free(NULL);
    ASSERT(1);
}

/* ── pln_deduction ───────────────────────────────────────────────── */

void test_deduction_basic(void) {
    truth_value_t ab, a, *result;
    TEST_CASE("pln_deduction: standard case (0.7,0.9) x (0.9,0.9)");
    ab.strength = 0.7;  ab.confidence = 0.9;
    a.strength  = 0.9;  a.confidence  = 0.9;
    result = pln_deduction(&ab, &a);
    ASSERT(result != NULL);
    /* strength = 0.7*0.9 = 0.63 */
    ASSERT(result->strength > 0.6 && result->strength < 0.7);
    /* confidence = 0.9*0.9 = 0.81 */
    ASSERT(result->confidence > 0.8 && result->confidence < 0.9);
    pln_tv_free(result);
}

void test_deduction_zero_premise(void) {
    truth_value_t ab, a, *result;
    TEST_CASE("pln_deduction: zero strength premise");
    ab.strength = 0.8;  ab.confidence = 0.9;
    a.strength  = 0.0;  a.confidence  = 0.9;
    result = pln_deduction(&ab, &a);
    ASSERT(result != NULL);
    ASSERT(approx_eq(result->strength, 0.0));
    pln_tv_free(result);
}

void test_deduction_certain_truth(void) {
    truth_value_t ab, a, *result;
    TEST_CASE("pln_deduction: all certainties = 1.0");
    ab.strength = 1.0;  ab.confidence = 1.0;
    a.strength  = 1.0;  a.confidence  = 1.0;
    result = pln_deduction(&ab, &a);
    ASSERT(result != NULL);
    ASSERT(approx_eq(result->strength, 1.0));
    ASSERT(approx_eq(result->confidence, 1.0));
    pln_tv_free(result);
}

void test_deduction_null_ab(void) {
    truth_value_t a;
    truth_value_t *result;
    TEST_CASE("pln_deduction: NULL ab returns NULL");
    a.strength = 0.5; a.confidence = 0.5;
    result = pln_deduction(NULL, &a);
    ASSERT(result == NULL);
}

void test_deduction_null_a(void) {
    truth_value_t ab;
    truth_value_t *result;
    TEST_CASE("pln_deduction: NULL a returns NULL");
    ab.strength = 0.5; ab.confidence = 0.5;
    result = pln_deduction(&ab, NULL);
    ASSERT(result == NULL);
}

void test_deduction_both_null(void) {
    truth_value_t *result;
    TEST_CASE("pln_deduction: both NULL returns NULL");
    result = pln_deduction(NULL, NULL);
    ASSERT(result == NULL);
}

/* ── pln_induction ───────────────────────────────────────────────── */

void test_induction_basic(void) {
    truth_value_t ab, ac, *result;
    TEST_CASE("pln_induction: basic case");
    ab.strength = 0.6;  ab.confidence = 0.8;
    ac.strength = 0.9;  ac.confidence = 0.7;
    result = pln_induction(&ab, &ac);
    ASSERT(result != NULL);
    ASSERT(in_range(result->strength,   0.0, 1.0));
    ASSERT(in_range(result->confidence, 0.0, 1.0));
    pln_tv_free(result);
}

void test_induction_null_args(void) {
    truth_value_t ab;
    truth_value_t *result;
    TEST_CASE("pln_induction: NULL args return NULL");
    ab.strength = 0.5; ab.confidence = 0.5;
    result = pln_induction(NULL, &ab);
    ASSERT(result == NULL);
    result = pln_induction(&ab, NULL);
    ASSERT(result == NULL);
}

/* ── pln_abduction ───────────────────────────────────────────────── */

void test_abduction_basic(void) {
    truth_value_t ac, bc, *result;
    TEST_CASE("pln_abduction: basic case");
    ac.strength = 0.8;  ac.confidence = 0.9;
    bc.strength = 0.4;  bc.confidence = 0.8;
    result = pln_abduction(&ac, &bc);
    ASSERT(result != NULL);
    ASSERT(in_range(result->strength,   0.0, 1.0));
    ASSERT(in_range(result->confidence, 0.0, 1.0));
    pln_tv_free(result);
}

void test_abduction_null_args(void) {
    truth_value_t ac;
    truth_value_t *result;
    TEST_CASE("pln_abduction: NULL args return NULL");
    ac.strength = 0.5; ac.confidence = 0.5;
    result = pln_abduction(NULL, &ac);
    ASSERT(result == NULL);
    result = pln_abduction(&ac, NULL);
    ASSERT(result == NULL);
}

/* ── pln_revision ─────────────────────────────────────────────────── */

void test_revision_basic(void) {
    truth_value_t a, b, *result;
    TEST_CASE("pln_revision: combines evidence — confidence increases");
    a.strength = 0.9;  a.confidence = 0.7;
    b.strength = 0.3;  b.confidence = 0.7;
    result = pln_revision(&a, &b);
    ASSERT(result != NULL);
    /* Combined should be between 0.3 and 0.9 */
    ASSERT(in_range(result->strength, 0.3, 0.9));
    /* Confidence should increase when combining independent evidence */
    /* c_out = c_a + c_b - c_a*c_b = 0.7+0.7-0.49 = 0.91 */
    ASSERT(result->confidence > 0.7);
    ASSERT(result->confidence <= 1.0);
    pln_tv_free(result);
}

void test_revision_idempotent(void) {
    truth_value_t a, *result;
    TEST_CASE("pln_revision: same evidence stays unchanged (approx)");
    a.strength = 0.7;  a.confidence = 0.5;
    result = pln_revision(&a, &a);
    ASSERT(result != NULL);
    ASSERT(approx_eq(result->strength, 0.7));
    pln_tv_free(result);
}

void test_revision_null_args(void) {
    truth_value_t a;
    truth_value_t *result;
    TEST_CASE("pln_revision: NULL args return NULL");
    a.strength = 0.5; a.confidence = 0.5;
    result = pln_revision(NULL, &a);
    ASSERT(result == NULL);
    result = pln_revision(&a, NULL);
    ASSERT(result == NULL);
}

/* ── pln_and / pln_or / pln_not ──────────────────────────────────── */

void test_pln_and_basic(void) {
    truth_value_t a, b, *result;
    TEST_CASE("pln_and: P(A∧B) = P(A)*P(B)");
    a.strength = 0.6;  a.confidence = 0.9;
    b.strength = 0.5;  b.confidence = 0.8;
    result = pln_and(&a, &b);
    ASSERT(result != NULL);
    ASSERT(approx_eq(result->strength, 0.3));
    pln_tv_free(result);
}

void test_pln_and_with_zero(void) {
    truth_value_t a, b, *result;
    TEST_CASE("pln_and: P(A∧False) = 0");
    a.strength = 0.9;  a.confidence = 1.0;
    b.strength = 0.0;  b.confidence = 1.0;
    result = pln_and(&a, &b);
    ASSERT(result != NULL);
    ASSERT(approx_eq(result->strength, 0.0));
    pln_tv_free(result);
}

void test_pln_and_null_args(void) {
    truth_value_t a;
    truth_value_t *result;
    TEST_CASE("pln_and: NULL args return NULL");
    a.strength = 0.5; a.confidence = 0.5;
    result = pln_and(NULL, &a);
    ASSERT(result == NULL);
    result = pln_and(&a, NULL);
    ASSERT(result == NULL);
}

void test_pln_or_basic(void) {
    truth_value_t a, b, *result;
    TEST_CASE("pln_or: P(A∨B) = P(A)+P(B)-P(A)*P(B)");
    a.strength = 0.3;  a.confidence = 0.9;
    b.strength = 0.4;  b.confidence = 0.8;
    result = pln_or(&a, &b);
    ASSERT(result != NULL);
    /* 0.3 + 0.4 - 0.12 = 0.58 */
    ASSERT(approx_eq(result->strength, 0.58));
    pln_tv_free(result);
}

void test_pln_or_with_one(void) {
    truth_value_t a, b, *result;
    TEST_CASE("pln_or: P(A∨True) = 1");
    a.strength = 0.3;  a.confidence = 0.9;
    b.strength = 1.0;  b.confidence = 1.0;
    result = pln_or(&a, &b);
    ASSERT(result != NULL);
    ASSERT(approx_eq(result->strength, 1.0));
    pln_tv_free(result);
}

void test_pln_or_null_args(void) {
    truth_value_t a;
    truth_value_t *result;
    TEST_CASE("pln_or: NULL args return NULL");
    a.strength = 0.5; a.confidence = 0.5;
    result = pln_or(NULL, &a);
    ASSERT(result == NULL);
    result = pln_or(&a, NULL);
    ASSERT(result == NULL);
}

void test_pln_not_basic(void) {
    truth_value_t a, *result;
    TEST_CASE("pln_not: P(¬A) = 1 - P(A)");
    a.strength = 0.3;  a.confidence = 0.8;
    result = pln_not(&a);
    ASSERT(result != NULL);
    ASSERT(approx_eq(result->strength, 0.7));
    ASSERT(approx_eq(result->confidence, 0.8));
    pln_tv_free(result);
}

void test_pln_not_double_negation(void) {
    truth_value_t a, *neg, *dbl;
    TEST_CASE("pln_not: double negation P(¬¬A) ≈ P(A)");
    a.strength = 0.4;  a.confidence = 0.9;
    neg = pln_not(&a);
    ASSERT(neg != NULL);
    dbl = pln_not(neg);
    ASSERT(dbl != NULL);
    ASSERT(approx_eq(dbl->strength, 0.4));
    pln_tv_free(neg);
    pln_tv_free(dbl);
}

void test_pln_not_null(void) {
    truth_value_t *result;
    TEST_CASE("pln_not: NULL returns NULL");
    result = pln_not(NULL);
    ASSERT(result == NULL);
}

/* ── pln_forward_inference ───────────────────────────────────────── */

void test_forward_inference_basic(void) {
    atom_t *a, *b, *targets[2], *imp;
    truth_value_t *tv_a, *tv_imp, *result;
    TEST_CASE("pln_forward_inference: standard modus ponens");
    a = atom_create_node("A");
    b = atom_create_node("B");
    targets[0] = a; targets[1] = b;
    imp = atom_create_link("Imp", 2, targets);

    tv_a        = pln_tv_create(0.9, 0.9);
    tv_imp      = pln_tv_create(0.7, 0.9);
    a->tv       = tv_a;
    imp->tv     = tv_imp;

    result = pln_forward_inference(imp, a);
    ASSERT(result != NULL);
    ASSERT(result->strength   > 0.6 && result->strength   < 0.7);
    ASSERT(result->confidence > 0.8 && result->confidence < 0.9);
    pln_tv_free(result);
    atom_free(imp);
}

void test_forward_inference_null_imp(void) {
    atom_t *a;
    truth_value_t *tv_a, *result;
    TEST_CASE("pln_forward_inference: NULL implication returns NULL");
    a    = atom_create_node("A");
    tv_a = pln_tv_create(0.9, 0.9);
    a->tv = tv_a;
    result = pln_forward_inference(NULL, a);
    ASSERT(result == NULL);
    atom_free(a);
}

void test_forward_inference_null_premise(void) {
    atom_t *a, *b, *targets[2], *imp;
    truth_value_t *tv_imp, *result;
    TEST_CASE("pln_forward_inference: NULL premise returns NULL");
    a = atom_create_node("A");
    b = atom_create_node("B");
    targets[0] = a; targets[1] = b;
    imp = atom_create_link("Imp", 2, targets);
    tv_imp  = pln_tv_create(0.7, 0.9);
    imp->tv = tv_imp;
    result  = pln_forward_inference(imp, NULL);
    ASSERT(result == NULL);
    atom_free(imp);
}

void test_forward_inference_no_tv(void) {
    atom_t *a, *b, *targets[2], *imp;
    truth_value_t *result;
    TEST_CASE("pln_forward_inference: missing TV returns NULL");
    a = atom_create_node("A");  /* no tv set */
    b = atom_create_node("B");
    targets[0] = a; targets[1] = b;
    imp = atom_create_link("Imp", 2, targets);
    /* no tv on imp or a */
    result = pln_forward_inference(imp, a);
    ASSERT(result == NULL);
    atom_free(imp);
}

void test_forward_inference_certain(void) {
    atom_t *a, *b, *targets[2], *imp;
    truth_value_t *tv_a, *tv_imp, *result;
    TEST_CASE("pln_forward_inference: certainty = 1.0 propagates");
    a = atom_create_node("A");
    b = atom_create_node("B");
    targets[0] = a; targets[1] = b;
    imp = atom_create_link("Imp", 2, targets);
    tv_a   = pln_tv_create(1.0, 1.0);
    tv_imp = pln_tv_create(1.0, 1.0);
    a->tv  = tv_a;
    imp->tv = tv_imp;
    result = pln_forward_inference(imp, a);
    ASSERT(result != NULL);
    ASSERT(approx_eq(result->strength, 1.0));
    ASSERT(approx_eq(result->confidence, 1.0));
    pln_tv_free(result);
    atom_free(imp);
}

/* ── Engine lifecycle ────────────────────────────────────────────── */

void test_engine_create_free(void) {
    atomspace_t *as;
    pln_engine_t *engine;
    TEST_CASE("pln_engine_create / pln_engine_free");
    as     = atomspace_create();
    engine = pln_engine_create(as);
    ASSERT(engine != NULL);
    ASSERT(engine->as == as);
    pln_engine_free(engine);
    atomspace_free(as);
}

void test_engine_free_null(void) {
    TEST_CASE("pln_engine_free: NULL is safe");
    pln_engine_free(NULL);
    ASSERT(1);
}

void test_engine_stats(void) {
    atomspace_t *as;
    pln_engine_t *engine;
    pln_stats_t stats;
    TEST_CASE("pln_get_stats: initial stats are zero");
    as     = atomspace_create();
    engine = pln_engine_create(as);
    pln_get_stats(engine, &stats);
    ASSERT(stats.inferences   == 0);
    ASSERT(stats.rule_matches == 0);
    pln_engine_free(engine);
    atomspace_free(as);
}

void test_engine_reset_stats(void) {
    atomspace_t *as;
    pln_engine_t *engine;
    pln_stats_t stats;
    TEST_CASE("pln_reset_stats: resets counters to zero");
    as     = atomspace_create();
    engine = pln_engine_create(as);
    engine->inferences  = 42;
    engine->rule_matches = 7;
    pln_reset_stats(engine);
    pln_get_stats(engine, &stats);
    ASSERT(stats.inferences   == 0);
    ASSERT(stats.rule_matches == 0);
    pln_engine_free(engine);
    atomspace_free(as);
}

void test_engine_chain_stubs(void) {
    atomspace_t *as;
    pln_engine_t *engine;
    atom_t *target;
    atom_t **results;
    int count;
    TEST_CASE("pln_forward_chain / pln_backward_chain: stub returns empty");
    as     = atomspace_create();
    engine = pln_engine_create(as);
    target = atom_create_node("goal");
    results = pln_forward_chain(engine, target, 10, &count);
    ASSERT(count   == 0);
    ASSERT(results == NULL);
    results = pln_backward_chain(engine, target, 10, &count);
    ASSERT(count   == 0);
    ASSERT(results == NULL);
    atom_free(target);
    pln_engine_free(engine);
    atomspace_free(as);
}

/* ── Numerical corner cases ──────────────────────────────────────── */

void test_deduction_near_zero(void) {
    truth_value_t ab, a, *result;
    TEST_CASE("pln_deduction: near-zero strengths");
    ab.strength = 1e-10;  ab.confidence = 0.5;
    a.strength  = 1e-10;  a.confidence  = 0.5;
    result = pln_deduction(&ab, &a);
    ASSERT(result != NULL);
    ASSERT(result->strength >= 0.0);
    ASSERT(result->strength <= 1.0);
    pln_tv_free(result);
}

void test_revision_zero_confidence(void) {
    truth_value_t a, b, *result;
    TEST_CASE("pln_revision: zero confidence evidence");
    a.strength = 0.8;  a.confidence = 0.0;
    b.strength = 0.2;  b.confidence = 0.0;
    result = pln_revision(&a, &b);
    ASSERT(result != NULL);
    ASSERT(in_range(result->strength,   0.0, 1.0));
    ASSERT(in_range(result->confidence, 0.0, 1.0));
    pln_tv_free(result);
}

/* ── main ────────────────────────────────────────────────────────── */

int main(void) {
    /* TV creation */
    test_tv_create_basic();
    test_tv_create_zero();
    test_tv_create_one();
    test_tv_create_clamps_negative();
    test_tv_create_clamps_overflow();
    test_tv_free_null();

    /* Deduction */
    test_deduction_basic();
    test_deduction_zero_premise();
    test_deduction_certain_truth();
    test_deduction_null_ab();
    test_deduction_null_a();
    test_deduction_both_null();

    /* Induction */
    test_induction_basic();
    test_induction_null_args();

    /* Abduction */
    test_abduction_basic();
    test_abduction_null_args();

    /* Revision */
    test_revision_basic();
    test_revision_idempotent();
    test_revision_null_args();

    /* And / Or / Not */
    test_pln_and_basic();
    test_pln_and_with_zero();
    test_pln_and_null_args();
    test_pln_or_basic();
    test_pln_or_with_one();
    test_pln_or_null_args();
    test_pln_not_basic();
    test_pln_not_double_negation();
    test_pln_not_null();

    /* Forward inference */
    test_forward_inference_basic();
    test_forward_inference_null_imp();
    test_forward_inference_null_premise();
    test_forward_inference_no_tv();
    test_forward_inference_certain();

    /* Engine */
    test_engine_create_free();
    test_engine_free_null();
    test_engine_stats();
    test_engine_reset_stats();
    test_engine_chain_stubs();

    /* Numerical corners */
    test_deduction_near_zero();
    test_revision_zero_confidence();

    return TEST_SUMMARY();
}
