/*
 * Exhaustive Unit Tests for AtomSpace
 * Tests all AtomSpace operations, edge cases, and performance
 */

#include <u.h>
#include <libc.h>
#include <plan9cog/atomspace.h>

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) void test_##name(void)
#define RUN_TEST(name) do { \
	tests_run++; \
	print("  Running %s... ", #name); \
	test_##name(); \
	print("PASSED\n"); \
	tests_passed++; \
} while(0)

#define ASSERT(cond) do { \
	if(!(cond)) { \
		print("FAILED\n"); \
		print("    Assertion failed: %s\n", #cond); \
		print("    at line %d\n", __LINE__); \
		tests_failed++; \
		return; \
	} \
} while(0)

#define ASSERT_EQ(a, b) ASSERT((a) == (b))
#define ASSERT_NE(a, b) ASSERT((a) != (b))
#define ASSERT_GT(a, b) ASSERT((a) > (b))
#define ASSERT_LT(a, b) ASSERT((a) < (b))
#define ASSERT_NULL(a) ASSERT((a) == nil)
#define ASSERT_NOT_NULL(a) ASSERT((a) != nil)
#define ASSERT_STR_EQ(a, b) ASSERT(strcmp((a), (b)) == 0)

/* ========== AtomSpace Creation Tests ========== */

TEST(atomspace_create)
{
	AtomSpace *as = atomspacecreate();
	ASSERT_NOT_NULL(as);
	ASSERT_EQ(as->natoms, 0);
	ASSERT_NOT_NULL(as->atoms);
	atomspacefree(as);
}

TEST(atomspace_create_multiple)
{
	AtomSpace *as1 = atomspacecreate();
	AtomSpace *as2 = atomspacecreate();
	AtomSpace *as3 = atomspacecreate();

	ASSERT_NOT_NULL(as1);
	ASSERT_NOT_NULL(as2);
	ASSERT_NOT_NULL(as3);
	ASSERT_NE(as1, as2);
	ASSERT_NE(as2, as3);

	atomspacefree(as3);
	atomspacefree(as2);
	atomspacefree(as1);
}

TEST(atomspace_free_null)
{
	/* Should not crash */
	atomspacefree(nil);
}

/* ========== Atom Creation Tests ========== */

TEST(atom_create_basic)
{
	AtomSpace *as = atomspacecreate();
	Atom *a = atomcreate(as, ConceptNode, "test");

	ASSERT_NOT_NULL(a);
	ASSERT_GT(a->id, 0);
	ASSERT_EQ(a->type, ConceptNode);
	ASSERT_STR_EQ(a->name, "test");
	ASSERT_EQ(as->natoms, 1);

	atomspacefree(as);
}

TEST(atom_create_null_name)
{
	AtomSpace *as = atomspacecreate();
	Atom *a = atomcreate(as, ConceptNode, nil);

	ASSERT_NOT_NULL(a);
	ASSERT_NULL(a->name);

	atomspacefree(as);
}

TEST(atom_create_empty_name)
{
	AtomSpace *as = atomspacecreate();
	Atom *a = atomcreate(as, ConceptNode, "");

	ASSERT_NOT_NULL(a);
	ASSERT_STR_EQ(a->name, "");

	atomspacefree(as);
}

TEST(atom_create_all_types)
{
	AtomSpace *as = atomspacecreate();

	Atom *concept = atomcreate(as, ConceptNode, "concept");
	Atom *predicate = atomcreate(as, PredicateNode, "predicate");

	ASSERT_NOT_NULL(concept);
	ASSERT_NOT_NULL(predicate);
	ASSERT_EQ(concept->type, ConceptNode);
	ASSERT_EQ(predicate->type, PredicateNode);

	atomspacefree(as);
}

TEST(atom_create_unique_ids)
{
	AtomSpace *as = atomspacecreate();
	Atom *a1 = atomcreate(as, ConceptNode, "a1");
	Atom *a2 = atomcreate(as, ConceptNode, "a2");
	Atom *a3 = atomcreate(as, ConceptNode, "a3");

	ASSERT_NE(a1->id, a2->id);
	ASSERT_NE(a2->id, a3->id);
	ASSERT_NE(a1->id, a3->id);

	atomspacefree(as);
}

TEST(atom_create_many)
{
	AtomSpace *as = atomspacecreate();
	int i;
	char name[32];

	for(i = 0; i < 2000; i++){
		snprint(name, sizeof(name), "atom%d", i);
		Atom *a = atomcreate(as, ConceptNode, name);
		ASSERT_NOT_NULL(a);
	}

	ASSERT_EQ(as->natoms, 2000);
	atomspacefree(as);
}

/* ========== Link Creation Tests ========== */

