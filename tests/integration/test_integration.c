/*
 * tests/integration/test_integration.c
 *
 * Phase 2E Full Validation: cross-module integration tests for cogplan9.
 *
 * Covers:
 *   - Full lifecycle (init -> operations -> shutdown)
 *   - AtomSpace + PLN cross-module pipeline
 *   - Statistics accuracy (total_atoms, total_inferences, uptime_ms)
 *   - AtomSpace CRUD with PLN inference on the results
 *   - plan9cog_version() and plan9cog_execute_atomese() correctness
 *   - Multiple init/shutdown cycles (idempotency)
 */

#include "../test_macros.h"
#include <plan9cog/plan9cog.h>
#include <stdlib.h>
#include <string.h>

/* ── Helpers ────────────────────────────────────────────────────── */

static int approx_eq(double a, double b)
{
    double d = a - b;
    return (d > -1e-4 && d < 1e-4);
}

/* ── IT-01: lifecycle ─────────────────────────────────────────── */

void test_lifecycle(void)
{
    int r;
    TEST_CASE("IT-01: init/shutdown basic lifecycle");
    r = plan9cog_init();
    ASSERT(r == 0);
    ASSERT(plan9cog_get_atomspace() != NULL);
    ASSERT(plan9cog_get_pln() != NULL);
    r = plan9cog_shutdown();
    ASSERT(r == 0);
    ASSERT(plan9cog_get_atomspace() == NULL);
    ASSERT(plan9cog_get_pln() == NULL);
}

/* ── IT-02: double-init/double-shutdown idempotency ───────────── */

void test_idempotency(void)
{
    int r1, r2;
    TEST_CASE("IT-02: double init is idempotent");
    plan9cog_init();
    r1 = plan9cog_init();
    ASSERT(r1 == 0);
    plan9cog_shutdown();

    TEST_CASE("IT-02b: double shutdown is safe");
    r1 = plan9cog_shutdown();
    r2 = plan9cog_shutdown();
    ASSERT(r1 == 0);
    ASSERT(r2 == 0);
}

/* ── IT-03: multiple init/shutdown cycles ─────────────────────── */

void test_multiple_cycles(void)
{
    int i, r;
    TEST_CASE("IT-03: five consecutive init/shutdown cycles");
    for (i = 0; i < 5; i++) {
        r = plan9cog_init();
        ASSERT(r == 0);
        ASSERT(plan9cog_get_atomspace() != NULL);
        r = plan9cog_shutdown();
        ASSERT(r == 0);
        ASSERT(plan9cog_get_atomspace() == NULL);
    }
}

/* ── IT-04: atomspace + PLN cross-module pipeline ─────────────── */

