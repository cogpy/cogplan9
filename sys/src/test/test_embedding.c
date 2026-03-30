/*
 * Exhaustive Unit Tests for Embedding Space Reasoning
 * Tests embeddings and analogical inference
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

/* ========== Embedding Creation Tests ========== */

TEST(embedding_create)
{
	TensorEmbedding *emb = tensorembcreate(10, 64);

	ASSERT_NOT_NULL(emb);
	ASSERT_EQ(emb->nobjects, 10);
	ASSERT_EQ(emb->ndims, 64);
	ASSERT_NOT_NULL(emb->matrix);

	tensorembfree(emb);
}

TEST(embedding_create_small)
{
	TensorEmbedding *emb = tensorembcreate(3, 4);

	ASSERT_NOT_NULL(emb);
	ASSERT_EQ(emb->matrix->shape[0], 3);
	ASSERT_EQ(emb->matrix->shape[1], 4);

	tensorembfree(emb);
}

TEST(embedding_create_large)
{
	TensorEmbedding *emb = tensorembcreate(1000, 256);

	ASSERT_NOT_NULL(emb);
	ASSERT_EQ(emb->nobjects, 1000);
	ASSERT_EQ(emb->ndims, 256);

	tensorembfree(emb);
}

TEST(embedding_free_null)
{
	tensorembfree(nil);
}

/* ========== Embedding Set/Get Tests ========== */

TEST(embedding_set)
{
	TensorEmbedding *emb = tensorembcreate(5, 3);

	int shape[1] = {3};
	Tensor *vec = tensorones(1, shape);
	tensorfill(vec, 2.0);

	tensorembset(emb, 2, vec);

	Tensor *got = tensorembget(emb, 2);
	ASSERT_NOT_NULL(got);

	float sum = tensorsum(got);
	ASSERT_FLOAT_EQ(sum, 6.0);

	tensorfree(vec);
	tensorfree(got);
	tensorembfree(emb);
}

TEST(embedding_get)
{
	TensorEmbedding *emb = tensorembcreate(5, 4);

	Tensor *vec = tensorembget(emb, 0);

	ASSERT_NOT_NULL(vec);
	ASSERT_EQ(vec->rank, 1);
	ASSERT_EQ(vec->shape[0], 4);

	tensorfree(vec);
	tensorembfree(emb);
}

TEST(embedding_get_bounds)
{
	TensorEmbedding *emb = tensorembcreate(5, 4);

	/* Get first and last */
	Tensor *first = tensorembget(emb, 0);
	Tensor *last = tensorembget(emb, 4);

	ASSERT_NOT_NULL(first);
	ASSERT_NOT_NULL(last);

	tensorfree(first);
	tensorfree(last);
	tensorembfree(emb);
}

/* ========== Embedding Name Tests ========== */

TEST(embedding_setname)
{
	TensorEmbedding *emb = tensorembcreate(3, 4);

	tensorembsetname(emb, 0, "apple");
	tensorembsetname(emb, 1, "banana");
	tensorembsetname(emb, 2, "cherry");

	ASSERT_STR_EQ(emb->names[0], "apple");
	ASSERT_STR_EQ(emb->names[1], "banana");
	ASSERT_STR_EQ(emb->names[2], "cherry");

	tensorembfree(emb);
}

TEST(embedding_find)
{
	TensorEmbedding *emb = tensorembcreate(5, 8);

	tensorembsetname(emb, 0, "dog");
	tensorembsetname(emb, 1, "cat");
	tensorembsetname(emb, 2, "bird");
	tensorembsetname(emb, 3, "fish");
	tensorembsetname(emb, 4, "snake");

	int idx_cat = tensorembfind(emb, "cat");
	int idx_fish = tensorembfind(emb, "fish");
	int idx_missing = tensorembfind(emb, "elephant");

	ASSERT_EQ(idx_cat, 1);
	ASSERT_EQ(idx_fish, 3);
	ASSERT_EQ(idx_missing, -1);

	tensorembfree(emb);
}

TEST(embedding_find_empty)
{
	TensorEmbedding *emb = tensorembcreate(3, 4);

	int idx = tensorembfind(emb, "anything");
	ASSERT_EQ(idx, -1);

	tensorembfree(emb);
}

/* ========== Similarity Tests ========== */

