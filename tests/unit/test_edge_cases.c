/*
 * tests/unit/test_edge_cases.c
 *
 * Edge-case and error-handling tests for the full plan9cog C API:
 *   - Double init/shutdown idempotency
 *   - plan9cog_execute_atomese with various inputs
 *   - NULL/empty arguments across every API surface
 *   - plan9cog_get_atomspace / plan9cog_get_pln before and after init
 *   - Version string validity
 *   - Statistics correctness after operations
 */

#include "../test_macros.h"
#include <plan9cog/plan9cog.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ── Lifecycle edge cases ────────────────────────────────────────── */

void test_double_init(void) {
    int r1, r2;
    TEST_CASE("plan9cog_init: double init is idempotent");
    plan9cog_shutdown(); /* ensure clean state */
    r1 = plan9cog_init();
    r2 = plan9cog_init();
    ASSERT(r1 == 0);
    ASSERT(r2 == 0); /* second call should be a no-op */
    plan9cog_shutdown();
}

void test_double_shutdown(void) {
    int r1, r2;
    TEST_CASE("plan9cog_shutdown: double shutdown is safe");
    plan9cog_init();
    r1 = plan9cog_shutdown();
    r2 = plan9cog_shutdown();
    ASSERT(r1 == 0);
    ASSERT(r2 == 0);
}

void test_shutdown_without_init(void) {
    int r;
    TEST_CASE("plan9cog_shutdown: without prior init is safe");
    plan9cog_shutdown(); /* ensure uninitialized */
    r = plan9cog_shutdown();
    ASSERT(r == 0);
}

void test_init_shutdown_cycle(void) {
    int i;
    TEST_CASE("plan9cog_init/shutdown: multiple cycles");
    for (i = 0; i < 5; i++) {
        ASSERT(plan9cog_init()     == 0);
        ASSERT(plan9cog_shutdown() == 0);
    }
}

/* ── Execute atomese ─────────────────────────────────────────────── */

void test_execute_atomese_basic(void) {
    char *result;
    TEST_CASE("plan9cog_execute_atomese: typical Atomese expression");
    plan9cog_init();
    result = plan9cog_execute_atomese(
        "(Evaluation (PredicateNode \"test\") (ListLink (ConceptNode \"arg\")))");
    ASSERT(result != NULL);
    ASSERT(strlen(result) > 0);
    free(result);
    plan9cog_shutdown();
}

void test_execute_atomese_returns_stv(void) {
    char *result;
    TEST_CASE("plan9cog_execute_atomese: returns stv string");
    plan9cog_init();
    result = plan9cog_execute_atomese("(ConceptNode \"x\")");
    ASSERT(result != NULL);
    ASSERT_STR_EQ(result, "(stv 1.0 1.0)");
    free(result);
    plan9cog_shutdown();
}

void test_execute_atomese_null(void) {
    char *result;
    TEST_CASE("plan9cog_execute_atomese: NULL input returns NULL");
    plan9cog_init();
    result = plan9cog_execute_atomese(NULL);
    ASSERT(result == NULL);
    plan9cog_shutdown();
}

void test_execute_atomese_empty(void) {
    char *result;
    TEST_CASE("plan9cog_execute_atomese: empty string input");
    plan9cog_init();
    result = plan9cog_execute_atomese("");
    ASSERT(result != NULL);
    free(result);
    plan9cog_shutdown();
}

void test_execute_atomese_long_input(void) {
    char *buf, *result;
    int len = 10000;
    TEST_CASE("plan9cog_execute_atomese: very long input");
    plan9cog_init();
    buf = (char *)malloc((size_t)(len + 1));
    ASSERT(buf != NULL);
    memset(buf, 'x', (size_t)len);
    buf[len] = '\0';
    result = plan9cog_execute_atomese(buf);
    ASSERT(result != NULL);
    free(result);
    free(buf);
    plan9cog_shutdown();
}

/* ── Accessor functions ──────────────────────────────────────────── */

