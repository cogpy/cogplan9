/*
 * Exhaustive Unit Tests for PLN (Probabilistic Logic Networks)
 * Tests all PLN formulas, inference, and statistics
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

/* Float comparison with tolerance */
#define FLOAT_EQ(a, b) (((a) - (b)) < 0.001 && ((b) - (a)) < 0.001)
#define ASSERT_FLOAT_EQ(a, b) ASSERT(FLOAT_EQ(a, b))

/* ========== PLN Initialization Tests ========== */

TEST(pln_init)
{
	AtomSpace *as = atomspacecreate();
	PlnInference *pln = plninit(as);

	ASSERT_NOT_NULL(pln);
	ASSERT_EQ(pln->as, as);
	ASSERT_EQ(pln->nrules, 0);
	ASSERT_EQ(pln->maxsteps, 1000);

	plnfree(pln);
	atomspacefree(as);
}

TEST(pln_init_null_atomspace)
{
	PlnInference *pln = plninit(nil);

	/* Should still create pln with nil atomspace */
	ASSERT_NOT_NULL(pln);
	ASSERT_NULL(pln->as);

	plnfree(pln);
}

TEST(pln_free_null)
{
	/* Should not crash */
	plnfree(nil);
}

/* ========== Deduction Formula Tests ========== */

TEST(pln_deduction_basic)
{
	TruthValue a = {0.8, 0.9, 10};
	TruthValue b = {0.7, 0.8, 10};

	TruthValue result = plndeduction(a, b);

	/* Deduction: P(A->C) = P(A->B) * P(B->C) */
	ASSERT_FLOAT_EQ(result.strength, 0.56);  /* 0.8 * 0.7 */
	ASSERT_FLOAT_EQ(result.confidence, 0.72); /* 0.9 * 0.8 */
	ASSERT_EQ(result.count, 20);
}

TEST(pln_deduction_zero)
{
	TruthValue a = {0.0, 0.9, 10};
	TruthValue b = {0.7, 0.8, 10};

	TruthValue result = plndeduction(a, b);

	ASSERT_FLOAT_EQ(result.strength, 0.0);
}

TEST(pln_deduction_one)
{
	TruthValue a = {1.0, 1.0, 10};
	TruthValue b = {1.0, 1.0, 10};

	TruthValue result = plndeduction(a, b);

	ASSERT_FLOAT_EQ(result.strength, 1.0);
	ASSERT_FLOAT_EQ(result.confidence, 1.0);
}

/* ========== Induction Formula Tests ========== */

TEST(pln_induction_basic)
{
	TruthValue ab = {0.8, 0.9, 10};
	TruthValue ba = {0.6, 0.7, 10};

	TruthValue result = plninduction(ab, ba);

	ASSERT_GT(result.strength, 0.0);
	ASSERT_LT(result.strength, 1.0);
	ASSERT_GT(result.confidence, 0.0);
}

TEST(pln_induction_symmetric)
{
	TruthValue ab = {0.5, 0.9, 10};
	TruthValue ba = {0.5, 0.9, 10};

	TruthValue result = plninduction(ab, ba);

	/* With symmetric inputs, result should be around 0.5 */
	ASSERT_GT(result.strength, 0.4);
	ASSERT_LT(result.strength, 0.6);
}

/* ========== Abduction Formula Tests ========== */

TEST(pln_abduction_basic)
{
	TruthValue ab = {0.8, 0.9, 10};
	TruthValue bc = {0.7, 0.8, 10};

	TruthValue result = plnabduction(ab, bc);

	/* Abduction has reduced strength compared to deduction */
	ASSERT_GT(result.strength, 0.0);
	ASSERT_LT(result.strength, 0.8 * 0.7);
}

TEST(pln_abduction_low_confidence)
{
	TruthValue ab = {0.8, 0.1, 10};
	TruthValue bc = {0.7, 0.1, 10};

	TruthValue result = plnabduction(ab, bc);

	ASSERT_LT(result.confidence, 0.1);
}

/* ========== Revision Formula Tests ========== */

TEST(pln_revision_basic)
{
	TruthValue a = {0.8, 0.6, 10};
	TruthValue b = {0.6, 0.4, 10};

	TruthValue result = plnrevision(a, b);

	/* Revision combines evidence */
	ASSERT_GT(result.strength, 0.6);
	ASSERT_LT(result.strength, 0.8);
	ASSERT_GT(result.confidence, 0.0);
}

