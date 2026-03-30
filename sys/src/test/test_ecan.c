/*
 * Exhaustive Unit Tests for ECAN (Economic Attention Network)
 * Tests all attention allocation, focus, and decay operations
 */

#include <u.h>
#include <libc.h>
#include <plan9cog.h>

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
#define ASSERT_GE(a, b) ASSERT((a) >= (b))
#define ASSERT_LE(a, b) ASSERT((a) <= (b))
#define ASSERT_NULL(a) ASSERT((a) == nil)
#define ASSERT_NOT_NULL(a) ASSERT((a) != nil)

/* ========== ECAN Initialization Tests ========== */

TEST(ecan_init)
{
	AtomSpace *as = atomspacecreate();
	EcanNetwork *ecan = ecaninit(as, 1000, 500);

	ASSERT_NOT_NULL(ecan);
	ASSERT_EQ(ecan->as, as);
	ASSERT_EQ(ecan->totalsti, 1000);
	ASSERT_EQ(ecan->totallti, 500);
	ASSERT_EQ(ecan->attentionalfocus, 20);
	ASSERT_EQ(ecan->nfocus, 0);

	ecanfree(ecan);
	atomspacefree(as);
}

TEST(ecan_init_null_atomspace)
{
	EcanNetwork *ecan = ecaninit(nil, 1000, 500);

	ASSERT_NOT_NULL(ecan);
	ASSERT_NULL(ecan->as);

	ecanfree(ecan);
}

TEST(ecan_free_null)
{
	/* Should not crash */
	ecanfree(nil);
}

/* ========== ECAN Allocation Tests ========== */

TEST(ecan_allocate_basic)
{
	AtomSpace *as = atomspacecreate();
	EcanNetwork *ecan = ecaninit(as, 1000, 500);

	Atom *a = atomcreate(as, ConceptNode, "test");
	ecanallocate(ecan, a, 100);

	AttentionValue av = atomgetattention(a);
	ASSERT_EQ(av.sti, 100);

	ecanfree(ecan);
	atomspacefree(as);
}

TEST(ecan_allocate_cumulative)
{
	AtomSpace *as = atomspacecreate();
	EcanNetwork *ecan = ecaninit(as, 1000, 500);

	Atom *a = atomcreate(as, ConceptNode, "test");
	ecanallocate(ecan, a, 50);
	ecanallocate(ecan, a, 30);

	AttentionValue av = atomgetattention(a);
	ASSERT_EQ(av.sti, 80);

	ecanfree(ecan);
	atomspacefree(as);
}

TEST(ecan_allocate_negative)
{
	AtomSpace *as = atomspacecreate();
	EcanNetwork *ecan = ecaninit(as, 1000, 500);

	Atom *a = atomcreate(as, ConceptNode, "test");
	ecanallocate(ecan, a, 100);
	ecanallocate(ecan, a, -30);

	AttentionValue av = atomgetattention(a);
	ASSERT_EQ(av.sti, 70);

	ecanfree(ecan);
	atomspacefree(as);
}

TEST(ecan_allocate_null_atom)
{
	AtomSpace *as = atomspacecreate();
	EcanNetwork *ecan = ecaninit(as, 1000, 500);

	/* Should not crash */
	ecanallocate(ecan, nil, 100);

	ecanfree(ecan);
	atomspacefree(as);
}

TEST(ecan_allocate_multiple_atoms)
{
	AtomSpace *as = atomspacecreate();
	EcanNetwork *ecan = ecaninit(as, 1000, 500);

	Atom *a1 = atomcreate(as, ConceptNode, "a1");
	Atom *a2 = atomcreate(as, ConceptNode, "a2");
	Atom *a3 = atomcreate(as, ConceptNode, "a3");

	ecanallocate(ecan, a1, 100);
	ecanallocate(ecan, a2, 50);
	ecanallocate(ecan, a3, 75);

	ASSERT_EQ(atomgetattention(a1).sti, 100);
	ASSERT_EQ(atomgetattention(a2).sti, 50);
	ASSERT_EQ(atomgetattention(a3).sti, 75);

	ecanfree(ecan);
	atomspacefree(as);
}

/* ========== ECAN Update Tests ========== */

TEST(ecan_update_empty)
{
	AtomSpace *as = atomspacecreate();
	EcanNetwork *ecan = ecaninit(as, 1000, 500);

	ecanupdate(ecan);

	ASSERT_EQ(ecan->nfocus, 0);

	ecanfree(ecan);
	atomspacefree(as);
}