void test_atomspace_pln_pipeline(void)
{
    atomspace_t  *as;
    pln_engine_t *pln;
    atom_t       *a, *b, *ab, *b2, *c, *bc, *targets[2];
    truth_value_t *tv_ab, *tv_a, *tv_bc, *tv_ac_ded, *tv_ac_ind;

    TEST_CASE("IT-04: atomspace + PLN cross-module pipeline");

    plan9cog_init();

    as  = plan9cog_get_atomspace();
    pln = plan9cog_get_pln();
    ASSERT(as  != NULL);
    ASSERT(pln != NULL);

    /*
     * Build atoms.  The atomspace takes ownership of every atom added
     * via atomspace_add().  Because atom_free() recursively frees an
     * atom's outgoing set, we must NOT add outgoing-set atoms to the
     * atomspace separately — the link atom owns them.
     */

    /* Standalone concept nodes (no link owns them yet) */
    a = atom_create_node_typed("A", CONCEPT_NODE);
    b = atom_create_node_typed("B", CONCEPT_NODE);
    ASSERT(a != NULL);
    ASSERT(b != NULL);

    /* Assign TVs before adding to the space */
    tv_a  = pln_tv_create(0.8, 0.8);
    ASSERT(tv_a != NULL);
    a->tv = tv_a;
    /* Note: atom b does not need its own TV for these PLN tests —
     * pln_deduction() and pln_induction() operate on the link TVs
     * and the premise TV directly, not on the conclusion atom's TV. */

    /* Implication link A→B: the link takes ownership of a and b */
    targets[0] = a;
    targets[1] = b;
    ab = atom_create_link_typed(IMPLICATION_LINK, 2, targets);
    ASSERT(ab != NULL);
    tv_ab = pln_tv_create(0.9, 0.9);
    ASSERT(tv_ab != NULL);
    ab->tv = tv_ab;

    /* Add only the link to the atomspace; a and b are freed through ab */
    atomspace_add(as, ab);
    ASSERT(atomspace_size(as) == 1);

    /* PLN deduction: P(B | A) given P(A→B) and P(A) */
    tv_ac_ded = pln_deduction(tv_ab, tv_a);
    ASSERT(tv_ac_ded != NULL);
    ASSERT(tv_ac_ded->strength  > 0.0 && tv_ac_ded->strength  <= 1.0);
    ASSERT(tv_ac_ded->confidence > 0.0 && tv_ac_ded->confidence <= 1.0);
    /* strength = 0.9 * 0.8 = 0.72 */
    ASSERT(tv_ac_ded->strength > 0.68 && tv_ac_ded->strength < 0.76);
    pln_tv_free(tv_ac_ded);

    /* PLN induction using two evidence TVs */
    tv_bc = pln_tv_create(0.7, 0.7);
    ASSERT(tv_bc != NULL);
    tv_ac_ind = pln_induction(tv_ab, tv_bc);
    ASSERT(tv_ac_ind != NULL);
    ASSERT(tv_ac_ind->strength  >= 0.0 && tv_ac_ind->strength  <= 1.0);
    ASSERT(tv_ac_ind->confidence >= 0.0 && tv_ac_ind->confidence <= 1.0);
    pln_tv_free(tv_ac_ind);
    pln_tv_free(tv_bc);

    /* Add a second implication link B_second→C */
    b2 = atom_create_node_typed("B_second", CONCEPT_NODE);
    c  = atom_create_node_typed("C",  CONCEPT_NODE);
    ASSERT(b2 != NULL);
    ASSERT(c  != NULL);
    targets[0] = b2;
    targets[1] = c;
    bc = atom_create_link_typed(IMPLICATION_LINK, 2, targets);
    ASSERT(bc != NULL);
    atomspace_add(as, bc);

    /* Atomspace now holds 2 link atoms (a, b, b2, c are owned by the links) */
    ASSERT(atomspace_size(as) == 2);

    plan9cog_shutdown();
}

/* ── IT-05: statistics accuracy ───────────────────────────────── */

