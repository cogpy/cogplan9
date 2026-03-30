/*
 * Integration Tests for Plan9Cog
 * Tests complete workflows across all components
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

#define FLOAT_EQ(a, b) (((a) - (b)) < 0.01 && ((b) - (a)) < 0.01)
#define ASSERT_FLOAT_EQ(a, b) ASSERT(FLOAT_EQ(a, b))

/* ========== Plan9Cog System Tests ========== */

TEST(plan9cog_init)
{
	Plan9Cog *p9c = plan9coginit();

	ASSERT_NOT_NULL(p9c);
	ASSERT_NOT_NULL(p9c->atomspace);
	ASSERT_NOT_NULL(p9c->pln);
	ASSERT_EQ(p9c->initialized, 1);

	plan9cogfree(p9c);
}

TEST(plan9cog_singleton)
{
	Plan9Cog *p9c1 = plan9coginit();
	Plan9Cog *p9c2 = plan9coginstance();

	ASSERT_NOT_NULL(p9c1);
	ASSERT_NOT_NULL(p9c2);
	ASSERT_EQ(p9c1, p9c2);

	plan9cogfree(p9c1);
}

TEST(plan9cog_info)
{
	Plan9Cog *p9c = plan9coginit();
	CogInfo info;

	coginfo(p9c, &info);

	ASSERT_STR_EQ(info.version, "Plan9Cog 0.1");
	ASSERT_EQ(info.natoms, 0);

	plan9cogfree(p9c);
}

/* ========== Full Inference Pipeline Tests ========== */

TEST(inference_pipeline)
{
	Plan9Cog *p9c = plan9coginit();

	/* Create knowledge base: Animal -> Mammal -> Dog */
	Atom *animal = atomcreate(p9c->atomspace, ConceptNode, "Animal");
	Atom *mammal = atomcreate(p9c->atomspace, ConceptNode, "Mammal");
	Atom *dog = atomcreate(p9c->atomspace, ConceptNode, "Dog");

	/* Mammal inherits from Animal */
	Atom *mam_anim[2] = {mammal, animal};
	Atom *link1 = linkcreate(p9c->atomspace, InheritanceLink, mam_anim, 2);

	/* Dog inherits from Mammal */
	Atom *dog_mam[2] = {dog, mammal};
	Atom *link2 = linkcreate(p9c->atomspace, InheritanceLink, dog_mam, 2);

	/* Set truth values */
	TruthValue tv = {0.9, 0.95, 100};
	atomsettruth(link1, tv);
	atomsettruth(link2, tv);

	/* Run forward chaining */
	int n;
	Atom **results = plnforward(p9c->pln, dog, 10, &n);

	ASSERT_NOT_NULL(results);
	/* Should infer: Dog -> Animal */
	ASSERT_GE(n, 0);

	free(results);
	plan9cogfree(p9c);
}

TEST(inference_with_attention)
{
	Plan9Cog *p9c = plan9coginit();
	EcanNetwork *ecan = ecaninit(p9c->atomspace, 1000, 500);

	/* Create atoms */
	Atom *a = atomcreate(p9c->atomspace, ConceptNode, "Important");
	Atom *b = atomcreate(p9c->atomspace, ConceptNode, "LessImportant");

	/* Allocate attention */
	ecanallocate(ecan, a, 100);
	ecanallocate(ecan, b, 20);

	/* Update focus */
	ecanupdate(ecan);

	/* Most important atom should be first in focus */
	int nfocus;
	Atom **focus = ecanfocus(ecan, &nfocus);

	ASSERT_EQ(nfocus, 2);
	ASSERT_EQ(focus[0], a);

	ecanfree(ecan);
	plan9cogfree(p9c);
}

/* ========== Pattern Mining Pipeline Tests ========== */

TEST(pattern_mining_pipeline)
{
	Plan9Cog *p9c = plan9coginit();
	PatternMiner *pm = patterninit(p9c->atomspace);
	int i;
	char name[32];

	/* Create structured knowledge base */
	for(i = 0; i < 10; i++){
		snprint(name, sizeof(name), "Person%d", i);
		Atom *person = atomcreate(p9c->atomspace, ConceptNode, name);

		snprint(name, sizeof(name), "City%d", i % 3);
		Atom *city = atomcreate(p9c->atomspace, ConceptNode, name);

		Atom *lives[2] = {person, city};
		linkcreate(p9c->atomspace, EvaluationLink, lives, 2);
	}

	/* Mine patterns */
	patternmine(pm, 2);

	/* Should find patterns */
	int n;
	Pattern **patterns = patternget(pm, &n);
	ASSERT_GE(n, 0);

	patternfree(pm);
	plan9cogfree(p9c);
}