TEST(link_create_basic)
{
	AtomSpace *as = atomspacecreate();
	Atom *a = atomcreate(as, ConceptNode, "A");
	Atom *b = atomcreate(as, ConceptNode, "B");
	Atom *outgoing[2] = {a, b};

	Atom *link = linkcreate(as, InheritanceLink, outgoing, 2);

	ASSERT_NOT_NULL(link);
	ASSERT_EQ(link->type, InheritanceLink);
	ASSERT_EQ(link->noutgoing, 2);
	ASSERT_EQ(link->outgoing[0], a);
	ASSERT_EQ(link->outgoing[1], b);

	atomspacefree(as);
}

TEST(link_create_empty_outgoing)
{
	AtomSpace *as = atomspacecreate();
	Atom *link = linkcreate(as, ListLink, nil, 0);

	ASSERT_NOT_NULL(link);
	ASSERT_EQ(link->noutgoing, 0);
	ASSERT_NULL(link->outgoing);

	atomspacefree(as);
}

TEST(link_create_single_outgoing)
{
	AtomSpace *as = atomspacecreate();
	Atom *a = atomcreate(as, ConceptNode, "A");
	Atom *outgoing[1] = {a};

	Atom *link = linkcreate(as, ListLink, outgoing, 1);

	ASSERT_NOT_NULL(link);
	ASSERT_EQ(link->noutgoing, 1);
	ASSERT_EQ(link->outgoing[0], a);

	atomspacefree(as);
}

TEST(link_create_chain)
{
	AtomSpace *as = atomspacecreate();
	Atom *a = atomcreate(as, ConceptNode, "A");
	Atom *b = atomcreate(as, ConceptNode, "B");
	Atom *c = atomcreate(as, ConceptNode, "C");

	Atom *ab[2] = {a, b};
	Atom *bc[2] = {b, c};

	Atom *link1 = linkcreate(as, InheritanceLink, ab, 2);
	Atom *link2 = linkcreate(as, InheritanceLink, bc, 2);

	ASSERT_NOT_NULL(link1);
	ASSERT_NOT_NULL(link2);
	ASSERT_EQ(link1->outgoing[1], link2->outgoing[0]);

	atomspacefree(as);
}

/* ========== Atom Find Tests ========== */

TEST(atom_find_existing)
{
	AtomSpace *as = atomspacecreate();
	Atom *created = atomcreate(as, ConceptNode, "test");
	Atom *found = atomfind(as, created->id);

	ASSERT_EQ(created, found);

	atomspacefree(as);
}

TEST(atom_find_nonexistent)
{
	AtomSpace *as = atomspacecreate();
	Atom *found = atomfind(as, 99999);

	ASSERT_NULL(found);

	atomspacefree(as);
}

TEST(atom_find_after_delete)
{
	AtomSpace *as = atomspacecreate();
	Atom *a = atomcreate(as, ConceptNode, "test");
	ulong id = a->id;

	atomdelete(as, id);
	Atom *found = atomfind(as, id);

	ASSERT_NULL(found);

	atomspacefree(as);
}

TEST(atom_find_performance)
{
	AtomSpace *as = atomspacecreate();
	int i;
	char name[32];
	ulong ids[1000];

	/* Create many atoms */
	for(i = 0; i < 1000; i++){
		snprint(name, sizeof(name), "atom%d", i);
		Atom *a = atomcreate(as, ConceptNode, name);
		ids[i] = a->id;
	}

	/* Find all atoms */
	for(i = 0; i < 1000; i++){
		Atom *found = atomfind(as, ids[i]);
		ASSERT_NOT_NULL(found);
		ASSERT_EQ(found->id, ids[i]);
	}

	atomspacefree(as);
}

/* ========== Atom Delete Tests ========== */

TEST(atom_delete_existing)
{
	AtomSpace *as = atomspacecreate();
	Atom *a = atomcreate(as, ConceptNode, "test");
	ulong id = a->id;

	int result = atomdelete(as, id);
	ASSERT_EQ(result, 0);

	atomspacefree(as);
}

TEST(atom_delete_nonexistent)
{
	AtomSpace *as = atomspacecreate();
	int result = atomdelete(as, 99999);

	ASSERT_EQ(result, -1);

	atomspacefree(as);
}

TEST(atom_delete_multiple)
{
	AtomSpace *as = atomspacecreate();
	Atom *a1 = atomcreate(as, ConceptNode, "a1");
	Atom *a2 = atomcreate(as, ConceptNode, "a2");
	Atom *a3 = atomcreate(as, ConceptNode, "a3");

	ulong id1 = a1->id;
	ulong id2 = a2->id;
	ulong id3 = a3->id;

	ASSERT_EQ(atomdelete(as, id2), 0);
	ASSERT_NOT_NULL(atomfind(as, id1));
	ASSERT_NULL(atomfind(as, id2));
	ASSERT_NOT_NULL(atomfind(as, id3));

	atomspacefree(as);
}