void test_get_atomspace_before_init(void) {
    atomspace_t *as;
    TEST_CASE("plan9cog_get_atomspace: returns NULL before init");
    plan9cog_shutdown();
    as = plan9cog_get_atomspace();
    ASSERT(as == NULL);
}

void test_get_atomspace_after_init(void) {
    atomspace_t *as;
    TEST_CASE("plan9cog_get_atomspace: returns non-NULL after init");
    plan9cog_init();
    as = plan9cog_get_atomspace();
    ASSERT(as != NULL);
    plan9cog_shutdown();
}

void test_get_pln_before_init(void) {
    pln_engine_t *pln;
    TEST_CASE("plan9cog_get_pln: returns NULL before init");
    plan9cog_shutdown();
    pln = plan9cog_get_pln();
    ASSERT(pln == NULL);
}

void test_get_pln_after_init(void) {
    pln_engine_t *pln;
    TEST_CASE("plan9cog_get_pln: returns non-NULL after init");
    plan9cog_init();
    pln = plan9cog_get_pln();
    ASSERT(pln != NULL);
    ASSERT(pln->as != NULL);
    plan9cog_shutdown();
}

/* ── Version string ──────────────────────────────────────────────── */

void test_version_not_null(void) {
    const char *v;
    TEST_CASE("plan9cog_version: not NULL");
    v = plan9cog_version();
    ASSERT(v != NULL);
}

void test_version_non_empty(void) {
    const char *v;
    TEST_CASE("plan9cog_version: non-empty");
    v = plan9cog_version();
    ASSERT(strlen(v) > 0);
}

/* ── Statistics ──────────────────────────────────────────────────── */

void test_stats_after_init(void) {
    cog_stats_t stats;
    TEST_CASE("plan9cog_get_stats: valid after init");
    plan9cog_init();
    memset(&stats, 0xFF, sizeof(stats)); /* poison the struct */
    plan9cog_get_stats(&stats);
    ASSERT(stats.total_atoms == 0);
    ASSERT(stats.total_inferences == 0);
    plan9cog_shutdown();
}

void test_stats_null_out(void) {
    TEST_CASE("plan9cog_get_stats: NULL out is safe");
    plan9cog_init();
    plan9cog_get_stats(NULL); /* must not crash */
    ASSERT(1);
    plan9cog_shutdown();
}

void test_stats_before_init(void) {
    cog_stats_t stats;
    TEST_CASE("plan9cog_get_stats: before init produces zeros");
    plan9cog_shutdown();
    memset(&stats, 0xFF, sizeof(stats));
    plan9cog_get_stats(&stats);
    ASSERT(stats.total_atoms == 0);
    ASSERT(stats.uptime_ms   == 0);
}

/* ── AtomSpace through global API ────────────────────────────────── */

void test_global_atomspace_operations(void) {
    atomspace_t *as;
    atom_t *n;
    TEST_CASE("global atomspace: add and find atom via accessor");
    plan9cog_init();
    as = plan9cog_get_atomspace();
    ASSERT(as != NULL);
    n = atom_create_node("global_test");
    ASSERT(atomspace_add(as, n) == 0);
    ASSERT(atomspace_size(as) >= 1);
    ASSERT(atomspace_find(as, n->id) == n);
    plan9cog_shutdown(); /* frees atomspace and all atoms */
}

/* ── main ────────────────────────────────────────────────────────── */

int main(void) {
    /* Lifecycle */
    test_double_init();
    test_double_shutdown();
    test_shutdown_without_init();
    test_init_shutdown_cycle();

    /* Execute atomese */
    test_execute_atomese_basic();
    test_execute_atomese_returns_stv();
    test_execute_atomese_null();
    test_execute_atomese_empty();
    test_execute_atomese_long_input();

    /* Accessors */
    test_get_atomspace_before_init();
    test_get_atomspace_after_init();
    test_get_pln_before_init();
    test_get_pln_after_init();

    /* Version */
    test_version_not_null();
    test_version_non_empty();

    /* Statistics */
    test_stats_after_init();
    test_stats_null_out();
    test_stats_before_init();

    /* Global operations */
    test_global_atomspace_operations();

    return TEST_SUMMARY();
}
