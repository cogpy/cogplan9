/*
 * Exhaustive Unit Tests for MachSpace (Distributed Hypergraph)
 * Tests distributed atom operations and synchronization
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

/* ========== MachSpace Initialization Tests ========== */

TEST(machspace_init)
{
	AtomSpace *as = atomspacecreate();
	MachSpace *ms = machspaceinit(as);

	ASSERT_NOT_NULL(ms);
	ASSERT_EQ(ms->local, as);
	ASSERT_EQ(ms->nremote, 0);
	ASSERT_NULL(ms->remote);
	ASSERT_NULL(ms->hosts);

	machspacefree(ms);
	atomspacefree(as);
}

TEST(machspace_init_null)
{
	MachSpace *ms = machspaceinit(nil);

	ASSERT_NOT_NULL(ms);
	ASSERT_NULL(ms->local);

	machspacefree(ms);
}

TEST(machspace_free_null)
{
	/* Should not crash */
	machspacefree(nil);
}

/* ========== MachSpace Connect Tests ========== */

TEST(machspace_connect_single)
{
	AtomSpace *as = atomspacecreate();
	MachSpace *ms = machspaceinit(as);

	int result = machspaceconnect(ms, "host1");

	ASSERT_EQ(result, 0);
	ASSERT_EQ(ms->nremote, 1);
	ASSERT_NOT_NULL(ms->hosts);
	ASSERT_STR_EQ(ms->hosts[0], "host1");

	machspacefree(ms);
	atomspacefree(as);
}

TEST(machspace_connect_multiple)
{
	AtomSpace *as = atomspacecreate();
	MachSpace *ms = machspaceinit(as);

	machspaceconnect(ms, "host1");
	machspaceconnect(ms, "host2");
	machspaceconnect(ms, "host3");

	ASSERT_EQ(ms->nremote, 3);
	ASSERT_STR_EQ(ms->hosts[0], "host1");
	ASSERT_STR_EQ(ms->hosts[1], "host2");
	ASSERT_STR_EQ(ms->hosts[2], "host3");

	machspacefree(ms);
	atomspacefree(as);
}

TEST(machspace_connect_null_ms)
{
	int result = machspaceconnect(nil, "host1");
	ASSERT_EQ(result, -1);
}

TEST(machspace_connect_null_host)
{
	AtomSpace *as = atomspacecreate();
	MachSpace *ms = machspaceinit(as);

	int result = machspaceconnect(ms, nil);
	ASSERT_EQ(result, -1);

	machspacefree(ms);
	atomspacefree(as);
}

/* ========== MachSpace Find Tests ========== */

TEST(machspace_find_local)
{
	AtomSpace *as = atomspacecreate();
	MachSpace *ms = machspaceinit(as);

	Atom *a = atomcreate(as, ConceptNode, "test");
	Atom *found = machspacefind(ms, a->id);

	ASSERT_EQ(found, a);

	machspacefree(ms);
	atomspacefree(as);
}

TEST(machspace_find_nonexistent)
{
	AtomSpace *as = atomspacecreate();
	MachSpace *ms = machspaceinit(as);

	Atom *found = machspacefind(ms, 99999);
	ASSERT_NULL(found);

	machspacefree(ms);
	atomspacefree(as);
}

TEST(machspace_find_null)
{
	Atom *found = machspacefind(nil, 1);
	ASSERT_NULL(found);
}

/* ========== MachSpace Sync Tests ========== */

TEST(machspace_sync_empty)
{
	AtomSpace *as = atomspacecreate();
	MachSpace *ms = machspaceinit(as);

	int result = machspacesync(ms);
	ASSERT_EQ(result, 0);

	machspacefree(ms);
	atomspacefree(as);
}

TEST(machspace_sync_null)
{
	int result = machspacesync(nil);
	ASSERT_EQ(result, -1);
}