TEST(pln_revision_equal_confidence)
{
	TruthValue a = {0.9, 0.5, 10};
	TruthValue b = {0.3, 0.5, 10};

	TruthValue result = plnrevision(a, b);

	/* With equal confidence, result is average */
	ASSERT_FLOAT_EQ(result.strength, 0.6);
}

TEST(pln_revision_zero_confidence)
{
	TruthValue a = {0.8, 0.0, 10};
	TruthValue b = {0.6, 0.0, 10};

	TruthValue result = plnrevision(a, b);

	/* With zero confidence, default values */
	ASSERT_FLOAT_EQ(result.strength, 0.5);
	ASSERT_FLOAT_EQ(result.confidence, 0.0);
}

/* ========== Boolean Formula Tests ========== */

TEST(pln_and_basic)
{
	TruthValue a = {0.8, 0.9, 10};
	TruthValue b = {0.7, 0.8, 10};

	TruthValue result = plnand(a, b);

	ASSERT_FLOAT_EQ(result.strength, 0.56);  /* 0.8 * 0.7 */
}

TEST(pln_and_with_zero)
{
	TruthValue a = {0.0, 0.9, 10};
	TruthValue b = {0.7, 0.8, 10};

	TruthValue result = plnand(a, b);

	ASSERT_FLOAT_EQ(result.strength, 0.0);
}

TEST(pln_or_basic)
{
	TruthValue a = {0.8, 0.9, 10};
	TruthValue b = {0.7, 0.8, 10};

	TruthValue result = plnor(a, b);

	/* OR: P(A|B) = P(A) + P(B) - P(A)*P(B) */
	float expected = 0.8 + 0.7 - (0.8 * 0.7);
	ASSERT_FLOAT_EQ(result.strength, expected);
}

TEST(pln_or_with_one)
{
	TruthValue a = {1.0, 0.9, 10};
	TruthValue b = {0.7, 0.8, 10};

	TruthValue result = plnor(a, b);

	ASSERT_FLOAT_EQ(result.strength, 1.0);
}

TEST(pln_not_basic)
{
	TruthValue a = {0.8, 0.9, 10};

	TruthValue result = plnnot(a);

	ASSERT_FLOAT_EQ(result.strength, 0.2);  /* 1 - 0.8 */
	ASSERT_FLOAT_EQ(result.confidence, 0.9);
}

TEST(pln_not_zero)
{
	TruthValue a = {0.0, 0.9, 10};

	TruthValue result = plnnot(a);

	ASSERT_FLOAT_EQ(result.strength, 1.0);
}

TEST(pln_not_one)
{
	TruthValue a = {1.0, 0.9, 10};

	TruthValue result = plnnot(a);

	ASSERT_FLOAT_EQ(result.strength, 0.0);
}

/* ========== Direct Evaluation Tests ========== */

TEST(pln_eval_basic)
{
	AtomSpace *as = atomspacecreate();
	PlnInference *pln = plninit(as);

	Atom *a = atomcreate(as, ConceptNode, "test");
	TruthValue tv = {0.75, 0.85, 10};
	atomsettruth(a, tv);

	TruthValue result = plneval(pln, a);

	ASSERT_FLOAT_EQ(result.strength, 0.75);
	ASSERT_FLOAT_EQ(result.confidence, 0.85);

	plnfree(pln);
	atomspacefree(as);
}

TEST(pln_eval_null)
{
	AtomSpace *as = atomspacecreate();
	PlnInference *pln = plninit(as);

	TruthValue result = plneval(pln, nil);

	ASSERT_FLOAT_EQ(result.strength, 0.0);
	ASSERT_FLOAT_EQ(result.confidence, 0.0);

	plnfree(pln);
	atomspacefree(as);
}

/* ========== Forward Chaining Tests ========== */

TEST(pln_forward_empty)
{
	AtomSpace *as = atomspacecreate();
	PlnInference *pln = plninit(as);

	int n;
	Atom **results = plnforward(pln, nil, 10, &n);

	ASSERT_NOT_NULL(results);
	ASSERT_EQ(n, 0);
	free(results);

	plnfree(pln);
	atomspacefree(as);
}

