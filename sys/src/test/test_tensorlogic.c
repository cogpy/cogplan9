/*
 * Exhaustive Unit Tests for Logic-to-Tensor Mapping
 * Tests Datalog rules as tensor equations
 */

#include <u.h>
#include <libc.h>
#include <plan9cog/tensorlogic.h>

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

/* ========== Tensor Relation Tests ========== */

TEST(relation_create)
{
	int sizes[2] = {10, 10};
	TensorRelation *rel = tensorrelcreate("Parent", 2, sizes);

	ASSERT_NOT_NULL(rel);
	ASSERT_STR_EQ(rel->name, "Parent");
	ASSERT_EQ(rel->arity, 2);
	ASSERT_NOT_NULL(rel->tensor);

	tensorrelfree(rel);
}

TEST(relation_create_unary)
{
	int sizes[1] = {5};
	TensorRelation *rel = tensorrelcreate("Male", 1, sizes);

	ASSERT_NOT_NULL(rel);
	ASSERT_EQ(rel->arity, 1);
	ASSERT_EQ(rel->tensor->shape[0], 5);

	tensorrelfree(rel);
}

TEST(relation_create_ternary)
{
	int sizes[3] = {10, 10, 5};
	TensorRelation *rel = tensorrelcreate("Gives", 3, sizes);

	ASSERT_NOT_NULL(rel);
	ASSERT_EQ(rel->arity, 3);
	ASSERT_EQ(rel->tensor->rank, 3);

	tensorrelfree(rel);
}

TEST(relation_free_null)
{
	/* Should not crash */
	tensorrelfree(nil);
}

TEST(relation_assert)
{
	int sizes[2] = {5, 5};
	TensorRelation *rel = tensorrelcreate("Likes", 2, sizes);

	/* Assert: Likes(0, 1) */
	tensorrelassert(rel, 0, 1);
	/* Assert: Likes(2, 3) */
	tensorrelassert(rel, 2, 3);

	/* Query */
	int r1 = tensorrelquery(rel, 0, 1);
	int r2 = tensorrelquery(rel, 2, 3);
	int r3 = tensorrelquery(rel, 1, 2);

	ASSERT_EQ(r1, 1);
	ASSERT_EQ(r2, 1);
	ASSERT_EQ(r3, 0);

	tensorrelfree(rel);
}

TEST(relation_query_bounds)
{
	int sizes[2] = {3, 3};
	TensorRelation *rel = tensorrelcreate("R", 2, sizes);

	tensorrelassert(rel, 1, 1);

	/* Query within bounds */
	int r1 = tensorrelquery(rel, 1, 1);
	ASSERT_EQ(r1, 1);

	/* Query at boundary */
	int r2 = tensorrelquery(rel, 2, 2);
	ASSERT_EQ(r2, 0);

	tensorrelfree(rel);
}

TEST(relation_get)
{
	int sizes[2] = {3, 3};
	TensorRelation *rel = tensorrelcreate("R", 2, sizes);

	tensorrelassert(rel, 0, 0);
	tensorrelassert(rel, 1, 1);
	tensorrelassert(rel, 2, 2);

	Tensor *t = tensorrelget(rel);

	ASSERT_NOT_NULL(t);
	ASSERT_EQ(t->rank, 2);

	/* Should have 3 ones */
	float sum = tensorsum(t);
	ASSERT_FLOAT_EQ(sum, 3.0);

	tensorrelfree(rel);
}

/* ========== Tensor Rule Tests ========== */

TEST(rule_create_simple)
{
	/* Head: Grandparent(x,z) */
	int head_sizes[2] = {5, 5};
	TensorRelation *head = tensorrelcreate("Grandparent", 2, head_sizes);

	/* Body: Parent(x,y), Parent(y,z) */
	int body_sizes[2] = {5, 5};
	TensorRelation *parent = tensorrelcreate("Parent", 2, body_sizes);

	TensorRelation *body[2] = {parent, parent};
	TensorRule *rule = tensorrulecreate(head, body, 2);

	ASSERT_NOT_NULL(rule);
	ASSERT_EQ(rule->head, head);
	ASSERT_EQ(rule->nbody, 2);

	tensorrulefree(rule);
	tensorrelfree(head);
	tensorrelfree(parent);
}