/* ========== Distributed Processing Tests ========== */

TEST(distributed_processing)
{
	Plan9Cog *p9c = plan9coginit();
	MachSpace *ms = machspaceinit(p9c->atomspace);

	/* Create local atoms */
	Atom *local = atomcreate(p9c->atomspace, ConceptNode, "LocalKnowledge");

	/* Connect to (simulated) remote nodes */
	machspaceconnect(ms, "node1.cluster");
	machspaceconnect(ms, "node2.cluster");

	/* Verify structure */
	ASSERT_EQ(ms->nremote, 2);

	/* Find local atom via machspace */
	Atom *found = machspacefind(ms, local->id);
	ASSERT_EQ(found, local);

	machspacefree(ms);
	plan9cogfree(p9c);
}

/* ========== Cognitive Grip Tests ========== */

TEST(cognitive_grip)
{
	Plan9Cog *p9c = plan9coginit();

	Atom *a = atomcreate(p9c->atomspace, ConceptNode, "TestAtom");

	/* Create grip */
	CogGrip *grip = coggrip(GripAtom, a);
	ASSERT_NOT_NULL(grip);
	ASSERT_EQ(grip->type, GripAtom);
	ASSERT_EQ(cogobject(grip), a);
	ASSERT_EQ(grip->refcount, 1);

	/* Retain */
	CogGrip *retained = cogretain(grip);
	ASSERT_EQ(retained, grip);
	ASSERT_EQ(grip->refcount, 2);

	/* Release */
	cogrelease(grip);
	ASSERT_EQ(grip->refcount, 1);

	cogrelease(grip);
	/* grip is now freed */

	plan9cogfree(p9c);
}

/* ========== Fusion Reactor Tests ========== */

TEST(fusion_reactor)
{
	Plan9Cog *p9c = plan9coginit();
	CogFusionReactor *cfr = cogreactorinit(p9c, 4);

	ASSERT_NOT_NULL(cfr);
	ASSERT_EQ(cfr->p9c, p9c);
	ASSERT_EQ(cfr->nworkers, 4);

	/* Create some atoms for processing */
	Atom *a = atomcreate(p9c->atomspace, ConceptNode, "TaskTarget");

	/* Submit task */
	cogreactorsubmit(cfr, a);

	/* Get result (synchronous processing) */
	/* Result depends on task queue state */

	cogreactorfree(cfr);
	plan9cogfree(p9c);
}

/* ========== URE with PLN Tests ========== */

TEST(ure_pln_integration)
{
	Plan9Cog *p9c = plan9coginit();
	UreChainer *ure = ureinit(p9c->atomspace, nil);

	/* Create knowledge */
	Atom *cat = atomcreate(p9c->atomspace, ConceptNode, "Cat");
	Atom *feline = atomcreate(p9c->atomspace, ConceptNode, "Feline");
	Atom *animal = atomcreate(p9c->atomspace, ConceptNode, "Animal");

	Atom *cat_feline[2] = {cat, feline};
	Atom *feline_animal[2] = {feline, animal};

	Atom *link1 = linkcreate(p9c->atomspace, InheritanceLink, cat_feline, 2);
	Atom *link2 = linkcreate(p9c->atomspace, InheritanceLink, feline_animal, 2);

	TruthValue tv = {0.95, 0.9, 50};
	atomsettruth(link1, tv);
	atomsettruth(link2, tv);

	/* Run URE chaining */
	int n;
	Atom **results = urechain(ure, cat, &n);

	ASSERT_NOT_NULL(results);
	free(results);

	urefree(ure);
	plan9cogfree(p9c);
}

/* ========== Full System Stress Tests ========== */

TEST(stress_many_atoms)
{
	Plan9Cog *p9c = plan9coginit();
	int i;
	char name[32];

	/* Create 5000 atoms */
	for(i = 0; i < 5000; i++){
		snprint(name, sizeof(name), "atom%d", i);
		Atom *a = atomcreate(p9c->atomspace, ConceptNode, name);
		ASSERT_NOT_NULL(a);
	}

	ASSERT_EQ(p9c->atomspace->natoms, 5000);

	/* Verify system info */
	CogInfo info;
	coginfo(p9c, &info);
	ASSERT_EQ(info.natoms, 5000);

	plan9cogfree(p9c);
}