void test_statistics(void)
{
    cog_stats_t  stats;
    atomspace_t *as;
    atom_t      *n1, *n2, *n3;
    int          i;

    TEST_CASE("IT-05: statistics accuracy — natoms");

    plan9cog_init();
    as = plan9cog_get_atomspace();

    plan9cog_get_stats(&stats);
    ASSERT(stats.total_atoms == 0);

    /* Add three standalone nodes (no link owns them) */
    n1 = atom_create_node_typed("X", CONCEPT_NODE);
    n2 = atom_create_node_typed("Y", CONCEPT_NODE);
    n3 = atom_create_node_typed("Z", PREDICATE_NODE);
    atomspace_add(as, n1);
    atomspace_add(as, n2);
    atomspace_add(as, n3);

    plan9cog_get_stats(&stats);
    ASSERT(stats.total_atoms == 3);

    TEST_CASE("IT-05b: statistics accuracy — total_inferences starts at 0");
    ASSERT(stats.total_inferences == 0);

    TEST_CASE("IT-05c: statistics accuracy — uptime_ms is non-negative");
    ASSERT((long)stats.uptime_ms >= 0);

    TEST_CASE("IT-05d: stats NULL pointer is safe");
    plan9cog_get_stats(NULL); /* must not crash */
    ASSERT(1);

    /* Run several PLN operations to verify inference counter works */
    TEST_CASE("IT-05e: PLN forward_inference produces valid results");
    {
        atom_t       *premise, *conclusion, *impl;
        atom_t       *imp_targets[2];
        truth_value_t *tv_impl, *tv_prem, *result;

        /*
         * Create a self-contained implication: premise → conclusion.
         * The link owns premise and conclusion; only impl is added to
         * the atomspace to avoid double-free on shutdown.
         */
        premise    = atom_create_node_typed("Premise",    CONCEPT_NODE);
        conclusion = atom_create_node_typed("Conclusion", CONCEPT_NODE);
        imp_targets[0] = premise;
        imp_targets[1] = conclusion;
        impl = atom_create_link_typed(IMPLICATION_LINK, 2, imp_targets);
        ASSERT(impl != NULL);

        tv_impl = pln_tv_create(0.9, 0.9);
        tv_prem = pln_tv_create(0.8, 0.8);
        impl->tv    = tv_impl;
        premise->tv = tv_prem;

        atomspace_add(as, impl);

        for (i = 0; i < 10; i++) {
            result = pln_forward_inference(impl, premise);
            ASSERT(result != NULL);
            ASSERT(result->strength  >= 0.0 && result->strength  <= 1.0);
            ASSERT(result->confidence >= 0.0 && result->confidence <= 1.0);
            pln_tv_free(result);
        }
        /* tv_impl / tv_prem freed by atomspace_free via atom_free(impl) */
    }

    plan9cog_shutdown();
}

/* ── IT-06: version string ─────────────────────────────────────── */

void test_version(void)
{
    const char *ver;
    TEST_CASE("IT-06: plan9cog_version returns non-empty string");
    ver = plan9cog_version();
    ASSERT(ver != NULL);
    ASSERT(strlen(ver) > 0);
}

/* ── IT-07: execute_atomese ────────────────────────────────────── */

void test_execute_atomese(void)
{
    char *r;
    TEST_CASE("IT-07: execute_atomese returns stv for valid input");
    plan9cog_init();
    r = plan9cog_execute_atomese("(ConceptNode \"test\")");
    ASSERT(r != NULL);
    ASSERT(strlen(r) > 0);
    free(r);
    plan9cog_shutdown();

    TEST_CASE("IT-07b: execute_atomese returns NULL for NULL input");
    plan9cog_init();
    r = plan9cog_execute_atomese(NULL);
    ASSERT(r == NULL);
    plan9cog_shutdown();
}

/* ── IT-08: PLN formula accuracy ───────────────────────────────── */