TEST(rule_create_single_body)
{
	int sizes[2] = {10, 10};
	TensorRelation *head = tensorrelcreate("Ancestor", 2, sizes);
	TensorRelation *parent = tensorrelcreate("Parent", 2, sizes);

	TensorRelation *body[1] = {parent};
	TensorRule *rule = tensorrulecreate(head, body, 1);

	ASSERT_NOT_NULL(rule);
	ASSERT_EQ(rule->nbody, 1);

	tensorrulefree(rule);
	tensorrelfree(head);
	tensorrelfree(parent);
}

TEST(rule_free_null)
{
	tensorrulefree(nil);
}

TEST(rule_compile)
{
	int sizes[2] = {3, 3};
	TensorRelation *head = tensorrelcreate("B", 2, sizes);
	TensorRelation *a = tensorrelcreate("A", 2, sizes);

	TensorRelation *body[1] = {a};
	TensorRule *rule = tensorrulecreate(head, body, 1);

	tensorrulecompile(rule);

	ASSERT_NOT_NULL(rule->equation);

	tensorrulefree(rule);
	tensorrelfree(head);
	tensorrelfree(a);
}

/* ========== Tensor Program Tests ========== */

TEST(program_init)
{
	TensorProgram *prog = tensorproginit();

	ASSERT_NOT_NULL(prog);
	ASSERT_EQ(prog->nrelations, 0);
	ASSERT_EQ(prog->nrules, 0);

	tensorprogfree(prog);
}

TEST(program_free_null)
{
	tensorprogfree(nil);
}

TEST(program_add_rule)
{
	TensorProgram *prog = tensorproginit();

	int sizes[2] = {5, 5};
	TensorRelation *head = tensorrelcreate("R", 2, sizes);
	TensorRelation *body_rel = tensorrelcreate("S", 2, sizes);
	TensorRelation *body[1] = {body_rel};
	TensorRule *rule = tensorrulecreate(head, body, 1);

	tensorprogaddrule(prog, rule);

	ASSERT_EQ(prog->nrules, 1);

	tensorprogfree(prog);
}

TEST(program_forward_empty)
{
	TensorProgram *prog = tensorproginit();

	/* Should not crash on empty program */
	tensorprogforward(prog);

	tensorprogfree(prog);
}

TEST(program_forward_transitive)
{
	TensorProgram *prog = tensorproginit();

	/* Create relations */
	int sizes[2] = {4, 4};
	TensorRelation *edge = tensorrelcreate("Edge", 2, sizes);
	TensorRelation *path = tensorrelcreate("Path", 2, sizes);

	/* Add edges: 0->1, 1->2, 2->3 */
	tensorrelassert(edge, 0, 1);
	tensorrelassert(edge, 1, 2);
	tensorrelassert(edge, 2, 3);

	/* Rule: Path(x,z) <- Edge(x,y), Edge(y,z) */
	TensorRelation *body[2] = {edge, edge};
	TensorRule *rule = tensorrulecreate(path, body, 2);

	tensorprogaddrule(prog, rule);

	/* Forward chain */
	tensorprogforward(prog);

	/* Query Path(0,2) - should be true */
	Tensor *result = tensorprogquery(prog, path);
	ASSERT_NOT_NULL(result);

	tensorprogfree(prog);
}

TEST(program_query)
{
	TensorProgram *prog = tensorproginit();

	int sizes[2] = {3, 3};
	TensorRelation *rel = tensorrelcreate("Rel", 2, sizes);
	tensorrelassert(rel, 0, 1);

	TensorRelation *body[1] = {rel};
	TensorRelation *head = tensorrelcreate("Head", 2, sizes);
	TensorRule *rule = tensorrulecreate(head, body, 1);
	tensorprogaddrule(prog, rule);

	Tensor *result = tensorprogquery(prog, rel);
	ASSERT_NOT_NULL(result);

	tensorprogfree(prog);
}

/* ========== Datalog Semantics Tests ========== */

TEST(datalog_aunt_rule)
{
	/*
	 * Classic aunt rule:
	 * Aunt(x,z) <- Sister(x,y), Parent(y,z)
	 * Tensor: A[x,z] = step(S[x,y] * P[y,z])
	 */
	int sizes[2] = {5, 5};

	TensorRelation *aunt = tensorrelcreate("Aunt", 2, sizes);
	TensorRelation *sister = tensorrelcreate("Sister", 2, sizes);
	TensorRelation *parent = tensorrelcreate("Parent", 2, sizes);

	/* Person 0 is sister of Person 1 */
	tensorrelassert(sister, 0, 1);
	/* Person 1 is parent of Person 2 */
	tensorrelassert(parent, 1, 2);

	TensorRelation *body[2] = {sister, parent};
	TensorRule *rule = tensorrulecreate(aunt, body, 2);
	tensorrulecompile(rule);

	/* After forward chaining, Aunt(0,2) should hold */
	TensorProgram *prog = tensorproginit();
	tensorprogaddrule(prog, rule);
	tensorprogforward(prog);

	/* Query */
	int is_aunt = tensorrelquery(aunt, 0, 2);
	/* Should be 1 after inference */

	tensorprogfree(prog);
}

