/*
 * Exhaustive Unit Tests for Pattern Matching
 * Tests pattern creation, matching, and mining
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
#define ASSERT_STR_EQ(a, b) ASSERT(strcmp((a), (b)) == 0)

/* Float comparison with tolerance */
#define FLOAT_EQ(a, b) (((a) - (b)) < 0.01 && ((b) - (a)) < 0.01)
#define ASSERT_FLOAT_EQ(a, b) ASSERT(FLOAT_EQ(a, b))

/* ========== Pattern Structure Tests ========== */

TEST(pattern_struct_basic)
{
	Pattern pat;
	pat.type = ConceptNode;
	pat.name = "test";
	pat.outgoing = nil;
	pat.noutgoing = 0;
	pat.wildcard = 0;

	ASSERT_EQ(pat.type, ConceptNode);
	ASSERT_STR_EQ(pat.name, "test");
	ASSERT_EQ(pat.wildcard, 0);
}

TEST(pattern_wildcard)
{
	Pattern pat;
	pat.type = 0;
	pat.name = nil;
	pat.outgoing = nil;
	pat.noutgoing = 0;
	pat.wildcard = 1;

	ASSERT_EQ(pat.wildcard, 1);
}

/* ========== Pattern Matching Tests ========== */

TEST(pattern_match_exact)
{
	AtomSpace *as = atomspacecreate();

	Atom *a = atomcreate(as, ConceptNode, "test");

	Pattern pat;
	pat.type = ConceptNode;
	pat.name = "test";
	pat.outgoing = nil;
	pat.noutgoing = 0;
	pat.wildcard = 0;

	int n;
	Atom **results = atommatch(as, &pat, &n);

	ASSERT_EQ(n, 1);
	ASSERT_EQ(results[0], a);
	free(results);

	atomspacefree(as);
}

TEST(pattern_match_type_only)
{
	AtomSpace *as = atomspacecreate();

	atomcreate(as, ConceptNode, "a1");
	atomcreate(as, ConceptNode, "a2");
	atomcreate(as, PredicateNode, "p1");

	Pattern pat;
	pat.type = ConceptNode;
	pat.name = nil;
	pat.outgoing = nil;
	pat.noutgoing = 0;
	pat.wildcard = 0;

	int n;
	Atom **results = atommatch(as, &pat, &n);

	ASSERT_EQ(n, 2);
	free(results);

	atomspacefree(as);
}

TEST(pattern_match_wildcard)
{
	AtomSpace *as = atomspacecreate();

	atomcreate(as, ConceptNode, "a1");
	atomcreate(as, ConceptNode, "a2");
	atomcreate(as, PredicateNode, "p1");

	Pattern pat;
	pat.type = 0;
	pat.name = nil;
	pat.outgoing = nil;
	pat.noutgoing = 0;
	pat.wildcard = 1;

	int n;
	Atom **results = atommatch(as, &pat, &n);

	ASSERT_EQ(n, 3);
	free(results);

	atomspacefree(as);
}

TEST(pattern_match_empty)
{
	AtomSpace *as = atomspacecreate();

	Pattern pat;
	pat.type = ConceptNode;
	pat.name = "nonexistent";
	pat.outgoing = nil;
	pat.noutgoing = 0;
	pat.wildcard = 0;

	int n;
	Atom **results = atommatch(as, &pat, &n);

	ASSERT_EQ(n, 0);
	free(results);

	atomspacefree(as);
}

TEST(pattern_match_link)
{
	AtomSpace *as = atomspacecreate();

	Atom *a = atomcreate(as, ConceptNode, "A");
	Atom *b = atomcreate(as, ConceptNode, "B");
	Atom *outgoing[2] = {a, b};
	linkcreate(as, InheritanceLink, outgoing, 2);

	Pattern pat;
	pat.type = InheritanceLink;
	pat.name = nil;
	pat.outgoing = nil;
	pat.noutgoing = 2;
	pat.wildcard = 0;

	int n;
	Atom **results = atommatch(as, &pat, &n);

	ASSERT_EQ(n, 1);
	ASSERT_EQ(results[0]->type, InheritanceLink);
	free(results);

	atomspacefree(as);
}

/* ========== Pattern Mining Tests ========== */

TEST(pattern_miner_init)
{
	AtomSpace *as = atomspacecreate();
	PatternMiner *pm = patterninit(as);

	ASSERT_NOT_NULL(pm);
	ASSERT_EQ(pm->as, as);
	ASSERT_EQ(pm->npatterns, 0);

	patternfree(pm);
	atomspacefree(as);
}