TEST(machspace_sync_with_remotes)
{
	AtomSpace *local = atomspacecreate();
	AtomSpace *remote1 = atomspacecreate();
	MachSpace *ms = machspaceinit(local);

	/* Manually set up a remote atomspace for testing */
	machspaceconnect(ms, "remote1");
	ms->remote[0] = remote1;

	/* Create atoms in local */
	atomcreate(local, ConceptNode, "local_atom");

	/* Create atoms in remote */
	atomcreate(remote1, ConceptNode, "remote_atom");

	/* Sync */
	int result = machspacesync(ms);
	ASSERT_GT(result, 0);

	/* Clean up - don't double-free remote1 */
	ms->remote[0] = nil;
	machspacefree(ms);
	atomspacefree(remote1);
	atomspacefree(local);
}

/* ========== Integration Tests ========== */

TEST(machspace_full_workflow)
{
	AtomSpace *as = atomspacecreate();
	MachSpace *ms = machspaceinit(as);

	/* Connect to multiple hosts */
	machspaceconnect(ms, "node1.example.com");
	machspaceconnect(ms, "node2.example.com");

	ASSERT_EQ(ms->nremote, 2);

	/* Create atoms */
	Atom *a1 = atomcreate(as, ConceptNode, "concept1");
	Atom *a2 = atomcreate(as, ConceptNode, "concept2");

	/* Find atoms */
	ASSERT_EQ(machspacefind(ms, a1->id), a1);
	ASSERT_EQ(machspacefind(ms, a2->id), a2);

	/* Sync (with stub remotes) */
	machspacesync(ms);

	machspacefree(ms);
	atomspacefree(as);
}

TEST(machspace_distributed_find)
{
	AtomSpace *local = atomspacecreate();
	AtomSpace *remote = atomspacecreate();
	MachSpace *ms = machspaceinit(local);

	/* Create local atom */
	Atom *local_atom = atomcreate(local, ConceptNode, "local");

	/* Create remote atom and add remote space */
	Atom *remote_atom = atomcreate(remote, ConceptNode, "remote");
	machspaceconnect(ms, "remote");
	ms->remote[0] = remote;

	/* Find local atom */
	ASSERT_EQ(machspacefind(ms, local_atom->id), local_atom);

	/* Find remote atom */
	ASSERT_EQ(machspacefind(ms, remote_atom->id), remote_atom);

	/* Clean up */
	ms->remote[0] = nil;
	machspacefree(ms);
	atomspacefree(remote);
	atomspacefree(local);
}

/* ========== Main Test Runner ========== */

void
main(int argc, char *argv[])
{
	print("=== MachSpace Unit Tests ===\n\n");

	print("MachSpace Initialization Tests:\n");
	RUN_TEST(machspace_init);
	RUN_TEST(machspace_init_null);
	RUN_TEST(machspace_free_null);

	print("\nMachSpace Connect Tests:\n");
	RUN_TEST(machspace_connect_single);
	RUN_TEST(machspace_connect_multiple);
	RUN_TEST(machspace_connect_null_ms);
	RUN_TEST(machspace_connect_null_host);

	print("\nMachSpace Find Tests:\n");
	RUN_TEST(machspace_find_local);
	RUN_TEST(machspace_find_nonexistent);
	RUN_TEST(machspace_find_null);

	print("\nMachSpace Sync Tests:\n");
	RUN_TEST(machspace_sync_empty);
	RUN_TEST(machspace_sync_null);
	RUN_TEST(machspace_sync_with_remotes);

	print("\nIntegration Tests:\n");
	RUN_TEST(machspace_full_workflow);
	RUN_TEST(machspace_distributed_find);

	print("\n=== Test Summary ===\n");
	print("Tests run: %d\n", tests_run);
	print("Tests passed: %d\n", tests_passed);
	print("Tests failed: %d\n", tests_failed);

	exits(tests_failed > 0 ? "FAIL" : nil);
}