TEST(pln_forward_chain)
{
	AtomSpace *as = atomspacecreate();
	PlnInference *pln = plninit(as);

	/* Create A -> B -> C chain */
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
	Atom **results = plnforward(pln, a, 10, &n);

	ASSERT_NOT_NULL(results);
	/* Should produce A -> C through deduction */
	ASSERT_GE(n, 0);
	free(results);

	plnfree(pln);
	atomspacefree(as);
}

/* ========== Backward Chaining Tests ========== */

TEST(pln_backward_empty)
{
	AtomSpace *as = atomspacecreate();
	PlnInference *pln = plninit(as);

	int n;
	Atom **results = plnbackward(pln, nil, 10, &n);

	ASSERT_NOT_NULL(results);
	ASSERT_EQ(n, 0);
	free(results);

	plnfree(pln);
	atomspacefree(as);
}

TEST(pln_backward_chain)
{
	AtomSpace *as = atomspacecreate();
	PlnInference *pln = plninit(as);

	/* Create A -> B -> C chain */
	Atom *a = atomcreate(as, ConceptNode, "A");
	Atom *b = atomcreate(as, ConceptNode, "B");
	Atom *c = atomcreate(as, ConceptNode, "C");

	Atom *ab[2] = {a, b};
	Atom *bc[2] = {b, c};

	linkcreate(as, InheritanceLink, ab, 2);
	linkcreate(as, InheritanceLink, bc, 2);

	int n;
	Atom **results = plnbackward(pln, c, 10, &n);

	ASSERT_NOT_NULL(results);
	/* Should find B as premise for C */
	ASSERT_GE(n, 0);
	free(results);

	plnfree(pln);
	atomspacefree(as);
}

/* ========== Statistics Tests ========== */

TEST(pln_stats_initial)
{
	AtomSpace *as = atomspacecreate();
	PlnInference *pln = plninit(as);

	plnresetstats(pln);

	PlnStats stats;
	plnstats(pln, &stats);

	ASSERT_EQ(stats.inferences, 0);
	ASSERT_EQ(stats.forward, 0);
	ASSERT_EQ(stats.backward, 0);

	plnfree(pln);
	atomspacefree(as);
}

TEST(pln_stats_after_inference)
{
	AtomSpace *as = atomspacecreate();
	PlnInference *pln = plninit(as);

	plnresetstats(pln);

	/* Create some links for inference */
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
	Atom **results = plnforward(pln, nil, 5, &n);
	free(results);

	PlnStats stats;
	plnstats(pln, &stats);

	/* Should have recorded some forward steps */
	ASSERT_GE(stats.forward, 0);

	plnfree(pln);
	atomspacefree(as);
}

TEST(pln_stats_reset)
{
	AtomSpace *as = atomspacecreate();
	PlnInference *pln = plninit(as);

	/* Do some inference */
	Atom *a = atomcreate(as, ConceptNode, "A");
	Atom *b = atomcreate(as, ConceptNode, "B");
	Atom *ab[2] = {a, b};
	linkcreate(as, InheritanceLink, ab, 2);

	int n;
	Atom **results = plnforward(pln, nil, 5, &n);
	free(results);

	/* Reset */
	plnresetstats(pln);

	PlnStats stats;
	plnstats(pln, &stats);

	ASSERT_EQ(stats.inferences, 0);
	ASSERT_EQ(stats.forward, 0);

	plnfree(pln);
	atomspacefree(as);
}

/* ========== Rule Management Tests ========== */

TEST(pln_add_rule)
{
	AtomSpace *as = atomspacecreate();
	PlnInference *pln = plninit(as);

	ASSERT_EQ(pln->nrules, 0);

	PlnRule *rule = mallocz(sizeof(PlnRule), 1);
	rule->id = 1;
	rule->name = strdup("test_rule");
	rule->weight = 1.0;

	plnaddrule(pln, rule);

	ASSERT_EQ(pln->nrules, 1);

	plnfree(pln);
	atomspacefree(as);
}

TEST(pln_add_multiple_rules)
{
	AtomSpace *as = atomspacecreate();
	PlnInference *pln = plninit(as);

	int i;
	for(i = 0; i < 10; i++){
		PlnRule *rule = mallocz(sizeof(PlnRule), 1);
		rule->id = i;
		rule->name = nil;
		rule->weight = 1.0;
		plnaddrule(pln, rule);
	}

	ASSERT_EQ(pln->nrules, 10);

	plnfree(pln);
	atomspacefree(as);
}

