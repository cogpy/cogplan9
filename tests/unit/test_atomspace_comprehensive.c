/*
 * tests/unit/test_atomspace_comprehensive.c
 *
 * Exhaustive unit tests for the AtomSpace API covering:
 *   - Normal operation
 *   - Edge cases (NULL inputs, empty strings, zero counts)
 *   - Boundary values (max counts, deep nesting)
 *   - Error handling
 *   - AtomSpace CRUD operations
 *   - Query/predicate system
 *   - Truth value and attention value handling
 */

#include "../test_macros.h"
#include <plan9cog/atomspace.h>
#include <stdlib.h>
#include <string.h>

/* ── Atom creation ───────────────────────────────────────────────── */

void test_node_basic(void) {
    atom_t *n;
    TEST_CASE("atom_create_node: basic");
    n = atom_create_node("hello");
    ASSERT(n != NULL);
    ASSERT(n->type == NODE);
    ASSERT_STR_EQ(n->name, "hello");
    ASSERT(n->outgoing_set == NULL);
    ASSERT(n->outgoing_set_size == 0);
    ASSERT(n->tv == NULL);
    ASSERT(n->id > 0);
    atom_free(n);
}

void test_node_empty_name(void) {
    atom_t *n;
    TEST_CASE("atom_create_node: empty name");
    n = atom_create_node("");
    ASSERT(n != NULL);
    ASSERT(n->type == NODE);
    atom_free(n);
}

void test_node_null_name(void) {
    atom_t *n;
    TEST_CASE("atom_create_node: NULL name");
    n = atom_create_node(NULL);
    ASSERT(n != NULL);
    ASSERT(n->type == NODE);
    atom_free(n);
}

