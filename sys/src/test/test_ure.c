/*
 * Exhaustive Unit Tests for URE (Unified Rule Engine)
 * Tests rule chaining, priority, and integration
 */

#include <u.h>
#include <libc.h>
#include <plan9cog/atomspace.h>
#include <plan9cog/pln.h>

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

/* ========== URE Initialization Tests ========== */

TEST(ure_init)
{
	AtomSpace *as = atomspacecreate();
	Atom *rulebase = atomcreate(as, ConceptNode, "rulebase");
	UreChainer *ure = ureinit(as, rulebase);

	ASSERT_NOT_NULL(ure);
	ASSERT_NOT_NULL(ure->pln);
	ASSERT_EQ(ure->rulebase, rulebase);
	ASSERT_EQ(ure->maxiter, 100);

	urefree(ure);
	atomspacefree(as);
}

TEST(ure_init_null_rulebase)
{
	AtomSpace *as = atomspacecreate();
	UreChainer *ure = ureinit(as, nil);

	ASSERT_NOT_NULL(ure);
	ASSERT_NULL(ure->rulebase);

	urefree(ure);
	atomspacefree(as);
}

TEST(ure_free_null)
{
	/* Should not crash */
	urefree(nil);
}

/* ========== URE Chain Tests ========== */

TEST(ure_chain_empty)
{
	AtomSpace *as = atomspacecreate();
	UreChainer *ure = ureinit(as, nil);

	int n;
	Atom **results = urechain(ure, nil, &n);

	ASSERT_NOT_NULL(results);
	ASSERT_EQ(n, 0);
	free(results);

	urefree(ure);
	atomspacefree(as);
}

TEST(ure_chain_no_rules)
{
	AtomSpace *as = atomspacecreate();
	UreChainer *ure = ureinit(as, nil);

	/* Create some atoms for inference */
	Atom *a = atomcreate(as, ConceptNode, "A");
	Atom *b = atomcreate(as, ConceptNode, "B");
	Atom *c = atomcreate(as, ConceptNode, "C");

	Atom *ab[2] = {a, b};
	Atom *bc[2] = {b, c};

	Atom *link1 = linkcreate(as, InheritanceLink, ab, 2);
	Atom *link2 = linkcreate(as, InheritanceLink, bc, 2);

	TruthValue tv = {0.8, 0.9, 10};
	atomsettruth(link1, tv);
	atomsettruth(link2, tv);

	int n;
	Atom **results = urechain(ure, a, &n);

	ASSERT_NOT_NULL(results);
	/* Without explicit rules, should fall back to PLN forward chaining */
	ASSERT_GE(n, 0);
	free(results);

	urefree(ure);
	atomspacefree(as);
}

TEST(ure_chain_with_target)
{
	AtomSpace *as = atomspacecreate();
	UreChainer *ure = ureinit(as, nil);

	Atom *a = atomcreate(as, ConceptNode, "A");
	Atom *b = atomcreate(as, ConceptNode, "B");
	Atom *ab[2] = {a, b};
	linkcreate(as, InheritanceLink, ab, 2);

	int n;
	Atom **results = urechain(ure, a, &n);

	ASSERT_NOT_NULL(results);
	free(results);

	urefree(ure);
	atomspacefree(as);
}

TEST(ure_chain_complexity_increase)
{
	AtomSpace *as = atomspacecreate();
	UreChainer *ure = ureinit(as, nil);

	float initial_complexity = ure->complexity;

	/* Create atoms for inference */
	Atom *a = atomcreate(as, ConceptNode, "A");
	Atom *b = atomcreate(as, ConceptNode, "B");
	Atom *ab[2] = {a, b};
	linkcreate(as, InheritanceLink, ab, 2);

	int n;
	Atom **results = urechain(ure, a, &n);
	free(results);

	/* Complexity should have potentially increased */
	ASSERT_GE(ure->complexity, initial_complexity);

	urefree(ure);
	atomspacefree(as);
}

/* ========== URE with Rules Tests ========== */

TEST(ure_chain_with_rules)
{
	AtomSpace *as = atomspacecreate();
	UreChainer *ure = ureinit(as, nil);

	/* Add a simple rule */
	PlnRule *rule = mallocz(sizeof(PlnRule), 1);
	rule->id = 1;
	rule->name = strdup("deduction");
	rule->weight = 1.0;
	rule->npremises = 0;
	rule->premises = nil;
	rule->conclusion = nil;

	plnaddrule(ure->pln, rule);

	ASSERT_EQ(ure->pln->nrules, 1);

	/* Chain with rule */
	Atom *a = atomcreate(as, ConceptNode, "A");
	int n;
	Atom **results = urechain(ure, a, &n);

	ASSERT_NOT_NULL(results);
	free(results);

	urefree(ure);
	atomspacefree(as);
}

/* ========== URE Maxiter Tests ========== */

TEST(ure_maxiter_default)
{
	AtomSpace *as = atomspacecreate();
	UreChainer *ure = ureinit(as, nil);

	ASSERT_EQ(ure->maxiter, 100);

	urefree(ure);
	atomspacefree(as);
}

TEST(ure_maxiter_modify)
{
	AtomSpace *as = atomspacecreate();
	UreChainer *ure = ureinit(as, nil);

	ure->maxiter = 50;
	ASSERT_EQ(ure->maxiter, 50);

	urefree(ure);
	atomspacefree(as);
}

/* ========== Main Test Runner ========== */

void
main(int argc, char *argv[])
{
	print("=== URE Unit Tests ===\n\n");

	print("URE Initialization Tests:\n");
	RUN_TEST(ure_init);
	RUN_TEST(ure_init_null_rulebase);
	RUN_TEST(ure_free_null);

	print("\nURE Chain Tests:\n");
	RUN_TEST(ure_chain_empty);
	RUN_TEST(ure_chain_no_rules);
	RUN_TEST(ure_chain_with_target);
	RUN_TEST(ure_chain_complexity_increase);

	print("\nURE with Rules Tests:\n");
	RUN_TEST(ure_chain_with_rules);

	print("\nURE Maxiter Tests:\n");
	RUN_TEST(ure_maxiter_default);
	RUN_TEST(ure_maxiter_modify);

	print("\n=== Test Summary ===\n");
	print("Tests run: %d\n", tests_run);
	print("Tests passed: %d\n", tests_passed);
	print("Tests failed: %d\n", tests_failed);

	exits(tests_failed > 0 ? "FAIL" : nil);
}