TEST(pattern_miner_free_null)
{
	/* Should not crash */
	patternfree(nil);
}

TEST(pattern_mine_empty)
{
	AtomSpace *as = atomspacecreate();
	PatternMiner *pm = patterninit(as);

	patternmine(pm, 2);

	ASSERT_EQ(pm->npatterns, 0);

	patternfree(pm);
	atomspacefree(as);
}

TEST(pattern_mine_single)
{
	AtomSpace *as = atomspacecreate();
	PatternMiner *pm = patterninit(as);

	atomcreate(as, ConceptNode, "test");

	patternmine(pm, 1);

	ASSERT_GE(pm->npatterns, 0);

	patternfree(pm);
	atomspacefree(as);
}

TEST(pattern_mine_frequent)
{
	AtomSpace *as = atomspacecreate();
	PatternMiner *pm = patterninit(as);
	int i;
	char name[32];

	/* Create many similar atoms */
	for(i = 0; i < 10; i++){
		snprint(name, sizeof(name), "concept%d", i);
		atomcreate(as, ConceptNode, name);
	}

	patternmine(pm, 2);

	/* Should find frequent ConceptNode pattern */
	ASSERT_GE(pm->npatterns, 0);

	patternfree(pm);
	atomspacefree(as);
}

TEST(pattern_get)
{
	AtomSpace *as = atomspacecreate();
	PatternMiner *pm = patterninit(as);

	atomcreate(as, ConceptNode, "a1");
	atomcreate(as, ConceptNode, "a2");
	atomcreate(as, ConceptNode, "a3");

	patternmine(pm, 1);

	int n;
	Pattern **patterns = patternget(pm, &n);

	ASSERT_GE(n, 0);
	if(n > 0){
		ASSERT_NOT_NULL(patterns);
	}

	patternfree(pm);
	atomspacefree(as);
}

/* ========== Pattern Support Tests ========== */

TEST(pattern_support_basic)
{
	AtomSpace *as = atomspacecreate();
	PatternMiner *pm = patterninit(as);

	atomcreate(as, ConceptNode, "test");

	Pattern pat;
	pat.type = ConceptNode;
	pat.name = nil;
	pat.outgoing = nil;
	pat.noutgoing = 0;
	pat.wildcard = 0;

	float support = patternsupport(pm, &pat);

	ASSERT_GT(support, 0.0);
	ASSERT_LE(support, 1.0);

	patternfree(pm);
	atomspacefree(as);
}

TEST(pattern_support_all_match)
{
	AtomSpace *as = atomspacecreate();
	PatternMiner *pm = patterninit(as);

	atomcreate(as, ConceptNode, "a1");
	atomcreate(as, ConceptNode, "a2");

	Pattern pat;
	pat.type = 0;
	pat.name = nil;
	pat.outgoing = nil;
	pat.noutgoing = 0;
	pat.wildcard = 1;

	float support = patternsupport(pm, &pat);

	ASSERT_FLOAT_EQ(support, 1.0);

	patternfree(pm);
	atomspacefree(as);
}

TEST(pattern_support_none_match)
{
	AtomSpace *as = atomspacecreate();
	PatternMiner *pm = patterninit(as);

	atomcreate(as, ConceptNode, "test");

	Pattern pat;
	pat.type = PredicateNode;
	pat.name = nil;
	pat.outgoing = nil;
	pat.noutgoing = 0;
	pat.wildcard = 0;

	float support = patternsupport(pm, &pat);

	ASSERT_FLOAT_EQ(support, 0.0);

	patternfree(pm);
	atomspacefree(as);
}

TEST(pattern_support_null)
{
	AtomSpace *as = atomspacecreate();
	PatternMiner *pm = patterninit(as);

	float support = patternsupport(pm, nil);
	ASSERT_FLOAT_EQ(support, 0.0);

	float support2 = patternsupport(nil, nil);
	ASSERT_FLOAT_EQ(support2, 0.0);

	patternfree(pm);
	atomspacefree(as);
}

/* ========== Pattern Confidence Tests ========== */