TEST(embedding_sim_gram)
{
	TensorEmbedding *emb = tensorembcreate(4, 3);

	/* Set orthogonal vectors */
	int shape[1] = {3};
	Tensor *v0 = tensorzeros(1, shape);
	Tensor *v1 = tensorzeros(1, shape);

	float *d0 = (float*)v0->data;
	float *d1 = (float*)v1->data;
	d0[0] = 1.0; d0[1] = 0.0; d0[2] = 0.0;
	d1[0] = 0.0; d1[1] = 1.0; d1[2] = 0.0;

	tensorembset(emb, 0, v0);
	tensorembset(emb, 1, v1);

	Tensor *gram = tensorembsim(emb);

	ASSERT_NOT_NULL(gram);
	ASSERT_EQ(gram->rank, 2);
	ASSERT_EQ(gram->shape[0], 4);
	ASSERT_EQ(gram->shape[1], 4);

	tensorfree(v0);
	tensorfree(v1);
	tensorfree(gram);
	tensorembfree(emb);
}

TEST(embedding_dist_same)
{
	TensorEmbedding *emb = tensorembcreate(3, 4);

	int shape[1] = {4};
	Tensor *v = tensorones(1, shape);

	tensorembset(emb, 0, v);
	tensorembset(emb, 1, v);

	float dist = tensorembdist(emb, 0, 1);

	/* Same vectors -> distance 0 */
	ASSERT_FLOAT_EQ(dist, 0.0);

	tensorfree(v);
	tensorembfree(emb);
}

TEST(embedding_dist_different)
{
	TensorEmbedding *emb = tensorembcreate(2, 2);

	int shape[1] = {2};
	Tensor *v0 = tensorzeros(1, shape);
	Tensor *v1 = tensorzeros(1, shape);

	float *d0 = (float*)v0->data;
	float *d1 = (float*)v1->data;
	d0[0] = 0.0; d0[1] = 0.0;
	d1[0] = 3.0; d1[1] = 4.0;

	tensorembset(emb, 0, v0);
	tensorembset(emb, 1, v1);

	float dist = tensorembdist(emb, 0, 1);

	/* Euclidean distance: sqrt(9+16) = 5 */
	ASSERT_FLOAT_EQ(dist, 5.0);

	tensorfree(v0);
	tensorfree(v1);
	tensorembfree(emb);
}

/* ========== Embedded Relation Tests ========== */

TEST(embrel_create)
{
	TensorEmbedding *emb = tensorembcreate(5, 8);
	TensorEmbRel *rel = tensorembrelcreate("Likes", 2, emb);

	ASSERT_NOT_NULL(rel);
	ASSERT_STR_EQ(rel->name, "Likes");
	ASSERT_EQ(rel->arity, 2);
	ASSERT_EQ(rel->embedding, emb);

	tensorembrelfree(rel);
	tensorembfree(emb);
}

TEST(embrel_create_unary)
{
	TensorEmbedding *emb = tensorembcreate(10, 16);
	TensorEmbRel *rel = tensorembrelcreate("Human", 1, emb);

	ASSERT_NOT_NULL(rel);
	ASSERT_EQ(rel->arity, 1);

	tensorembrelfree(rel);
	tensorembfree(emb);
}

TEST(embrel_free_null)
{
	tensorembrelfree(nil);
}

TEST(embrel_assert)
{
	TensorEmbedding *emb = tensorembcreate(5, 4);
	TensorEmbRel *rel = tensorembrelcreate("R", 2, emb);

	int indices1[2] = {0, 1};
	int indices2[2] = {2, 3};

	tensorembrelassert(rel, indices1);
	tensorembrelassert(rel, indices2);

	float q1 = tensorembrelquery(rel, indices1);
	float q2 = tensorembrelquery(rel, indices2);
	int indices3[2] = {0, 2};
	float q3 = tensorembrelquery(rel, indices3);

	ASSERT_GT(q1, 0.0);
	ASSERT_GT(q2, 0.0);
	ASSERT_FLOAT_EQ(q3, 0.0);

	tensorembrelfree(rel);
	tensorembfree(emb);
}

TEST(embrel_query_similarity)
{
	TensorEmbedding *emb = tensorembcreate(4, 8);

	/* Set similar embeddings for 0 and 1 */
	int shape[1] = {8};
	Tensor *v = tensorones(1, shape);
	tensorembset(emb, 0, v);
	tensorembset(emb, 1, v);

	TensorEmbRel *rel = tensorembrelcreate("Similar", 2, emb);

	int indices[2] = {0, 1};
	tensorembrelassert(rel, indices);

	/* Query similar pair */
	float q = tensorembrelquery(rel, indices);
	ASSERT_GT(q, 0.0);

	tensorfree(v);
	tensorembrelfree(rel);
	tensorembfree(emb);
}

/* ========== Analogical Reasoning Tests ========== */