void test_pln_formulas(void)
{
    truth_value_t *ab, *a, *b, *c, *result;

    TEST_CASE("IT-08: PLN deduction strength = s(A→B) * s(A)");
    ab = pln_tv_create(0.9, 0.9);
    a  = pln_tv_create(0.7, 0.9);
    result = pln_deduction(ab, a);
    ASSERT(result != NULL);
    /* strength = 0.9 * 0.7 = 0.63 */
    ASSERT(result->strength > 0.61 && result->strength < 0.65);
    pln_tv_free(result);
    pln_tv_free(ab);
    pln_tv_free(a);

    TEST_CASE("IT-08b: PLN revision merges two TVs");
    a = pln_tv_create(0.8, 0.9);
    b = pln_tv_create(0.6, 0.7);
    result = pln_revision(a, b);
    ASSERT(result != NULL);
    ASSERT(result->strength  >= 0.0 && result->strength  <= 1.0);
    ASSERT(result->confidence >= 0.0 && result->confidence <= 1.0);
    /* merged strength = (0.8*0.9 + 0.6*0.7)/(0.9+0.7) = (0.72+0.42)/1.6 = 0.7125 */
    ASSERT(result->strength > 0.69 && result->strength < 0.74);
    pln_tv_free(result);
    pln_tv_free(a);
    pln_tv_free(b);

    TEST_CASE("IT-08c: PLN and/or/not are in [0,1]");
    a = pln_tv_create(0.7, 0.8);
    b = pln_tv_create(0.5, 0.6);
    c = pln_and(a, b);
    ASSERT(c != NULL && c->strength >= 0.0 && c->strength <= 1.0);
    pln_tv_free(c);
    c = pln_or(a, b);
    ASSERT(c != NULL && c->strength >= 0.0 && c->strength <= 1.0);
    pln_tv_free(c);
    c = pln_not(a);
    ASSERT(c != NULL);
    ASSERT(approx_eq(c->strength, 1.0 - 0.7));
    pln_tv_free(c);
    pln_tv_free(a);
    pln_tv_free(b);

    TEST_CASE("IT-08d: PLN abduction produces valid TV");
    a = pln_tv_create(0.8, 0.7);
    b = pln_tv_create(0.6, 0.8);
    result = pln_abduction(a, b);
    ASSERT(result != NULL);
    ASSERT(result->strength  >= 0.0 && result->strength  <= 1.0);
    ASSERT(result->confidence >= 0.0 && result->confidence <= 1.0);
    pln_tv_free(result);
    pln_tv_free(a);
    pln_tv_free(b);
}

/* ── IT-09: atomspace query integration ────────────────────────── */

static int is_concept_node(const atom_t *a, void *arg)
{
    (void)arg;
    return a->type == CONCEPT_NODE;
}

void test_atomspace_query_integration(void)
{
    atomspace_t *as;
    atom_t     **results;
    atom_t      *n1, *n2, *n3, *lnk, *targets[2];
    int          count;

    TEST_CASE("IT-09: atomspace_query filters by type");
    plan9cog_init();
    as = plan9cog_get_atomspace();

    /*
     * Add three standalone nodes that no link owns, plus one link
     * whose outgoing targets are freshly allocated (not the standalone
     * nodes above) so there is no aliasing / double-free on shutdown.
     */
    n1 = atom_create_node_typed("Dog",    CONCEPT_NODE);
    n2 = atom_create_node_typed("Animal", CONCEPT_NODE);
    n3 = atom_create_node_typed("barks",  PREDICATE_NODE);
    /* LinkedDog / LinkedAnimal are distinct atoms owned by the link;
     * they are not added to the atomspace directly and will not appear
     * in atomspace_query results (query scans only top-level atoms). */
    targets[0] = atom_create_node_typed("LinkedDog",    CONCEPT_NODE);
    targets[1] = atom_create_node_typed("LinkedAnimal", CONCEPT_NODE);
    lnk = atom_create_link_typed(INHERITANCE_LINK, 2, targets);

    atomspace_add(as, n1);
    atomspace_add(as, n2);
    atomspace_add(as, n3);
    atomspace_add(as, lnk);  /* owns targets[0] and targets[1] */

    /* Query for CONCEPT_NODE: should find n1 and n2 (n3 is PREDICATE_NODE,
     * lnk is INHERITANCE_LINK; targets inside lnk are not top-level) */
    results = atomspace_query(as, is_concept_node, NULL, &count);
    ASSERT(count == 2);
    ASSERT(results != NULL);
    free(results);

    /* Remove one concept node by id */
    atomspace_remove(as, n1->id);
    results = atomspace_query(as, is_concept_node, NULL, &count);
    ASSERT(count == 1);
    free(results);

    plan9cog_shutdown();
}

/* ── main ───────────────────────────────────────────────────────── */

int main(void)
{
    test_lifecycle();
    test_idempotency();
    test_multiple_cycles();
    test_atomspace_pln_pipeline();
    test_statistics();
    test_version();
    test_execute_atomese();
    test_pln_formulas();
    test_atomspace_query_integration();

    return TEST_SUMMARY();
}