TEST(ecan_update_single)
{
	AtomSpace *as = atomspacecreate();
	EcanNetwork *ecan = ecaninit(as, 1000, 500);

	Atom *a = atomcreate(as, ConceptNode, "test");
	ecanallocate(ecan, a, 100);
	ecanupdate(ecan);

	ASSERT_EQ(ecan->nfocus, 1);
	ASSERT_EQ(ecan->focusatoms[0], a);

	ecanfree(ecan);
	atomspacefree(as);
}

TEST(ecan_update_sorting)
{
	AtomSpace *as = atomspacecreate();
	EcanNetwork *ecan = ecaninit(as, 1000, 500);

	Atom *a1 = atomcreate(as, ConceptNode, "a1");
	Atom *a2 = atomcreate(as, ConceptNode, "a2");
	Atom *a3 = atomcreate(as, ConceptNode, "a3");

	ecanallocate(ecan, a1, 50);
	ecanallocate(ecan, a2, 100);
	ecanallocate(ecan, a3, 75);

	ecanupdate(ecan);

	/* Should be sorted by STI descending */
	ASSERT_EQ(ecan->nfocus, 3);
	ASSERT_EQ(ecan->focusatoms[0], a2);  /* 100 */
	ASSERT_EQ(ecan->focusatoms[1], a3);  /* 75 */
	ASSERT_EQ(ecan->focusatoms[2], a1);  /* 50 */

	ecanfree(ecan);
	atomspacefree(as);
}

TEST(ecan_update_focus_limit)
{
	AtomSpace *as = atomspacecreate();
	EcanNetwork *ecan = ecaninit(as, 1000, 500);
	int i;
	char name[32];

	/* Create more atoms than focus size */
	for(i = 0; i < 30; i++){
		snprint(name, sizeof(name), "atom%d", i);
		Atom *a = atomcreate(as, ConceptNode, name);
		ecanallocate(ecan, a, i * 10);
	}

	ecanupdate(ecan);

	/* Focus should be limited to attentionalfocus size */
	ASSERT_EQ(ecan->nfocus, 20);

	/* Top focus atom should have highest STI */
	ASSERT_EQ(atomgetattention(ecan->focusatoms[0]).sti, 290);

	ecanfree(ecan);
	atomspacefree(as);
}

/* ========== ECAN Focus Tests ========== */

TEST(ecan_focus_empty)
{
	AtomSpace *as = atomspacecreate();
	EcanNetwork *ecan = ecaninit(as, 1000, 500);

	int n;
	Atom **focus = ecanfocus(ecan, &n);

	ASSERT_EQ(n, 0);
	ASSERT_NULL(focus);

	ecanfree(ecan);
	atomspacefree(as);
}

TEST(ecan_focus_after_update)
{
	AtomSpace *as = atomspacecreate();
	EcanNetwork *ecan = ecaninit(as, 1000, 500);

	Atom *a = atomcreate(as, ConceptNode, "test");
	ecanallocate(ecan, a, 100);
	ecanupdate(ecan);

	int n;
	Atom **focus = ecanfocus(ecan, &n);

	ASSERT_EQ(n, 1);
	ASSERT_NOT_NULL(focus);
	ASSERT_EQ(focus[0], a);

	ecanfree(ecan);
	atomspacefree(as);
}

TEST(ecan_focus_null)
{
	int n;
	Atom **focus = ecanfocus(nil, &n);

	ASSERT_EQ(n, 0);
	ASSERT_NULL(focus);
}

/* ========== ECAN Spread Tests ========== */

TEST(ecan_spread_no_incoming)
{
	AtomSpace *as = atomspacecreate();
	EcanNetwork *ecan = ecaninit(as, 1000, 500);

	Atom *a = atomcreate(as, ConceptNode, "test");
	ecanallocate(ecan, a, 100);

	/* Should not crash with no incoming links */
	ecanspread(ecan, a);

	ASSERT_EQ(atomgetattention(a).sti, 100);

	ecanfree(ecan);
	atomspacefree(as);
}