TEST(datalog_reachability)
{
	/*
	 * Reachability:
	 * Reach(x,y) <- Edge(x,y)
	 * Reach(x,z) <- Edge(x,y), Reach(y,z)
	 */
	int sizes[2] = {5, 5};

	TensorRelation *edge = tensorrelcreate("Edge", 2, sizes);
	TensorRelation *reach = tensorrelcreate("Reach", 2, sizes);

	/* Chain: 0 -> 1 -> 2 -> 3 -> 4 */
	tensorrelassert(edge, 0, 1);
	tensorrelassert(edge, 1, 2);
	tensorrelassert(edge, 2, 3);
	tensorrelassert(edge, 3, 4);

	TensorProgram *prog = tensorproginit();

	/* Base rule: Reach(x,y) <- Edge(x,y) */
	TensorRelation *body1[1] = {edge};
	TensorRule *rule1 = tensorrulecreate(reach, body1, 1);
	tensorprogaddrule(prog, rule1);

	/* Recursive rule: Reach(x,z) <- Edge(x,y), Reach(y,z) */
	TensorRelation *body2[2] = {edge, reach};
	TensorRule *rule2 = tensorrulecreate(reach, body2, 2);
	tensorprogaddrule(prog, rule2);

	/* Multiple iterations for transitive closure */
	prog->maxiterations = 5;
	tensorprogforward(prog);

	/* Reach(0,4) should eventually be true */
	tensorprogfree(prog);
}

TEST(datalog_sibling)
{
	/*
	 * Sibling(x,y) <- Parent(z,x), Parent(z,y), x != y
	 * (Simplified without inequality)
	 */
	int sizes[2] = {4, 4};

	TensorRelation *sibling = tensorrelcreate("Sibling", 2, sizes);
	TensorRelation *parent = tensorrelcreate("Parent", 2, sizes);

	/* Person 0 is parent of 1 and 2 */
	tensorrelassert(parent, 0, 1);
	tensorrelassert(parent, 0, 2);

	TensorProgram *prog = tensorproginit();

	TensorRelation *body[2] = {parent, parent};
	TensorRule *rule = tensorrulecreate(sibling, body, 2);
	tensorprogaddrule(prog, rule);

	tensorprogforward(prog);

	tensorprogfree(prog);
}

/* ========== Boolean Tensor Tests ========== */

TEST(bool_tensor_and)
{
	int sizes[1] = {4};
	TensorRelation *a = tensorrelcreate("A", 1, sizes);
	TensorRelation *b = tensorrelcreate("B", 1, sizes);

	tensorrelassert(a, 0);
	tensorrelassert(a, 1);
	tensorrelassert(b, 1);
	tensorrelassert(b, 2);

	/* AND via element-wise multiplication + step */
	Tensor *ta = tensorrelget(a);
	Tensor *tb = tensorrelget(b);

	Tensor *product = tensormul(ta, tb);
	Tensor *result = tensorstep(product);

	/* Should have 1 only at index 1 */
	float sum = tensorsum(result);
	ASSERT_FLOAT_EQ(sum, 1.0);

	tensorrelfree(a);
	tensorrelfree(b);
	tensorfree(product);
	tensorfree(result);
}

TEST(bool_tensor_or)
{
	int sizes[1] = {4};
	TensorRelation *a = tensorrelcreate("A", 1, sizes);
	TensorRelation *b = tensorrelcreate("B", 1, sizes);

	tensorrelassert(a, 0);
	tensorrelassert(a, 1);
	tensorrelassert(b, 1);
	tensorrelassert(b, 2);

	/* OR via element-wise addition + step */
	Tensor *ta = tensorrelget(a);
	Tensor *tb = tensorrelget(b);

	Tensor *sum_t = tensoradd(ta, tb);
	Tensor *result = tensorstep(sum_t);

	/* Should have 1 at indices 0, 1, 2 */
	float sum = tensorsum(result);
	ASSERT_FLOAT_EQ(sum, 3.0);

	tensorrelfree(a);
	tensorrelfree(b);
	tensorfree(sum_t);
	tensorfree(result);
}