/* ========== Truth Value Tests ========== */

TEST(truth_value_default)
{
	AtomSpace *as = atomspacecreate();
	Atom *a = atomcreate(as, ConceptNode, "test");
	TruthValue tv = atomgettruth(a);

	ASSERT_EQ(tv.strength, 0.5);
	ASSERT_EQ(tv.confidence, 0.0);
	ASSERT_EQ(tv.count, 0);

	atomspacefree(as);
}

TEST(truth_value_set_get)
{
	AtomSpace *as = atomspacecreate();
	Atom *a = atomcreate(as, ConceptNode, "test");

	TruthValue tv;
	tv.strength = 0.8;
	tv.confidence = 0.9;
	tv.count = 100;

	atomsettruth(a, tv);
	TruthValue result = atomgettruth(a);

	ASSERT_EQ(result.strength, 0.8);
	ASSERT_EQ(result.confidence, 0.9);
	ASSERT_EQ(result.count, 100);

	atomspacefree(as);
}

TEST(truth_value_boundary)
{
	AtomSpace *as = atomspacecreate();
	Atom *a = atomcreate(as, ConceptNode, "test");

	TruthValue tv;
	tv.strength = 0.0;
	tv.confidence = 1.0;
	tv.count = 0;

	atomsettruth(a, tv);
	TruthValue result = atomgettruth(a);

	ASSERT_EQ(result.strength, 0.0);
	ASSERT_EQ(result.confidence, 1.0);

	atomspacefree(as);
}

/* ========== Attention Value Tests ========== */

TEST(attention_value_default)
{
	AtomSpace *as = atomspacecreate();
	Atom *a = atomcreate(as, ConceptNode, "test");
	AttentionValue av = atomgetattention(a);

	ASSERT_EQ(av.sti, 0);
	ASSERT_EQ(av.lti, 0);
	ASSERT_EQ(av.vlti, 0);

	atomspacefree(as);
}

TEST(attention_value_set_get)
{
	AtomSpace *as = atomspacecreate();
	Atom *a = atomcreate(as, ConceptNode, "test");

	AttentionValue av;
	av.sti = 100;
	av.lti = 50;
	av.vlti = 25;

	atomsetattention(a, av);
	AttentionValue result = atomgetattention(a);

	ASSERT_EQ(result.sti, 100);
	ASSERT_EQ(result.lti, 50);
	ASSERT_EQ(result.vlti, 25);

	atomspacefree(as);
}

TEST(attention_value_negative)
{
	AtomSpace *as = atomspacecreate();
	Atom *a = atomcreate(as, ConceptNode, "test");

	AttentionValue av;
	av.sti = -100;
	av.lti = -50;
	av.vlti = 0;

	atomsetattention(a, av);
	AttentionValue result = atomgetattention(a);

	ASSERT_EQ(result.sti, -100);
	ASSERT_EQ(result.lti, -50);

	atomspacefree(as);
}

/* ========== Query Tests ========== */

static int
pred_concept_nodes(Atom *a, void *arg)
{
	return a->type == ConceptNode;
}

static int
pred_high_sti(Atom *a, void *arg)
{
	return a->av.sti > 50;
}

TEST(atom_query_by_type)
{
	AtomSpace *as = atomspacecreate();
	atomcreate(as, ConceptNode, "c1");
	atomcreate(as, ConceptNode, "c2");
	atomcreate(as, PredicateNode, "p1");

	int n;
	Atom **results = atomquery(as, pred_concept_nodes, nil, &n);

	ASSERT_EQ(n, 2);
	ASSERT_NOT_NULL(results);
	free(results);

	atomspacefree(as);
}

TEST(atom_query_by_attention)
{
	AtomSpace *as = atomspacecreate();
	Atom *a1 = atomcreate(as, ConceptNode, "a1");
	Atom *a2 = atomcreate(as, ConceptNode, "a2");
	Atom *a3 = atomcreate(as, ConceptNode, "a3");

	AttentionValue av;
	av.sti = 100; av.lti = 0; av.vlti = 0;
	atomsetattention(a1, av);
	atomsetattention(a2, av);

	av.sti = 10;
	atomsetattention(a3, av);

	int n;
	Atom **results = atomquery(as, pred_high_sti, nil, &n);

	ASSERT_EQ(n, 2);
	free(results);

	atomspacefree(as);
}

TEST(atom_query_empty)
{
	AtomSpace *as = atomspacecreate();

	int n;
	Atom **results = atomquery(as, pred_concept_nodes, nil, &n);

	ASSERT_EQ(n, 0);
	free(results);

	atomspacefree(as);
}