TEST(ecan_spread_with_link)
{
	AtomSpace *as = atomspacecreate();
	EcanNetwork *ecan = ecaninit(as, 1000, 500);

	Atom *a = atomcreate(as, ConceptNode, "A");
	Atom *b = atomcreate(as, ConceptNode, "B");
	Atom *outgoing[2] = {a, b};
	Atom *link = linkcreate(as, InheritanceLink, outgoing, 2);

	ecanallocate(ecan, a, 100);
	ecanspread(ecan, a);

	/* Link should have received some attention */
	ASSERT_GT(atomgetattention(link).sti, 0);

	ecanfree(ecan);
	atomspacefree(as);
}

TEST(ecan_spread_null)
{
	AtomSpace *as = atomspacecreate();
	EcanNetwork *ecan = ecaninit(as, 1000, 500);

	/* Should not crash */
	ecanspread(ecan, nil);
	ecanspread(nil, nil);

	ecanfree(ecan);
	atomspacefree(as);
}

TEST(ecan_spread_zero_sti)
{
	AtomSpace *as = atomspacecreate();
	EcanNetwork *ecan = ecaninit(as, 1000, 500);

	Atom *a = atomcreate(as, ConceptNode, "A");
	Atom *b = atomcreate(as, ConceptNode, "B");
	Atom *outgoing[2] = {a, b};
	linkcreate(as, InheritanceLink, outgoing, 2);

	/* STI is 0, should not spread */
	ecanspread(ecan, a);

	ecanfree(ecan);
	atomspacefree(as);
}

/* ========== ECAN Decay Tests ========== */

TEST(ecan_decay_basic)
{
	AtomSpace *as = atomspacecreate();
	EcanNetwork *ecan = ecaninit(as, 1000, 500);

	Atom *a = atomcreate(as, ConceptNode, "test");
	ecanallocate(ecan, a, 100);

	ecandecay(ecan, 0.1);  /* 10% decay */

	AttentionValue av = atomgetattention(a);
	ASSERT_EQ(av.sti, 90);  /* 100 * 0.9 = 90 */

	ecanfree(ecan);
	atomspacefree(as);
}

TEST(ecan_decay_multiple)
{
	AtomSpace *as = atomspacecreate();
	EcanNetwork *ecan = ecaninit(as, 1000, 500);

	Atom *a = atomcreate(as, ConceptNode, "test");
	ecanallocate(ecan, a, 100);

	ecandecay(ecan, 0.1);
	ecandecay(ecan, 0.1);
	ecandecay(ecan, 0.1);

	AttentionValue av = atomgetattention(a);
	/* 100 * 0.9 * 0.9 * 0.9 = 72.9 -> 72 */
	ASSERT_LT(av.sti, 75);

	ecanfree(ecan);
	atomspacefree(as);
}

TEST(ecan_decay_zero_rate)
{
	AtomSpace *as = atomspacecreate();
	EcanNetwork *ecan = ecaninit(as, 1000, 500);

	Atom *a = atomcreate(as, ConceptNode, "test");
	ecanallocate(ecan, a, 100);

	ecandecay(ecan, 0.0);

	AttentionValue av = atomgetattention(a);
	ASSERT_EQ(av.sti, 100);

	ecanfree(ecan);
	atomspacefree(as);
}

TEST(ecan_decay_full_rate)
{
	AtomSpace *as = atomspacecreate();
	EcanNetwork *ecan = ecaninit(as, 1000, 500);

	Atom *a = atomcreate(as, ConceptNode, "test");
	ecanallocate(ecan, a, 100);

	ecandecay(ecan, 1.0);

	AttentionValue av = atomgetattention(a);
	ASSERT_EQ(av.sti, 0);

	ecanfree(ecan);
	atomspacefree(as);
}

TEST(ecan_decay_all_atoms)
{
	AtomSpace *as = atomspacecreate();
	EcanNetwork *ecan = ecaninit(as, 1000, 500);

	Atom *a1 = atomcreate(as, ConceptNode, "a1");
	Atom *a2 = atomcreate(as, ConceptNode, "a2");
	Atom *a3 = atomcreate(as, ConceptNode, "a3");

	ecanallocate(ecan, a1, 100);
	ecanallocate(ecan, a2, 50);
	ecanallocate(ecan, a3, 200);

	ecandecay(ecan, 0.5);  /* 50% decay */

	ASSERT_EQ(atomgetattention(a1).sti, 50);
	ASSERT_EQ(atomgetattention(a2).sti, 25);
	ASSERT_EQ(atomgetattention(a3).sti, 100);

	ecanfree(ecan);
	atomspacefree(as);
}