void test_node_long_name(void) {
    char buf[4097];
    atom_t *n;
    TEST_CASE("atom_create_node: very long name");
    memset(buf, 'a', sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    n = atom_create_node(buf);
    ASSERT(n != NULL);
    ASSERT_STR_EQ(n->name, buf);
    atom_free(n);
}

void test_node_typed(void) {
    atom_t *n;
    TEST_CASE("atom_create_node_typed: CONCEPT_NODE");
    n = atom_create_node_typed("Cat", CONCEPT_NODE);
    ASSERT(n != NULL);
    ASSERT(n->type == CONCEPT_NODE);
    ASSERT_STR_EQ(n->name, "Cat");
    atom_free(n);
}

void test_node_typed_all_types(void) {
    int types[] = {NODE, LINK, CONCEPT_NODE, PREDICATE_NODE,
                   EVALUATION_LINK, INHERITANCE_LINK, SIMILARITY_LINK,
                   IMPLICATION_LINK, LIST_LINK, EXECUTION_LINK};
    int i;
    atom_t *n;
    TEST_CASE("atom_create_node_typed: all atom types");
    for (i = 0; i < 10; i++) {
        n = atom_create_node_typed("x", types[i]);
        ASSERT(n != NULL);
        ASSERT(n->type == types[i]);
        atom_free(n);
    }
}

/* ── Link creation ───────────────────────────────────────────────── */

void test_link_basic(void) {
    atom_t *a, *b, *targets[2], *link;
    TEST_CASE("atom_create_link: basic 2-outgoing");
    a = atom_create_node("A");
    b = atom_create_node("B");
    targets[0] = a;
    targets[1] = b;
    link = atom_create_link("MyLink", 2, targets);
    ASSERT(link != NULL);
    ASSERT(link->type == LINK);
    ASSERT_STR_EQ(link->name, "MyLink");
    ASSERT(link->outgoing_set_size == 2);
    ASSERT(link->outgoing_set[0] == a);
    ASSERT(link->outgoing_set[1] == b);
    atom_free(link);
}

void test_link_zero_outgoing(void) {
    atom_t *link;
    TEST_CASE("atom_create_link: zero outgoing");
    link = atom_create_link("EmptyLink", 0, NULL);
    ASSERT(link != NULL);
    ASSERT(link->outgoing_set_size == 0);
    atom_free(link);
}

void test_link_single_outgoing(void) {
    atom_t *child, *targets[1], *link;
    TEST_CASE("atom_create_link: single outgoing");
    child = atom_create_node("Child");
    targets[0] = child;
    link = atom_create_link("UnaryLink", 1, targets);
    ASSERT(link != NULL);
    ASSERT(link->outgoing_set_size == 1);
    ASSERT(link->outgoing_set[0] == child);
    atom_free(link);
}

void test_link_deep_nesting(void) {
    /* Build a chain: L3 -> L2 -> L1 -> leaf */
    atom_t *leaf, *t1[1], *l1, *t2[1], *l2, *t3[1], *l3;
    TEST_CASE("atom_create_link: deep nesting (3 levels)");
    leaf = atom_create_node("leaf");
    t1[0] = leaf;
    l1 = atom_create_link("L1", 1, t1);
    t2[0] = l1;
    l2 = atom_create_link("L2", 1, t2);
    t3[0] = l2;
    l3 = atom_create_link("L3", 1, t3);
    ASSERT(l3 != NULL);
    ASSERT(l3->outgoing_set[0] == l2);
    ASSERT(l3->outgoing_set[0]->outgoing_set[0] == l1);
    ASSERT(l3->outgoing_set[0]->outgoing_set[0]->outgoing_set[0] == leaf);
    atom_free(l3); /* recursively frees all */
}

void test_link_typed(void) {
    atom_t *a, *b, *targets[2], *link;
    TEST_CASE("atom_create_link_typed: IMPLICATION_LINK");
    a = atom_create_node("premise");
    b = atom_create_node("conclusion");
    targets[0] = a;
    targets[1] = b;
    link = atom_create_link_typed(IMPLICATION_LINK, 2, targets);
    ASSERT(link != NULL);
    ASSERT(link->type == IMPLICATION_LINK);
    ASSERT(link->outgoing_set_size == 2);
    atom_free(link);
}

/* ── Unique IDs ──────────────────────────────────────────────────── */

void test_unique_ids(void) {
    atom_t *a, *b, *c;
    TEST_CASE("atom IDs are unique");
    a = atom_create_node("x");
    b = atom_create_node("y");
    c = atom_create_node("z");
    ASSERT(a->id != b->id);
    ASSERT(b->id != c->id);
    ASSERT(a->id != c->id);
    atom_free(a);
    atom_free(b);
    atom_free(c);
}

/* ── Truth value on atoms ────────────────────────────────────────── */

void test_atom_truth_value(void) {
    atom_t *n;
    truth_value_t *tv;
    TEST_CASE("atom truth value: set and read");
    n = atom_create_node("node");
    ASSERT(n->tv == NULL);
    tv = tv_create(0.75, 0.5);
    ASSERT(tv != NULL);
    n->tv = tv;
    ASSERT(n->tv->strength == 0.75);
    ASSERT(n->tv->confidence == 0.5);
    atom_free(n); /* frees tv too */
}

void test_truth_value_boundary(void) {
    truth_value_t *tv0, *tv1;
    TEST_CASE("tv_create: boundary values 0.0 and 1.0");
    tv0 = tv_create(0.0, 0.0);
    tv1 = tv_create(1.0, 1.0);
    ASSERT(tv0 != NULL);
    ASSERT(tv1 != NULL);
    ASSERT(tv0->strength == 0.0);
    ASSERT(tv0->confidence == 0.0);
    ASSERT(tv1->strength == 1.0);
    ASSERT(tv1->confidence == 1.0);
    tv_free(tv0);
    tv_free(tv1);
}

void test_tv_free_null(void) {
    TEST_CASE("tv_free: NULL is safe");
    tv_free(NULL); /* must not crash */
    ASSERT(1);
}

/* ── atom_free edge cases ────────────────────────────────────────── */

void test_atom_free_null(void) {
    TEST_CASE("atom_free: NULL is safe");
    atom_free(NULL); /* must not crash */
    ASSERT(1);
}

void test_atom_free_no_outgoing(void) {
    atom_t *n;
    TEST_CASE("atom_free: node with no outgoing");
    n = atom_create_node("solo");
    atom_free(n);
    ASSERT(1); /* no crash */
}

void test_atom_free_shallow(void) {
    atom_t *leaf, *targets[1], *link;
    TEST_CASE("atom_free_shallow: does not free outgoing atoms");
    leaf = atom_create_node("keep_me");
    targets[0] = leaf;
    link = atom_create_link("L", 1, targets);
    atom_free_shallow(link); /* only frees link, not leaf */
    /* leaf should still be valid */
    ASSERT_STR_EQ(leaf->name, "keep_me");
    atom_free(leaf);
}

/* ── AtomSpace ───────────────────────────────────────────────────── */

void test_atomspace_create_free(void) {
    atomspace_t *as;
    TEST_CASE("atomspace_create and atomspace_free");
    as = atomspace_create();
    ASSERT(as != NULL);
    ASSERT(atomspace_size(as) == 0);
    atomspace_free(as);
    ASSERT(1);
}

void test_atomspace_free_null(void) {
    TEST_CASE("atomspace_free: NULL is safe");
    atomspace_free(NULL);
    ASSERT(1);
}

void test_atomspace_add_find(void) {
    atomspace_t *as;
    atom_t *n, *found;
    unsigned long id;
    TEST_CASE("atomspace_add and atomspace_find");
    as = atomspace_create();
    n  = atom_create_node("thing");
    id = n->id;
    ASSERT(atomspace_add(as, n) == 0);
    ASSERT(atomspace_size(as) == 1);
    found = atomspace_find(as, id);
    ASSERT(found == n);
    atomspace_free(as);
}

void test_atomspace_find_missing(void) {
    atomspace_t *as;
    atom_t *found;
    TEST_CASE("atomspace_find: missing id returns NULL");
    as    = atomspace_create();
    found = atomspace_find(as, 999999UL);
    ASSERT(found == NULL);
    atomspace_free(as);
}

void test_atomspace_find_null_space(void) {
    atom_t *found;
    TEST_CASE("atomspace_find: NULL atomspace returns NULL");
    found = atomspace_find(NULL, 1UL);
    ASSERT(found == NULL);
}

void test_atomspace_remove(void) {
    atomspace_t *as;
    atom_t *n;
    unsigned long id;
    int rc;
    TEST_CASE("atomspace_remove: existing atom");
    as = atomspace_create();
    n  = atom_create_node("remove_me");
    id = n->id;
    atomspace_add(as, n);
    ASSERT(atomspace_size(as) == 1);
    rc = atomspace_remove(as, id);
    ASSERT(rc == 0);
    ASSERT(atomspace_size(as) == 0);
    ASSERT(atomspace_find(as, id) == NULL);
    atomspace_free(as);
}

void test_atomspace_remove_missing(void) {
    atomspace_t *as;
    int rc;
    TEST_CASE("atomspace_remove: missing id returns -1");
    as = atomspace_create();
    rc = atomspace_remove(as, 999999UL);
    ASSERT(rc == -1);
    atomspace_free(as);
}

void test_atomspace_add_null(void) {
    atomspace_t *as;
    int rc;
    TEST_CASE("atomspace_add: NULL atom returns -1");
    as = atomspace_create();
    rc = atomspace_add(as, NULL);
    ASSERT(rc == -1);
    ASSERT(atomspace_size(as) == 0);
    atomspace_free(as);
}

void test_atomspace_add_null_space(void) {
    atom_t *n;
    int rc;
    TEST_CASE("atomspace_add: NULL space returns -1");
    n  = atom_create_node("x");
    rc = atomspace_add(NULL, n);
    ASSERT(rc == -1);
    atom_free(n);
}

void test_atomspace_many_atoms(void) {
    atomspace_t *as;
    int i, n = 200;
    unsigned long ids[200];
    atom_t *atoms[200];
    char name[32];
    TEST_CASE("atomspace: add 200 atoms and find each");
    as = atomspace_create();
    for (i = 0; i < n; i++) {
        snprintf(name, sizeof(name), "atom_%d", i);
        atoms[i] = atom_create_node(name);
        ids[i]   = atoms[i]->id;
        atomspace_add(as, atoms[i]);
    }
    ASSERT(atomspace_size(as) == n);
    for (i = 0; i < n; i++)
        ASSERT(atomspace_find(as, ids[i]) != NULL);
    atomspace_free(as);
    ASSERT(1);
}

/* ── Query system ────────────────────────────────────────────────── */

static int pred_is_link(const atom_t *a, void *arg)
{
    (void)arg;
    return a->type == LINK;
}

static int pred_by_name(const atom_t *a, void *arg)
{
    return strcmp(a->name ? a->name : "", (const char *)arg) == 0;
}

void test_atomspace_query_type(void) {
    atomspace_t *as;
    atom_t *n1, *n2, *targets[2], *link;
    atom_t **results;
    int count;
    TEST_CASE("atomspace_query: filter by type");
    as       = atomspace_create();
    n1       = atom_create_node("n1");
    n2       = atom_create_node("n2");
    targets[0] = n1; targets[1] = n2;
    link     = atom_create_link("L", 2, targets);
    atomspace_add(as, atom_create_node("extra1"));
    atomspace_add(as, atom_create_node("extra2"));
    atomspace_add(as, link);
    results  = atomspace_query(as, pred_is_link, NULL, &count);
    ASSERT(count == 1);
    ASSERT(results != NULL);
    ASSERT(results[0] == link);
    free(results);
    atomspace_free(as);
}

void test_atomspace_query_by_name(void) {
    atomspace_t *as;
    atom_t *target_atom, **results;
    int count, i;
    TEST_CASE("atomspace_query: filter by name");
    as          = atomspace_create();
    for (i = 0; i < 5; i++) {
        char name[16];
        snprintf(name, sizeof(name), "item%d", i);
        atomspace_add(as, atom_create_node(name));
    }
    target_atom = atom_create_node("needle");
    atomspace_add(as, target_atom);
    results = atomspace_query(as, pred_by_name, "needle", &count);
    ASSERT(count == 1);
    ASSERT(results[0] == target_atom);
    free(results);
    atomspace_free(as);
}

void test_atomspace_query_no_match(void) {
    atomspace_t *as;
    atom_t **results;
    int count;
    TEST_CASE("atomspace_query: no matches returns NULL with count 0");
    as      = atomspace_create();
    atomspace_add(as, atom_create_node("a"));
    results = atomspace_query(as, pred_is_link, NULL, &count);
    ASSERT(count == 0);
    ASSERT(results == NULL);
    atomspace_free(as);
}

void test_atomspace_query_null_args(void) {
    atomspace_t *as;
    atom_t **results;
    int count;
    TEST_CASE("atomspace_query: NULL args are safe");
    as      = atomspace_create();
    results = atomspace_query(NULL, pred_is_link, NULL, &count);
    ASSERT(results == NULL);
    results = atomspace_query(as, NULL, NULL, &count);
    ASSERT(results == NULL);
    atomspace_free(as);
}

/* ── AtomSpace capacity growth ───────────────────────────────────── */

void test_atomspace_growth(void) {
    atomspace_t *as;
    int i;
    /* Force internal realloc by adding > initial capacity (64) */
    TEST_CASE("atomspace: growth beyond initial capacity");
    as = atomspace_create();
    for (i = 0; i < 128; i++) {
        char name[32];
        snprintf(name, sizeof(name), "g%d", i);
        ASSERT(atomspace_add(as, atom_create_node(name)) == 0);
    }
    ASSERT(atomspace_size(as) == 128);
    atomspace_free(as);
}

/* ── Attention value ─────────────────────────────────────────────── */

void test_attention_value(void) {
    atom_t *n;
    TEST_CASE("atom attention value: set and read");
    n        = atom_create_node("focused");
    n->av.sti = 100;
    n->av.lti = 50;
    n->av.vlti = 10;
    ASSERT(n->av.sti  == 100);
    ASSERT(n->av.lti  == 50);
    ASSERT(n->av.vlti == 10);
    atom_free(n);
}

/* ── main ────────────────────────────────────────────────────────── */

int main(void) {
    /* Node creation */
    test_node_basic();
    test_node_empty_name();
    test_node_null_name();
    test_node_long_name();
    test_node_typed();
    test_node_typed_all_types();

    /* Link creation */
    test_link_basic();
    test_link_zero_outgoing();
    test_link_single_outgoing();
    test_link_deep_nesting();
    test_link_typed();

    /* IDs */
    test_unique_ids();

    /* Truth values */
    test_atom_truth_value();
    test_truth_value_boundary();
    test_tv_free_null();

    /* atom_free edge cases */
    test_atom_free_null();
    test_atom_free_no_outgoing();
    test_atom_free_shallow();

    /* AtomSpace */
    test_atomspace_create_free();
    test_atomspace_free_null();
    test_atomspace_add_find();
    test_atomspace_find_missing();
    test_atomspace_find_null_space();
    test_atomspace_remove();
    test_atomspace_remove_missing();
    test_atomspace_add_null();
    test_atomspace_add_null_space();
    test_atomspace_many_atoms();

    /* Queries */
    test_atomspace_query_type();
    test_atomspace_query_by_name();
    test_atomspace_query_no_match();
    test_atomspace_query_null_args();

    /* Growth */
    test_atomspace_growth();

    /* Attention value */
    test_attention_value();

    return TEST_SUMMARY();
}