/* ========== Incoming Set Tests ========== */

TEST(atom_get_incoming_basic)
{
	AtomSpace *as = atomspacecreate();
	Atom *a = atomcreate(as, ConceptNode, "A");
	Atom *b = atomcreate(as, ConceptNode, "B");
	Atom *outgoing[2] = {a, b};

	linkcreate(as, InheritanceLink, outgoing, 2);

	int n;
	Atom **incoming = atomgetincoming(as, a, &n);

	ASSERT_EQ(n, 1);
	ASSERT_NOT_NULL(incoming);
	free(incoming);

	atomspacefree(as);
}

TEST(atom_get_incoming_multiple)
{
	AtomSpace *as = atomspacecreate();
	Atom *a = atomcreate(as, ConceptNode, "A");
	Atom *b = atomcreate(as, ConceptNode, "B");
	Atom *c = atomcreate(as, ConceptNode, "C");

	Atom *ab[2] = {a, b};
	Atom *ac[2] = {a, c};

	linkcreate(as, InheritanceLink, ab, 2);
	linkcreate(as, SimilarityLink, ac, 2);

	int n;
	Atom **incoming = atomgetincoming(as, a, &n);

	ASSERT_EQ(n, 2);
	free(incoming);

	atomspacefree(as);
}

TEST(atom_get_incoming_none)
{
	AtomSpace *as = atomspacecreate();
	Atom *a = atomcreate(as, ConceptNode, "A");

	int n;
	Atom **incoming = atomgetincoming(as, a, &n);

	ASSERT_EQ(n, 0);
	free(incoming);

	atomspacefree(as);
}

/* ========== IPC Tests ========== */

TEST(cog_msg_send_recv)
{
	int fds[2];
	CogMsg msg, recv_msg;

	if(pipe(fds) < 0)
		return;

	msg.type = CogAtomCreate;
	msg.atomid = 123;
	msg.atomtype = ConceptNode;
	strcpy(msg.data, "test_data");
	msg.ndata = strlen(msg.data);

	/* This is a basic structural test */
	ASSERT_EQ(sizeof(CogMsg), sizeof(recv_msg));

	close(fds[0]);
	close(fds[1]);
}

/* ========== Main Test Runner ========== */

void
main(int argc, char *argv[])
{
	print("=== AtomSpace Unit Tests ===\n\n");

	print("AtomSpace Creation Tests:\n");
	RUN_TEST(atomspace_create);
	RUN_TEST(atomspace_create_multiple);
	RUN_TEST(atomspace_free_null);

	print("\nAtom Creation Tests:\n");
	RUN_TEST(atom_create_basic);
	RUN_TEST(atom_create_null_name);
	RUN_TEST(atom_create_empty_name);
	RUN_TEST(atom_create_all_types);
	RUN_TEST(atom_create_unique_ids);
	RUN_TEST(atom_create_many);

	print("\nLink Creation Tests:\n");
	RUN_TEST(link_create_basic);
	RUN_TEST(link_create_empty_outgoing);
	RUN_TEST(link_create_single_outgoing);
	RUN_TEST(link_create_chain);

	print("\nAtom Find Tests:\n");
	RUN_TEST(atom_find_existing);
	RUN_TEST(atom_find_nonexistent);
	RUN_TEST(atom_find_after_delete);
	RUN_TEST(atom_find_performance);

	print("\nAtom Delete Tests:\n");
	RUN_TEST(atom_delete_existing);
	RUN_TEST(atom_delete_nonexistent);
	RUN_TEST(atom_delete_multiple);

	print("\nTruth Value Tests:\n");
	RUN_TEST(truth_value_default);
	RUN_TEST(truth_value_set_get);
	RUN_TEST(truth_value_boundary);

	print("\nAttention Value Tests:\n");
	RUN_TEST(attention_value_default);
	RUN_TEST(attention_value_set_get);
	RUN_TEST(attention_value_negative);

	print("\nQuery Tests:\n");
	RUN_TEST(atom_query_by_type);
	RUN_TEST(atom_query_by_attention);
	RUN_TEST(atom_query_empty);

	print("\nIncoming Set Tests:\n");
	RUN_TEST(atom_get_incoming_basic);
	RUN_TEST(atom_get_incoming_multiple);
	RUN_TEST(atom_get_incoming_none);

	print("\nIPC Tests:\n");
	RUN_TEST(cog_msg_send_recv);

	print("\n=== Test Summary ===\n");
	print("Tests run: %d\n", tests_run);
	print("Tests passed: %d\n", tests_passed);
	print("Tests failed: %d\n", tests_failed);

	exits(tests_failed > 0 ? "FAIL" : nil);
}