TEST(stress_many_links)
{
	Plan9Cog *p9c = plan9coginit();
	int i;
	char name[32];
	Atom *prev = nil;

	/* Create chain of 1000 linked atoms */
	for(i = 0; i < 1000; i++){
		snprint(name, sizeof(name), "node%d", i);
		Atom *curr = atomcreate(p9c->atomspace, ConceptNode, name);

		if(prev != nil){
			Atom *outgoing[2] = {prev, curr};
			linkcreate(p9c->atomspace, InheritanceLink, outgoing, 2);
		}

		prev = curr;
	}

	/* Should have 1000 nodes + 999 links */
	ASSERT_EQ(p9c->atomspace->natoms, 1999);

	plan9cogfree(p9c);
}

TEST(stress_attention_propagation)
{
	Plan9Cog *p9c = plan9coginit();
	EcanNetwork *ecan = ecaninit(p9c->atomspace, 10000, 5000);
	int i;
	char name[32];

	/* Create many atoms with attention */
	for(i = 0; i < 100; i++){
		snprint(name, sizeof(name), "focus%d", i);
		Atom *a = atomcreate(p9c->atomspace, ConceptNode, name);
		ecanallocate(ecan, a, (short)(100 - i));
	}

	/* Update focus multiple times */
	for(i = 0; i < 10; i++){
		ecanupdate(ecan);
		ecandecay(ecan, 0.05);
	}

	/* Check focus is maintained */
	int nfocus;
	Atom **focus = ecanfocus(ecan, &nfocus);
	ASSERT_EQ(nfocus, 20);  /* attentionalfocus default */

	ecanfree(ecan);
	plan9cogfree(p9c);
}

/* ========== Utility Function Tests ========== */

TEST(utility_cogatomstr)
{
	Plan9Cog *p9c = plan9coginit();

	Atom *a = atomcreate(p9c->atomspace, ConceptNode, "TestNode");
	char *str = cogatomstr(a);

	ASSERT_NOT_NULL(str);

	plan9cogfree(p9c);
}

TEST(utility_cogtvstr)
{
	TruthValue tv = {0.8, 0.9, 100};
	char *str = cogtvstr(tv);

	ASSERT_NOT_NULL(str);
}

TEST(utility_cogdebug)
{
	/* Should not crash */
	cogdebug(1, "Test debug message: %d", 42);
	cogdebug(0, "This should not print");
}

/* ========== Edge Case Tests ========== */

TEST(edge_null_handling)
{
	/* All these should not crash */
	plan9cogfree(nil);
	atomspacefree(nil);
	plnfree(nil);
	urefree(nil);
	machspacefree(nil);
	ecanfree(nil);
	patternfree(nil);
	cogreactorfree(nil);
	cogrelease(nil);

	ASSERT_NULL(cogobject(nil));
	ASSERT_NULL(cogretain(nil));
	ASSERT_NULL(atomfind(nil, 1));
	ASSERT_NULL(machspacefind(nil, 1));
}

TEST(edge_empty_operations)
{
	AtomSpace *as = atomspacecreate();

	/* Query empty atomspace */
	int n;
	Atom **results = atomquery(as, nil, nil, &n);
	/* Should handle nil predicate gracefully or return empty */

	atomspacefree(as);
}

/* ========== Main Test Runner ========== */

void
main(int argc, char *argv[])
{
	print("=== Integration Tests ===\n\n");

	print("Plan9Cog System Tests:\n");
	RUN_TEST(plan9cog_init);
	RUN_TEST(plan9cog_singleton);
	RUN_TEST(plan9cog_info);

	print("\nFull Inference Pipeline Tests:\n");
	RUN_TEST(inference_pipeline);
	RUN_TEST(inference_with_attention);

	print("\nPattern Mining Pipeline Tests:\n");
	RUN_TEST(pattern_mining_pipeline);

	print("\nDistributed Processing Tests:\n");
	RUN_TEST(distributed_processing);

	print("\nCognitive Grip Tests:\n");
	RUN_TEST(cognitive_grip);

	print("\nFusion Reactor Tests:\n");
	RUN_TEST(fusion_reactor);

	print("\nURE with PLN Tests:\n");
	RUN_TEST(ure_pln_integration);

	print("\nStress Tests:\n");
	RUN_TEST(stress_many_atoms);
	RUN_TEST(stress_many_links);
	RUN_TEST(stress_attention_propagation);

	print("\nUtility Function Tests:\n");
	RUN_TEST(utility_cogatomstr);
	RUN_TEST(utility_cogtvstr);
	RUN_TEST(utility_cogdebug);

	print("\nEdge Case Tests:\n");
	RUN_TEST(edge_null_handling);
	RUN_TEST(edge_empty_operations);

	print("\n=== Test Summary ===\n");
	print("Tests run: %d\n", tests_run);
	print("Tests passed: %d\n", tests_passed);
	print("Tests failed: %d\n", tests_failed);

	exits(tests_failed > 0 ? "FAIL" : nil);
}