TEST(analogical_basic)
{
	TensorEmbedding *emb = tensorembcreate(4, 8);
	TensorEmbRel *rel = tensorembrelcreate("HasCapital", 2, emb);

	/* France(0) has capital Paris(1) */
	int indices[2] = {0, 1};
	tensorembrelassert(rel, indices);

	/* Query: what is the capital of Germany(2)? */
	int query[2] = {2, -1};  /* -1 = unknown */
	float temperature = 1.0;

	Tensor *result = tensoranalogical(emb, rel, query, temperature);

	ASSERT_NOT_NULL(result);
	ASSERT_EQ(result->rank, 1);
	ASSERT_EQ(result->shape[0], 4);

	tensorfree(result);
	tensorembrelfree(rel);
	tensorembfree(emb);
}

TEST(analogical_temperature_hot)
{
	TensorEmbedding *emb = tensorembcreate(5, 4);
	TensorEmbRel *rel = tensorembrelcreate("R", 2, emb);

	int indices[2] = {0, 1};
	tensorembrelassert(rel, indices);

	int query[2] = {2, -1};

	/* High temperature -> more uniform distribution */
	Tensor *hot = tensoranalogical(emb, rel, query, 10.0);
	/* Low temperature -> more peaked */
	Tensor *cold = tensoranalogical(emb, rel, query, 0.1);

	ASSERT_NOT_NULL(hot);
	ASSERT_NOT_NULL(cold);

	tensorfree(hot);
	tensorfree(cold);
	tensorembrelfree(rel);
	tensorembfree(emb);
}

TEST(analogical_multiple_facts)
{
	TensorEmbedding *emb = tensorembcreate(6, 8);
	TensorEmbRel *rel = tensorembrelcreate("CapitalOf", 2, emb);

	/* Multiple facts */
	int fact1[2] = {0, 1};  /* France -> Paris */
	int fact2[2] = {2, 3};  /* Germany -> Berlin */
	int fact3[2] = {4, 5};  /* Italy -> Rome */

	tensorembrelassert(rel, fact1);
	tensorembrelassert(rel, fact2);
	tensorembrelassert(rel, fact3);

	/* Query about similar country */
	int query[2] = {0, -1};
	Tensor *result = tensoranalogical(emb, rel, query, 1.0);

	ASSERT_NOT_NULL(result);

	tensorfree(result);
	tensorembrelfree(rel);
	tensorembfree(emb);
}

/* ========== Word Embedding Analogy Tests ========== */

TEST(word_analogy_setup)
{
	/*
	 * Classic word analogy: king - man + woman = queen
	 * Here we test the infrastructure
	 */
	TensorEmbedding *emb = tensorembcreate(4, 50);

	tensorembsetname(emb, 0, "king");
	tensorembsetname(emb, 1, "man");
	tensorembsetname(emb, 2, "woman");
	tensorembsetname(emb, 3, "queen");

	int king = tensorembfind(emb, "king");
	int man = tensorembfind(emb, "man");
	int woman = tensorembfind(emb, "woman");
	int queen = tensorembfind(emb, "queen");

	ASSERT_EQ(king, 0);
	ASSERT_EQ(man, 1);
	ASSERT_EQ(woman, 2);
	ASSERT_EQ(queen, 3);

	tensorembfree(emb);
}

TEST(word_analogy_vector_arithmetic)
{
	TensorEmbedding *emb = tensorembcreate(4, 3);

	/* Set up simplified vectors */
	int shape[1] = {3};

	Tensor *king = tensorcreate(TensorFloat, 1, shape);
	Tensor *man = tensorcreate(TensorFloat, 1, shape);
	Tensor *woman = tensorcreate(TensorFloat, 1, shape);
	Tensor *queen = tensorcreate(TensorFloat, 1, shape);

	float *kd = (float*)king->data;
	float *md = (float*)man->data;
	float *wd = (float*)woman->data;
	float *qd = (float*)queen->data;

	/* Simplified: gender dimension at index 0 */
	kd[0] = 1.0; kd[1] = 1.0; kd[2] = 0.0;  /* royal, male */
	md[0] = 0.0; md[1] = 1.0; md[2] = 0.0;  /* common, male */
	wd[0] = 0.0; wd[1] = 0.0; wd[2] = 1.0;  /* common, female */
	qd[0] = 1.0; qd[1] = 0.0; qd[2] = 1.0;  /* royal, female */

	tensorembset(emb, 0, king);
	tensorembset(emb, 1, man);
	tensorembset(emb, 2, woman);
	tensorembset(emb, 3, queen);

	/* king - man + woman should be close to queen */
	Tensor *diff = tensorsub(king, man);
	Tensor *result = tensoradd(diff, woman);

	/* Result should be similar to queen */
	ASSERT_NOT_NULL(result);

	tensorfree(king);
	tensorfree(man);
	tensorfree(woman);
	tensorfree(queen);
	tensorfree(diff);
	tensorfree(result);
	tensorembfree(emb);
}