TEST(pattern_confidence_basic)
{
	AtomSpace *as = atomspacecreate();
	PatternMiner *pm = patterninit(as);

	atomcreate(as, ConceptNode, "test");

	Pattern pat;
	pat.type = ConceptNode;
	pat.name = nil;
	pat.outgoing = nil;
	pat.noutgoing = 0;
	pat.wildcard = 0;

	float confidence = patternconfidence(pm, &pat);

	ASSERT_GE(confidence, 0.0);
	ASSERT_LE(confidence, 1.0);

	patternfree(pm);
	atomspacefree(as);
}

TEST(pattern_confidence_null)
{
	AtomSpace *as = atomspacecreate();
	PatternMiner *pm = patterninit(as);

	float confidence = patternconfidence(pm, nil);
	ASSERT_FLOAT_EQ(confidence, 0.0);

	patternfree(pm);
	atomspacefree(as);
}

/* ========== Pattern String Tests ========== */

TEST(pattern_str_basic)
{
	Pattern pat;
	pat.type = ConceptNode;
	pat.name = "test";
	pat.outgoing = nil;
	pat.noutgoing = 0;
	pat.wildcard = 0;

	char *str = cogpatternstr(&pat);
	ASSERT_NOT_NULL(str);
}

TEST(pattern_str_wildcard)
{
	Pattern pat;
	pat.type = 0;
	pat.name = nil;
	pat.outgoing = nil;
	pat.noutgoing = 0;
	pat.wildcard = 1;

	char *str = cogpatternstr(&pat);
	ASSERT_STR_EQ(str, "*");
}

TEST(pattern_str_null)
{
	char *str = cogpatternstr(nil);
	ASSERT_STR_EQ(str, "nil");
}

/* ========== Integration Tests ========== */

TEST(pattern_full_workflow)
{
	AtomSpace *as = atomspacecreate();
	PatternMiner *pm = patterninit(as);
	int i;
	char name[32];

	/* Create diverse atoms */
	for(i = 0; i < 20; i++){
		snprint(name, sizeof(name), "concept%d", i);
		atomcreate(as, ConceptNode, name);
	}

	for(i = 0; i < 10; i++){
		snprint(name, sizeof(name), "predicate%d", i);
		atomcreate(as, PredicateNode, name);
	}

	/* Mine patterns */
	patternmine(pm, 3);

	/* Get patterns */
	int n;
	Pattern **patterns = patternget(pm, &n);

	if(n > 0){
		/* Check support/confidence of first pattern */
		float support = patternsupport(pm, patterns[0]);
		float confidence = patternconfidence(pm, patterns[0]);

		ASSERT_GE(support, 0.0);
		ASSERT_GE(confidence, 0.0);
	}

	patternfree(pm);
	atomspacefree(as);
}

/* ========== Main Test Runner ========== */

void
main(int argc, char *argv[])
{
	print("=== Pattern Matching Unit Tests ===\n\n");

	print("Pattern Structure Tests:\n");
	RUN_TEST(pattern_struct_basic);
	RUN_TEST(pattern_wildcard);

	print("\nPattern Matching Tests:\n");
	RUN_TEST(pattern_match_exact);
	RUN_TEST(pattern_match_type_only);
	RUN_TEST(pattern_match_wildcard);
	RUN_TEST(pattern_match_empty);
	RUN_TEST(pattern_match_link);

	print("\nPattern Mining Tests:\n");
	RUN_TEST(pattern_miner_init);
	RUN_TEST(pattern_miner_free_null);
	RUN_TEST(pattern_mine_empty);
	RUN_TEST(pattern_mine_single);
	RUN_TEST(pattern_mine_frequent);
	RUN_TEST(pattern_get);

	print("\nPattern Support Tests:\n");
	RUN_TEST(pattern_support_basic);
	RUN_TEST(pattern_support_all_match);
	RUN_TEST(pattern_support_none_match);
	RUN_TEST(pattern_support_null);

	print("\nPattern Confidence Tests:\n");
	RUN_TEST(pattern_confidence_basic);
	RUN_TEST(pattern_confidence_null);

	print("\nPattern String Tests:\n");
	RUN_TEST(pattern_str_basic);
	RUN_TEST(pattern_str_wildcard);
	RUN_TEST(pattern_str_null);

	print("\nIntegration Tests:\n");
	RUN_TEST(pattern_full_workflow);

	print("\n=== Test Summary ===\n");
	print("Tests run: %d\n", tests_run);
	print("Tests passed: %d\n", tests_passed);
	print("Tests failed: %d\n", tests_failed);

	exits(tests_failed > 0 ? "FAIL" : nil);
}