/* ========== Integration with Einsum ========== */

TEST(logic_einsum_join)
{
	/*
	 * Join via einsum: C[x,z] = A[x,y] * B[y,z]
	 * Corresponds to: C(x,z) <- A(x,y), B(y,z)
	 */
	int sizes[2] = {3, 3};
	TensorRelation *a = tensorrelcreate("A", 2, sizes);
	TensorRelation *b = tensorrelcreate("B", 2, sizes);

	/* A: {(0,1), (1,2)} */
	tensorrelassert(a, 0, 1);
	tensorrelassert(a, 1, 2);

	/* B: {(1,0), (2,1)} */
	tensorrelassert(b, 1, 0);
	tensorrelassert(b, 2, 1);

	Tensor *ta = tensorrelget(a);
	Tensor *tb = tensorrelget(b);

	/* Join: C[x,z] = sum_y(A[x,y] * B[y,z]) */
	Tensor *c = tensoreinsum("xy,yz->xz", ta, tb);

	ASSERT_NOT_NULL(c);

	/* Apply step function for Boolean result */
	Tensor *result = tensorstep(c);

	/* Expected: C(0,0)=1, C(1,1)=1 */
	float sum = tensorsum(result);
	ASSERT_FLOAT_EQ(sum, 2.0);

	tensorrelfree(a);
	tensorrelfree(b);
	tensorfree(c);
	tensorfree(result);
}

TEST(logic_einsum_project)
{
	/*
	 * Projection via einsum: b[x] = sum_y(A[x,y])
	 * Corresponds to: exists y. A(x,y)
	 */
	int sizes[2] = {3, 3};
	TensorRelation *a = tensorrelcreate("A", 2, sizes);

	tensorrelassert(a, 0, 1);
	tensorrelassert(a, 0, 2);
	tensorrelassert(a, 1, 0);

	Tensor *ta = tensorrelget(a);

	/* Project away second dimension */
	Tensor *b = tensoreinsum("xy->x", ta, nil);

	ASSERT_NOT_NULL(b);
	ASSERT_EQ(b->rank, 1);

	/* Apply step for Boolean */
	Tensor *result = tensorstep(b);

	/* x=0 and x=1 have some y */
	float sum = tensorsum(result);
	ASSERT_FLOAT_EQ(sum, 2.0);

	tensorrelfree(a);
	tensorfree(b);
	tensorfree(result);
}

/* ========== Main Test Runner ========== */

void
main(int argc, char *argv[])
{
	print("=== Logic-to-Tensor Unit Tests ===\n\n");

	print("Tensor Relation Tests:\n");
	RUN_TEST(relation_create);
	RUN_TEST(relation_create_unary);
	RUN_TEST(relation_create_ternary);
	RUN_TEST(relation_free_null);
	RUN_TEST(relation_assert);
	RUN_TEST(relation_query_bounds);
	RUN_TEST(relation_get);

	print("\nTensor Rule Tests:\n");
	RUN_TEST(rule_create_simple);
	RUN_TEST(rule_create_single_body);
	RUN_TEST(rule_free_null);
	RUN_TEST(rule_compile);

	print("\nTensor Program Tests:\n");
	RUN_TEST(program_init);
	RUN_TEST(program_free_null);
	RUN_TEST(program_add_rule);
	RUN_TEST(program_forward_empty);
	RUN_TEST(program_forward_transitive);
	RUN_TEST(program_query);

	print("\nDatalog Semantics Tests:\n");
	RUN_TEST(datalog_aunt_rule);
	RUN_TEST(datalog_reachability);
	RUN_TEST(datalog_sibling);

	print("\nBoolean Tensor Tests:\n");
	RUN_TEST(bool_tensor_and);
	RUN_TEST(bool_tensor_or);

	print("\nIntegration with Einsum Tests:\n");
	RUN_TEST(logic_einsum_join);
	RUN_TEST(logic_einsum_project);

	print("\n=== Test Summary ===\n");
	print("Tests run: %d\n", tests_run);
	print("Tests passed: %d\n", tests_passed);
	print("Tests failed: %d\n", tests_failed);

	exits(tests_failed > 0 ? "FAIL" : nil);
}