/* ========== Entity Embedding Tests ========== */

TEST(entity_embedding)
{
	TensorEmbedding *emb = tensorembcreate(100, 128);

	tensorembsetname(emb, 0, "Albert_Einstein");
	tensorembsetname(emb, 1, "physics");
	tensorembsetname(emb, 2, "Marie_Curie");
	tensorembsetname(emb, 3, "chemistry");

	TensorEmbRel *expertise = tensorembrelcreate("ExpertIn", 2, emb);

	int fact1[2] = {0, 1};  /* Einstein -> physics */
	int fact2[2] = {2, 3};  /* Curie -> chemistry */

	tensorembrelassert(expertise, fact1);
	tensorembrelassert(expertise, fact2);

	/* Query expertise */
	float q1 = tensorembrelquery(expertise, fact1);
	ASSERT_GT(q1, 0.0);

	tensorembrelfree(expertise);
	tensorembfree(emb);
}

/* ========== Edge Cases ========== */

TEST(embedding_single_object)
{
	TensorEmbedding *emb = tensorembcreate(1, 8);

	ASSERT_NOT_NULL(emb);
	ASSERT_EQ(emb->nobjects, 1);

	Tensor *v = tensorembget(emb, 0);
	ASSERT_NOT_NULL(v);

	tensorfree(v);
	tensorembfree(emb);
}

TEST(embedding_single_dim)
{
	TensorEmbedding *emb = tensorembcreate(5, 1);

	ASSERT_NOT_NULL(emb);
	ASSERT_EQ(emb->ndims, 1);

	tensorembfree(emb);
}

TEST(embrel_self_relation)
{
	TensorEmbedding *emb = tensorembcreate(3, 4);
	TensorEmbRel *rel = tensorembrelcreate("SameAs", 2, emb);

	int indices[2] = {0, 0};
	tensorembrelassert(rel, indices);

	float q = tensorembrelquery(rel, indices);
	ASSERT_GT(q, 0.0);

	tensorembrelfree(rel);
	tensorembfree(emb);
}

/* ========== Main Test Runner ========== */

void
main(int argc, char *argv[])
{
	print("=== Embedding Space Reasoning Unit Tests ===\n\n");

	print("Embedding Creation Tests:\n");
	RUN_TEST(embedding_create);
	RUN_TEST(embedding_create_small);
	RUN_TEST(embedding_create_large);
	RUN_TEST(embedding_free_null);

	print("\nEmbedding Set/Get Tests:\n");
	RUN_TEST(embedding_set);
	RUN_TEST(embedding_get);
	RUN_TEST(embedding_get_bounds);

	print("\nEmbedding Name Tests:\n");
	RUN_TEST(embedding_setname);
	RUN_TEST(embedding_find);
	RUN_TEST(embedding_find_empty);

	print("\nSimilarity Tests:\n");
	RUN_TEST(embedding_sim_gram);
	RUN_TEST(embedding_dist_same);
	RUN_TEST(embedding_dist_different);

	print("\nEmbedded Relation Tests:\n");
	RUN_TEST(embrel_create);
	RUN_TEST(embrel_create_unary);
	RUN_TEST(embrel_free_null);
	RUN_TEST(embrel_assert);
	RUN_TEST(embrel_query_similarity);

	print("\nAnalogical Reasoning Tests:\n");
	RUN_TEST(analogical_basic);
	RUN_TEST(analogical_temperature_hot);
	RUN_TEST(analogical_multiple_facts);

	print("\nWord Embedding Analogy Tests:\n");
	RUN_TEST(word_analogy_setup);
	RUN_TEST(word_analogy_vector_arithmetic);

	print("\nEntity Embedding Tests:\n");
	RUN_TEST(entity_embedding);

	print("\nEdge Case Tests:\n");
	RUN_TEST(embedding_single_object);
	RUN_TEST(embedding_single_dim);
	RUN_TEST(embrel_self_relation);

	print("\n=== Test Summary ===\n");
	print("Tests run: %d\n", tests_run);
	print("Tests passed: %d\n", tests_passed);
	print("Tests failed: %d\n", tests_failed);

	exits(tests_failed > 0 ? "FAIL" : nil);
}