TEST(ecan_decay_null)
{
	/* Should not crash */
	ecandecay(nil, 0.5);
}

/* ========== Integration Tests ========== */

TEST(ecan_full_cycle)
{
	AtomSpace *as = atomspacecreate();
	EcanNetwork *ecan = ecaninit(as, 1000, 500);
	int i;
	char name[32];

	/* Create atoms with varying STI */
	for(i = 0; i < 10; i++){
		snprint(name, sizeof(name), "atom%d", i);
		Atom *a = atomcreate(as, ConceptNode, name);
		ecanallocate(ecan, a, (i + 1) * 10);
	}

	/* Update focus */
	ecanupdate(ecan);
	ASSERT_EQ(ecan->nfocus, 10);

	/* Decay */
	ecandecay(ecan, 0.1);

	/* Update again */
	ecanupdate(ecan);

	/* Focus should still be sorted correctly */
	ASSERT_GE(atomgetattention(ecan->focusatoms[0]).sti,
	          atomgetattention(ecan->focusatoms[1]).sti);

	ecanfree(ecan);
	atomspacefree(as);
}

TEST(ecan_attention_flow)
{
	AtomSpace *as = atomspacecreate();
	EcanNetwork *ecan = ecaninit(as, 1000, 500);

	/* Create a chain: A -> B -> C */
	Atom *a = atomcreate(as, ConceptNode, "A");
	Atom *b = atomcreate(as, ConceptNode, "B");
	Atom *c = atomcreate(as, ConceptNode, "C");

	Atom *ab[2] = {a, b};
	Atom *bc[2] = {b, c};

	linkcreate(as, InheritanceLink, ab, 2);
	linkcreate(as, InheritanceLink, bc, 2);

	/* Allocate attention to A */
	ecanallocate(ecan, a, 100);

	/* Spread attention */
	ecanspread(ecan, a);

	/* Update focus */
	ecanupdate(ecan);

	/* A should be in focus */
	int n;
	Atom **focus = ecanfocus(ecan, &n);
	ASSERT_GT(n, 0);

	ecanfree(ecan);
	atomspacefree(as);
}

/* ========== Main Test Runner ========== */

void
main(int argc, char *argv[])
{
	print("=== ECAN Unit Tests ===\n\n");

	print("ECAN Initialization Tests:\n");
	RUN_TEST(ecan_init);
	RUN_TEST(ecan_init_null_atomspace);
	RUN_TEST(ecan_free_null);

	print("\nECAN Allocation Tests:\n");
	RUN_TEST(ecan_allocate_basic);
	RUN_TEST(ecan_allocate_cumulative);
	RUN_TEST(ecan_allocate_negative);
	RUN_TEST(ecan_allocate_null_atom);
	RUN_TEST(ecan_allocate_multiple_atoms);

	print("\nECAN Update Tests:\n");
	RUN_TEST(ecan_update_empty);
	RUN_TEST(ecan_update_single);
	RUN_TEST(ecan_update_sorting);
	RUN_TEST(ecan_update_focus_limit);

	print("\nECAN Focus Tests:\n");
	RUN_TEST(ecan_focus_empty);
	RUN_TEST(ecan_focus_after_update);
	RUN_TEST(ecan_focus_null);

	print("\nECAN Spread Tests:\n");
	RUN_TEST(ecan_spread_no_incoming);
	RUN_TEST(ecan_spread_with_link);
	RUN_TEST(ecan_spread_null);
	RUN_TEST(ecan_spread_zero_sti);

	print("\nECAN Decay Tests:\n");
	RUN_TEST(ecan_decay_basic);
	RUN_TEST(ecan_decay_multiple);
	RUN_TEST(ecan_decay_zero_rate);
	RUN_TEST(ecan_decay_full_rate);
	RUN_TEST(ecan_decay_all_atoms);
	RUN_TEST(ecan_decay_null);

	print("\nIntegration Tests:\n");
	RUN_TEST(ecan_full_cycle);
	RUN_TEST(ecan_attention_flow);

	print("\n=== Test Summary ===\n");
	print("Tests run: %d\n", tests_run);
	print("Tests passed: %d\n", tests_passed);
	print("Tests failed: %d\n", tests_failed);

	exits(tests_failed > 0 ? "FAIL" : nil);
}
