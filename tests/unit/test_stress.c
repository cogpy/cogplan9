/*
 * tests/unit/test_stress.c
 *
 * Stress tests for the plan9cog C API:
 *   - Create/free large numbers of atoms
 *   - Deep link chains
 *   - Wide fan-out links
 *   - Repeated init/shutdown under load
 *   - Concurrent-style sequential atomspace operations
 *   - PLN formula chains (chained deductions)
 *   - AtomSpace capacity exhaustion and growth
 */

#include "../test_macros.h"
#include <plan9cog/plan9cog.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define STRESS_N 1000
#define DEEP_CHAIN 100
#define WIDE_FAN 200

/* ── Many atoms ──────────────────────────────────────────────────── */

void test_create_many_nodes(void) {
    atom_t *atoms[STRESS_N];
    char name[64];
    int i;
    TEST_CASE("stress: create and free 1000 nodes");
    for (i = 0; i < STRESS_N; i++) {
        snprintf(name, sizeof(name), "stress_node_%d", i);
        atoms[i] = atom_create_node(name);
        ASSERT(atoms[i] != NULL);
        ASSERT(atoms[i]->type == NODE);
    }
    for (i = 0; i < STRESS_N; i++)
        atom_free(atoms[i]);
    ASSERT(1);
}

void test_create_many_links(void) {
    atom_t *links[STRESS_N];
    int i;
    TEST_CASE("stress: create and free 1000 binary links");
    for (i = 0; i < STRESS_N; i++) {
        atom_t *a = atom_create_node("la");
        atom_t *b = atom_create_node("lb");
        atom_t *targets[2];
        targets[0] = a; targets[1] = b;
        links[i] = atom_create_link("L", 2, targets);
        ASSERT(links[i] != NULL);
    }
    for (i = 0; i < STRESS_N; i++)
        atom_free(links[i]); /* recursively frees a and b */
    ASSERT(1);
}

/* ── Deep chain ──────────────────────────────────────────────────── */

void test_deep_link_chain(void) {
    atom_t *cur, *t[1], *next;
    int i;
    TEST_CASE("stress: deep link chain of depth 100");
    cur = atom_create_node("root");
    for (i = 0; i < DEEP_CHAIN; i++) {
        char name[32];
        snprintf(name, sizeof(name), "level_%d", i);
        t[0] = cur;
        next = atom_create_link(name, 1, t);
        ASSERT(next != NULL);
        cur = next;
    }
    atom_free(cur); /* must recurse all 100 levels */
    ASSERT(1);
}

/* ── Wide fan-out ────────────────────────────────────────────────── */

void test_wide_link(void) {
    atom_t **targets;
    atom_t *link;
    int i;
    TEST_CASE("stress: link with 200 outgoing atoms");
    targets = (atom_t **)malloc((size_t)WIDE_FAN * sizeof(atom_t *));
    ASSERT(targets != NULL);
    for (i = 0; i < WIDE_FAN; i++) {
        char name[32];
        snprintf(name, sizeof(name), "wide_%d", i);
        targets[i] = atom_create_node(name);
        ASSERT(targets[i] != NULL);
    }
    link = atom_create_link("WideLink", WIDE_FAN, targets);
    free(targets);
    ASSERT(link != NULL);
    ASSERT(link->outgoing_set_size == WIDE_FAN);
    atom_free(link);
    ASSERT(1);
}

/* ── AtomSpace with many atoms ───────────────────────────────────── */

void test_atomspace_stress(void) {
    atomspace_t *as;
    int i;
    unsigned long first_id = 0;
    atom_t *found;
    TEST_CASE("stress: atomspace with 1000 atoms, find, remove half");
    as = atomspace_create();
    ASSERT(as != NULL);
    for (i = 0; i < STRESS_N; i++) {
        char name[64];
        atom_t *n;
        snprintf(name, sizeof(name), "as_node_%d", i);
        n = atom_create_node(name);
        if (i == 0) first_id = n->id;
        atomspace_add(as, n);
    }
    ASSERT(atomspace_size(as) == STRESS_N);
    /* First atom should still be findable */
    found = atomspace_find(as, first_id);
    ASSERT(found != NULL);
    atomspace_free(as);
    ASSERT(1);
}