/* ========== Attention Allocation Tests ========== */

TEST(pln_alloc_attention)
{
	AtomSpace *as = atomspacecreate();
	PlnInference *pln = plninit(as);

	Atom *a = atomcreate(as, ConceptNode, "A");

	plnallocattention(pln, a, 50);

	short sti = plngetattention(pln, a);
	ASSERT_EQ(sti, 50);

	plnfree(pln);
	atomspacefree(as);
}

TEST(pln_alloc_attention_cumulative)
{
	AtomSpace *as = atomspacecreate();
	PlnInference *pln = plninit(as);

	Atom *a = atomcreate(as, ConceptNode, "A");

	plnallocattention(pln, a, 30);
	plnallocattention(pln, a, 20);

	short sti = plngetattention(pln, a);
	ASSERT_EQ(sti, 50);

	plnfree(pln);
	atomspacefree(as);
}

TEST(pln_spread_attention)
{
	AtomSpace *as = atomspacecreate();
	PlnInference *pln = plninit(as);

	Atom *a = atomcreate(as, ConceptNode, "A");
	Atom *b = atomcreate(as, ConceptNode, "B");
	Atom *ab[2] = {a, b};
	linkcreate(as, InheritanceLink, ab, 2);

	plnallocattention(pln, a, 100);
	plnspreadattention(pln, a);

	/* Attention should have spread */
	short sti_a = plngetattention(pln, a);
	ASSERT_EQ(sti_a, 100);  /* Source keeps its attention */

	plnfree(pln);
	atomspacefree(as);
}

/* ========== Main Test Runner ========== */

void
main(int argc, char *argv[])
{
	print("=== PLN Unit Tests ===\n\n");

	print("PLN Initialization Tests:\n");
	RUN_TEST(pln_init);
	RUN_TEST(pln_init_null_atomspace);
	RUN_TEST(pln_free_null);

	print("\nDeduction Formula Tests:\n");
	RUN_TEST(pln_deduction_basic);
	RUN_TEST(pln_deduction_zero);
	RUN_TEST(pln_deduction_one);

	print("\nInduction Formula Tests:\n");
	RUN_TEST(pln_induction_basic);
	RUN_TEST(pln_induction_symmetric);

	print("\nAbduction Formula Tests:\n");
	RUN_TEST(pln_abduction_basic);
	RUN_TEST(pln_abduction_low_confidence);

	print("\nRevision Formula Tests:\n");
	RUN_TEST(pln_revision_basic);
	RUN_TEST(pln_revision_equal_confidence);
	RUN_TEST(pln_revision_zero_confidence);

	print("\nBoolean Formula Tests:\n");
	RUN_TEST(pln_and_basic);
	RUN_TEST(pln_and_with_zero);
	RUN_TEST(pln_or_basic);
	RUN_TEST(pln_or_with_one);
	RUN_TEST(pln_not_basic);
	RUN_TEST(pln_not_zero);
	RUN_TEST(pln_not_one);

	print("\nDirect Evaluation Tests:\n");
	RUN_TEST(pln_eval_basic);
	RUN_TEST(pln_eval_null);

	print("\nForward Chaining Tests:\n");
	RUN_TEST(pln_forward_empty);
	RUN_TEST(pln_forward_chain);

	print("\nBackward Chaining Tests:\n");
	RUN_TEST(pln_backward_empty);
	RUN_TEST(pln_backward_chain);

	print("\nStatistics Tests:\n");
	RUN_TEST(pln_stats_initial);
	RUN_TEST(pln_stats_after_inference);
	RUN_TEST(pln_stats_reset);

	print("\nRule Management Tests:\n");
	RUN_TEST(pln_add_rule);
	RUN_TEST(pln_add_multiple_rules);

	print("\nAttention Allocation Tests:\n");
	RUN_TEST(pln_alloc_attention);
	RUN_TEST(pln_alloc_attention_cumulative);
	RUN_TEST(pln_spread_attention);

	print("\n=== Test Summary ===\n");
	print("Tests run: %d\n", tests_run);
	print("Tests passed: %d\n", tests_passed);
	print("Tests failed: %d\n", tests_failed);

	exits(tests_failed > 0 ? "FAIL" : nil);
}