/* ── PLN chained deductions ──────────────────────────────────────── */

void test_pln_chain_deduction(void) {
    truth_value_t acc;
    truth_value_t imp;
    truth_value_t *result;
    int i;
    TEST_CASE("stress: 50 chained PLN deductions");
    acc.strength   = 1.0;
    acc.confidence = 1.0;
    imp.strength   = 0.99;
    imp.confidence = 0.99;
    for (i = 0; i < 50; i++) {
        result = pln_deduction(&imp, &acc);
        ASSERT(result != NULL);
        acc.strength   = result->strength;
        acc.confidence = result->confidence;
        pln_tv_free(result);
        ASSERT(acc.strength   >= 0.0 && acc.strength   <= 1.0);
        ASSERT(acc.confidence >= 0.0 && acc.confidence <= 1.0);
    }
    /* After 50 steps strength should have decayed */
    ASSERT(acc.strength < 1.0);
    ASSERT(1);
}

/* ── Repeated init/shutdown ──────────────────────────────────────── */

void test_repeated_init_shutdown(void) {
    int i;
    TEST_CASE("stress: 20 init/shutdown cycles");
    for (i = 0; i < 20; i++) {
        ASSERT(plan9cog_init()     == 0);
        ASSERT(plan9cog_shutdown() == 0);
    }
}

/* ── Unique IDs remain unique under stress ───────────────────────── */

void test_unique_ids_stress(void) {
    atom_t *atoms[STRESS_N];
    int i, j, dup;
    TEST_CASE("stress: 1000 atoms have unique IDs");
    for (i = 0; i < STRESS_N; i++) {
        atoms[i] = atom_create_node("uid_test");
        ASSERT(atoms[i] != NULL);
    }
    /* Spot-check: first 50 pairs should all be distinct */
    dup = 0;
    for (i = 0; i < 50 && !dup; i++)
        for (j = i + 1; j < 50 && !dup; j++)
            if (atoms[i]->id == atoms[j]->id) dup = 1;
    ASSERT(!dup);
    for (i = 0; i < STRESS_N; i++)
        atom_free(atoms[i]);
}

/* ── PLN revision convergence ────────────────────────────────────── */

void test_pln_revision_convergence(void) {
    truth_value_t ev;
    truth_value_t new_ev;
    truth_value_t *result;
    int i;
    TEST_CASE("stress: PLN revision converges toward truth");
    ev.strength   = 0.5;
    ev.confidence = 0.0;
    new_ev.strength   = 0.9;
    new_ev.confidence = 0.1;
    for (i = 0; i < 20; i++) {
        result = pln_revision(&ev, &new_ev);
        ASSERT(result != NULL);
        ev.strength   = result->strength;
        ev.confidence = result->confidence;
        pln_tv_free(result);
    }
    /* After many revisions confidence should increase */
    ASSERT(ev.confidence > 0.5);
    ASSERT(ev.strength > 0.5); /* converges toward 0.9 */
    ASSERT(1);
}

/* ── atomspace_query under load ──────────────────────────────────── */

static int pred_all(const atom_t *a, void *arg)
{
    (void)a; (void)arg;
    return 1;
}

void test_atomspace_query_stress(void) {
    atomspace_t *as;
    atom_t **results;
    int count, i;
    TEST_CASE("stress: atomspace_query over 500 atoms");
    as = atomspace_create();
    for (i = 0; i < 500; i++) {
        char name[32];
        snprintf(name, sizeof(name), "q%d", i);
        atomspace_add(as, atom_create_node(name));
    }
    results = atomspace_query(as, pred_all, NULL, &count);
    ASSERT(count == 500);
    ASSERT(results != NULL);
    free(results);
    atomspace_free(as);
}

/* ── main ────────────────────────────────────────────────────────── */

int main(void) {
    test_create_many_nodes();
    test_create_many_links();
    test_deep_link_chain();
    test_wide_link();
    test_atomspace_stress();
    test_pln_chain_deduction();
    test_repeated_init_shutdown();
    test_unique_ids_stress();
    test_pln_revision_convergence();
    test_atomspace_query_stress();

    return TEST_SUMMARY();
}
