/*
 * Exhaustive Unit Tests for Graphical Models
 * Tests factor graphs and probabilistic inference
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

/* ========== Factor Creation Tests ========== */

TEST(factor_create_binary)
{
	int vars[2] = {0, 1};
	int sizes[2] = {2, 2};

	TensorFactor *f = tensorfactorcreate("phi", vars, 2, sizes);

	ASSERT_NOT_NULL(f);
	ASSERT_STR_EQ(f->name, "phi");
	ASSERT_EQ(f->nvars, 2);
	ASSERT_NOT_NULL(f->values);

	tensorfactorfree(f);
}

TEST(factor_create_unary)
{
	int vars[1] = {0};
	int sizes[1] = {3};

	TensorFactor *f = tensorfactorcreate("psi", vars, 1, sizes);

	ASSERT_NOT_NULL(f);
	ASSERT_EQ(f->nvars, 1);
	ASSERT_EQ(f->values->shape[0], 3);

	tensorfactorfree(f);
}

TEST(factor_create_ternary)
{
	int vars[3] = {0, 1, 2};
	int sizes[3] = {2, 2, 2};

	TensorFactor *f = tensorfactorcreate("theta", vars, 3, sizes);

	ASSERT_NOT_NULL(f);
	ASSERT_EQ(f->nvars, 3);
	ASSERT_EQ(f->values->nelems, 8);

	tensorfactorfree(f);
}

TEST(factor_free_null)
{
	tensorfactorfree(nil);
}

/* ========== Factor Value Tests ========== */

TEST(factor_set_get)
{
	int vars[2] = {0, 1};
	int sizes[2] = {2, 2};

	TensorFactor *f = tensorfactorcreate("phi", vars, 2, sizes);

	int indices[2] = {0, 1};
	tensorfactorset(f, indices, 0.75);

	float val = tensorfactorget(f, indices);
	ASSERT_FLOAT_EQ(val, 0.75);

	tensorfactorfree(f);
}

TEST(factor_set_all)
{
	int vars[2] = {0, 1};
	int sizes[2] = {2, 2};

	TensorFactor *f = tensorfactorcreate("phi", vars, 2, sizes);

	int i, j;
	for(i = 0; i < 2; i++){
		for(j = 0; j < 2; j++){
			int indices[2] = {i, j};
			tensorfactorset(f, indices, (float)(i + j));
		}
	}

	int idx00[2] = {0, 0};
	int idx01[2] = {0, 1};
	int idx10[2] = {1, 0};
	int idx11[2] = {1, 1};

	ASSERT_FLOAT_EQ(tensorfactorget(f, idx00), 0.0);
	ASSERT_FLOAT_EQ(tensorfactorget(f, idx01), 1.0);
	ASSERT_FLOAT_EQ(tensorfactorget(f, idx10), 1.0);
	ASSERT_FLOAT_EQ(tensorfactorget(f, idx11), 2.0);

	tensorfactorfree(f);
}

TEST(factor_default_uniform)
{
	int vars[2] = {0, 1};
	int sizes[2] = {2, 2};

	TensorFactor *f = tensorfactorcreate("phi", vars, 2, sizes);

	/* Default should be uniform (all 1s) */
	float sum = tensorsum(f->values);
	ASSERT_FLOAT_EQ(sum, 4.0);

	tensorfactorfree(f);
}

/* ========== Factor Graph Tests ========== */

TEST(fg_init)
{
	int sizes[3] = {2, 2, 2};
	TensorFactorGraph *fg = tensorfginit(3, sizes);

	ASSERT_NOT_NULL(fg);
	ASSERT_EQ(fg->nvars, 3);
	ASSERT_EQ(fg->nfactors, 0);

	tensorfgfree(fg);
}

TEST(fg_init_different_sizes)
{
	int sizes[4] = {2, 3, 4, 2};
	TensorFactorGraph *fg = tensorfginit(4, sizes);

	ASSERT_NOT_NULL(fg);
	ASSERT_EQ(fg->varsizes[0], 2);
	ASSERT_EQ(fg->varsizes[1], 3);
	ASSERT_EQ(fg->varsizes[2], 4);
	ASSERT_EQ(fg->varsizes[3], 2);

	tensorfgfree(fg);
}

TEST(fg_free_null)
{
	tensorfgfree(nil);
}

TEST(fg_add_factor)
{
	int sizes[2] = {2, 2};
	TensorFactorGraph *fg = tensorfginit(2, sizes);

	int vars[2] = {0, 1};
	TensorFactor *f = tensorfactorcreate("phi01", vars, 2, sizes);

	tensorfgaddfactor(fg, f);

	ASSERT_EQ(fg->nfactors, 1);
	ASSERT_EQ(fg->factors[0], f);

	tensorfgfree(fg);
}

TEST(fg_add_multiple_factors)
{
	int sizes[3] = {2, 2, 2};
	TensorFactorGraph *fg = tensorfginit(3, sizes);

	int vars01[2] = {0, 1};
	int vars12[2] = {1, 2};
	int fsize[2] = {2, 2};

	TensorFactor *f1 = tensorfactorcreate("phi01", vars01, 2, fsize);
	TensorFactor *f2 = tensorfactorcreate("phi12", vars12, 2, fsize);

	tensorfgaddfactor(fg, f1);
	tensorfgaddfactor(fg, f2);

	ASSERT_EQ(fg->nfactors, 2);

	tensorfgfree(fg);
}

/* ========== Partition Function Tests ========== */

TEST(fg_partition_uniform)
{
	int sizes[2] = {2, 2};
	TensorFactorGraph *fg = tensorfginit(2, sizes);

	/* No factors -> uniform, partition = 4 (2*2) */
	float Z = tensorfgpartition(fg);
	ASSERT_FLOAT_EQ(Z, 4.0);

	tensorfgfree(fg);
}

TEST(fg_partition_single_factor)
{
	int sizes[2] = {2, 2};
	TensorFactorGraph *fg = tensorfginit(2, sizes);

	int vars[2] = {0, 1};
	TensorFactor *f = tensorfactorcreate("phi", vars, 2, sizes);

	/* All values = 1 (uniform) */
	tensorfgaddfactor(fg, f);

	float Z = tensorfgpartition(fg);
	/* Sum of factor over all configs = 4 */
	ASSERT_FLOAT_EQ(Z, 4.0);

	tensorfgfree(fg);
}

TEST(fg_partition_xor_like)
{
	int sizes[2] = {2, 2};
	TensorFactorGraph *fg = tensorfginit(2, sizes);

	int vars[2] = {0, 1};
	TensorFactor *f = tensorfactorcreate("xor", vars, 2, sizes);

	/* XOR factor: 1 when different, 0 when same */
	int idx00[2] = {0, 0};
	int idx01[2] = {0, 1};
	int idx10[2] = {1, 0};
	int idx11[2] = {1, 1};

	tensorfactorset(f, idx00, 0.0);
	tensorfactorset(f, idx01, 1.0);
	tensorfactorset(f, idx10, 1.0);
	tensorfactorset(f, idx11, 0.0);

	tensorfgaddfactor(fg, f);

	float Z = tensorfgpartition(fg);
	/* Only (0,1) and (1,0) contribute */
	ASSERT_FLOAT_EQ(Z, 2.0);

	tensorfgfree(fg);
}

/* ========== Marginal Tests ========== */

TEST(fg_marginal_uniform)
{
	int sizes[2] = {2, 2};
	TensorFactorGraph *fg = tensorfginit(2, sizes);

	Tensor *m = tensorfgmarginal(fg, 0);

	ASSERT_NOT_NULL(m);
	ASSERT_EQ(m->shape[0], 2);

	/* Should be uniform: [0.5, 0.5] */
	float *data = (float*)m->data;
	ASSERT_FLOAT_EQ(data[0], 0.5);
	ASSERT_FLOAT_EQ(data[1], 0.5);

	tensorfree(m);
	tensorfgfree(fg);
}

TEST(fg_marginal_biased)
{
	int sizes[1] = {2};
	TensorFactorGraph *fg = tensorfginit(1, sizes);

	int vars[1] = {0};
	TensorFactor *f = tensorfactorcreate("bias", vars, 1, sizes);

	int idx0[1] = {0};
	int idx1[1] = {1};
	tensorfactorset(f, idx0, 3.0);
	tensorfactorset(f, idx1, 1.0);

	tensorfgaddfactor(fg, f);

	Tensor *m = tensorfgmarginal(fg, 0);

	ASSERT_NOT_NULL(m);

	float *data = (float*)m->data;
	/* P(0) = 3/4 = 0.75, P(1) = 1/4 = 0.25 */
	ASSERT_FLOAT_EQ(data[0], 0.75);
	ASSERT_FLOAT_EQ(data[1], 0.25);

	tensorfree(m);
	tensorfgfree(fg);
}

TEST(fg_marginal_normalized)
{
	int sizes[3] = {2, 2, 2};
	TensorFactorGraph *fg = tensorfginit(3, sizes);

	int vars[2] = {0, 1};
	int fsize[2] = {2, 2};
	TensorFactor *f = tensorfactorcreate("phi", vars, 2, fsize);
	tensorfgaddfactor(fg, f);

	Tensor *m = tensorfgmarginal(fg, 1);

	ASSERT_NOT_NULL(m);

	/* Marginal should sum to 1 */
	float sum = tensorsum(m);
	ASSERT_FLOAT_EQ(sum, 1.0);

	tensorfree(m);
	tensorfgfree(fg);
}

/* ========== Conditional Probability Tests ========== */

TEST(fg_prob_basic)
{
	int sizes[2] = {2, 2};
	TensorFactorGraph *fg = tensorfginit(2, sizes);

	int vars[2] = {0, 1};
	TensorFactor *f = tensorfactorcreate("phi", vars, 2, sizes);
	tensorfgaddfactor(fg, f);

	/* P(X=0 | Y=0) */
	int evidence[2] = {1, 0};  /* var 1 = 0 */
	int query[2] = {0, 0};     /* var 0 = 0 */

	float p = tensorfgprob(fg, evidence, 2, query, 2);

	/* Uniform factor -> P should be 0.5 */
	ASSERT_FLOAT_EQ(p, 0.5);

	tensorfgfree(fg);
}

TEST(fg_prob_deterministic)
{
	int sizes[2] = {2, 2};
	TensorFactorGraph *fg = tensorfginit(2, sizes);

	int vars[2] = {0, 1};
	TensorFactor *f = tensorfactorcreate("det", vars, 2, sizes);

	/* Only (0,0) is valid */
	int idx00[2] = {0, 0};
	int idx01[2] = {0, 1};
	int idx10[2] = {1, 0};
	int idx11[2] = {1, 1};

	tensorfactorset(f, idx00, 1.0);
	tensorfactorset(f, idx01, 0.0);
	tensorfactorset(f, idx10, 0.0);
	tensorfactorset(f, idx11, 0.0);

	tensorfgaddfactor(fg, f);

	/* P(X=0 | Y=0) = 1 */
	int evidence[2] = {1, 0};
	int query[2] = {0, 0};

	float p = tensorfgprob(fg, evidence, 2, query, 2);
	ASSERT_FLOAT_EQ(p, 1.0);

	tensorfgfree(fg);
}

TEST(fg_prob_zero)
{
	int sizes[2] = {2, 2};
	TensorFactorGraph *fg = tensorfginit(2, sizes);

	int vars[2] = {0, 1};
	TensorFactor *f = tensorfactorcreate("det", vars, 2, sizes);

	int idx00[2] = {0, 0};
	int idx01[2] = {0, 1};
	int idx10[2] = {1, 0};
	int idx11[2] = {1, 1};

	tensorfactorset(f, idx00, 1.0);
	tensorfactorset(f, idx01, 0.0);
	tensorfactorset(f, idx10, 0.0);
	tensorfactorset(f, idx11, 0.0);

	tensorfgaddfactor(fg, f);

	/* P(X=1 | Y=0) = 0 */
	int evidence[2] = {1, 0};
	int query[2] = {0, 1};

	float p = tensorfgprob(fg, evidence, 2, query, 2);
	ASSERT_FLOAT_EQ(p, 0.0);

	tensorfgfree(fg);
}

/* ========== MAP Inference Tests ========== */

TEST(fg_map_uniform)
{
	int sizes[2] = {2, 2};
	TensorFactorGraph *fg = tensorfginit(2, sizes);

	Tensor *map = tensorfgmap(fg);

	ASSERT_NOT_NULL(map);
	ASSERT_EQ(map->rank, 1);
	ASSERT_EQ(map->shape[0], 2);

	tensorfree(map);
	tensorfgfree(fg);
}

TEST(fg_map_biased)
{
	int sizes[2] = {2, 2};
	TensorFactorGraph *fg = tensorfginit(2, sizes);

	int vars[2] = {0, 1};
	TensorFactor *f = tensorfactorcreate("bias", vars, 2, sizes);

	/* (1,1) is most likely */
	int idx00[2] = {0, 0};
	int idx01[2] = {0, 1};
	int idx10[2] = {1, 0};
	int idx11[2] = {1, 1};

	tensorfactorset(f, idx00, 1.0);
	tensorfactorset(f, idx01, 2.0);
	tensorfactorset(f, idx10, 3.0);
	tensorfactorset(f, idx11, 10.0);

	tensorfgaddfactor(fg, f);

	Tensor *map = tensorfgmap(fg);

	ASSERT_NOT_NULL(map);

	float *data = (float*)map->data;
	/* MAP should be (1, 1) */
	ASSERT_FLOAT_EQ(data[0], 1.0);
	ASSERT_FLOAT_EQ(data[1], 1.0);

	tensorfree(map);
	tensorfgfree(fg);
}

TEST(fg_map_chain)
{
	int sizes[3] = {2, 2, 2};
	TensorFactorGraph *fg = tensorfginit(3, sizes);

	/* Chain: X0 - X1 - X2 */
	int vars01[2] = {0, 1};
	int vars12[2] = {1, 2};
	int fsize[2] = {2, 2};

	TensorFactor *f1 = tensorfactorcreate("phi01", vars01, 2, fsize);
	TensorFactor *f2 = tensorfactorcreate("phi12", vars12, 2, fsize);

	/* Preference for all same values */
	int idx00[2] = {0, 0};
	int idx11[2] = {1, 1};

	tensorfactorset(f1, idx00, 10.0);
	tensorfactorset(f1, idx11, 10.0);
	tensorfactorset(f2, idx00, 10.0);
	tensorfactorset(f2, idx11, 10.0);

	tensorfgaddfactor(fg, f1);
	tensorfgaddfactor(fg, f2);

	Tensor *map = tensorfgmap(fg);

	ASSERT_NOT_NULL(map);

	/* Should be all 0s or all 1s */
	float *data = (float*)map->data;
	int all_zero = (data[0] == 0.0 && data[1] == 0.0 && data[2] == 0.0);
	int all_one = (data[0] == 1.0 && data[1] == 1.0 && data[2] == 1.0);
	ASSERT(all_zero || all_one);

	tensorfree(map);
	tensorfgfree(fg);
}

/* ========== Belief Propagation Tests ========== */

TEST(fg_bp_basic)
{
	int sizes[2] = {2, 2};
	TensorFactorGraph *fg = tensorfginit(2, sizes);

	int vars[2] = {0, 1};
	TensorFactor *f = tensorfactorcreate("phi", vars, 2, sizes);
	tensorfgaddfactor(fg, f);

	/* Run BP - should not crash */
	tensorfgbp(fg, 10);

	tensorfgfree(fg);
}

TEST(fg_bp_chain)
{
	int sizes[3] = {2, 2, 2};
	TensorFactorGraph *fg = tensorfginit(3, sizes);

	int vars01[2] = {0, 1};
	int vars12[2] = {1, 2};
	int fsize[2] = {2, 2};

	TensorFactor *f1 = tensorfactorcreate("phi01", vars01, 2, fsize);
	TensorFactor *f2 = tensorfactorcreate("phi12", vars12, 2, fsize);

	tensorfgaddfactor(fg, f1);
	tensorfgaddfactor(fg, f2);

	tensorfgbp(fg, 10);

	tensorfgfree(fg);
}

TEST(fg_beliefs)
{
	int sizes[2] = {2, 2};
	TensorFactorGraph *fg = tensorfginit(2, sizes);

	int vars[2] = {0, 1};
	TensorFactor *f = tensorfactorcreate("phi", vars, 2, sizes);
	tensorfgaddfactor(fg, f);

	tensorfgbp(fg, 10);

	Tensor *beliefs = tensorfgbeliefs(fg, 0);

	ASSERT_NOT_NULL(beliefs);
	ASSERT_EQ(beliefs->shape[0], 2);

	/* Beliefs should sum to 1 */
	float sum = tensorsum(beliefs);
	ASSERT_FLOAT_EQ(sum, 1.0);

	tensorfree(beliefs);
	tensorfgfree(fg);
}

/* ========== Ising Model Tests ========== */

TEST(fg_ising_ferromagnetic)
{
	/* 2x2 Ising model grid */
	int sizes[4] = {2, 2, 2, 2};
	TensorFactorGraph *fg = tensorfginit(4, sizes);

	/* Edges: 0-1, 0-2, 1-3, 2-3 */
	int vars01[2] = {0, 1};
	int vars02[2] = {0, 2};
	int vars13[2] = {1, 3};
	int vars23[2] = {2, 3};
	int fsize[2] = {2, 2};

	TensorFactor *f01 = tensorfactorcreate("phi01", vars01, 2, fsize);
	TensorFactor *f02 = tensorfactorcreate("phi02", vars02, 2, fsize);
	TensorFactor *f13 = tensorfactorcreate("phi13", vars13, 2, fsize);
	TensorFactor *f23 = tensorfactorcreate("phi23", vars23, 2, fsize);

	/* Ferromagnetic: prefer same values */
	int idx00[2] = {0, 0};
	int idx01[2] = {0, 1};
	int idx10[2] = {1, 0};
	int idx11[2] = {1, 1};

	float J = 2.0;  /* Coupling strength */
	tensorfactorset(f01, idx00, J);
	tensorfactorset(f01, idx11, J);
	tensorfactorset(f01, idx01, 1.0);
	tensorfactorset(f01, idx10, 1.0);

	tensorfactorset(f02, idx00, J);
	tensorfactorset(f02, idx11, J);
	tensorfactorset(f02, idx01, 1.0);
	tensorfactorset(f02, idx10, 1.0);

	tensorfactorset(f13, idx00, J);
	tensorfactorset(f13, idx11, J);
	tensorfactorset(f13, idx01, 1.0);
	tensorfactorset(f13, idx10, 1.0);

	tensorfactorset(f23, idx00, J);
	tensorfactorset(f23, idx11, J);
	tensorfactorset(f23, idx01, 1.0);
	tensorfactorset(f23, idx10, 1.0);

	tensorfgaddfactor(fg, f01);
	tensorfgaddfactor(fg, f02);
	tensorfgaddfactor(fg, f13);
	tensorfgaddfactor(fg, f23);

	/* MAP should be all same values */
	Tensor *map = tensorfgmap(fg);

	ASSERT_NOT_NULL(map);

	float *data = (float*)map->data;
	int all_same = (data[0] == data[1] && data[1] == data[2] && data[2] == data[3]);
	ASSERT(all_same);

	tensorfree(map);
	tensorfgfree(fg);
}

/* ========== Bayesian Network Tests ========== */

TEST(fg_bayes_simple)
{
	/*
	 * Simple Bayesian network:
	 * Rain -> WetGrass
	 * P(Rain) = 0.3
	 * P(WetGrass | Rain) = 0.9
	 * P(WetGrass | ~Rain) = 0.2
	 */
	int sizes[2] = {2, 2};
	TensorFactorGraph *fg = tensorfginit(2, sizes);

	/* Prior on Rain (var 0) */
	int vars_rain[1] = {0};
	int size_rain[1] = {2};
	TensorFactor *prior = tensorfactorcreate("rain", vars_rain, 1, size_rain);

	int idx0[1] = {0};
	int idx1[1] = {1};
	tensorfactorset(prior, idx0, 0.7);  /* P(~Rain) */
	tensorfactorset(prior, idx1, 0.3);  /* P(Rain) */

	/* Conditional P(WetGrass | Rain) */
	int vars_cond[2] = {0, 1};
	TensorFactor *cond = tensorfactorcreate("wet|rain", vars_cond, 2, sizes);

	int idx00[2] = {0, 0};
	int idx01[2] = {0, 1};
	int idx10[2] = {1, 0};
	int idx11[2] = {1, 1};

	tensorfactorset(cond, idx00, 0.8);  /* P(~Wet | ~Rain) */
	tensorfactorset(cond, idx01, 0.2);  /* P(Wet | ~Rain) */
	tensorfactorset(cond, idx10, 0.1);  /* P(~Wet | Rain) */
	tensorfactorset(cond, idx11, 0.9);  /* P(Wet | Rain) */

	tensorfgaddfactor(fg, prior);
	tensorfgaddfactor(fg, cond);

	/* Compute marginal for WetGrass */
	Tensor *m = tensorfgmarginal(fg, 1);

	ASSERT_NOT_NULL(m);

	float *data = (float*)m->data;
	/* P(Wet) = P(Wet|Rain)*P(Rain) + P(Wet|~Rain)*P(~Rain) */
	/* = 0.9 * 0.3 + 0.2 * 0.7 = 0.27 + 0.14 = 0.41 */
	ASSERT_GT(data[1], 0.3);
	ASSERT_LT(data[1], 0.5);

	tensorfree(m);
	tensorfgfree(fg);
}

/* ========== Edge Cases ========== */

TEST(fg_empty)
{
	int sizes[2] = {2, 2};
	TensorFactorGraph *fg = tensorfginit(2, sizes);

	/* Operations on empty graph */
	float Z = tensorfgpartition(fg);
	ASSERT_GT(Z, 0.0);

	Tensor *m = tensorfgmarginal(fg, 0);
	ASSERT_NOT_NULL(m);
	tensorfree(m);

	Tensor *map = tensorfgmap(fg);
	ASSERT_NOT_NULL(map);
	tensorfree(map);

	tensorfgfree(fg);
}

TEST(fg_single_var)
{
	int sizes[1] = {3};
	TensorFactorGraph *fg = tensorfginit(1, sizes);

	int vars[1] = {0};
	TensorFactor *f = tensorfactorcreate("phi", vars, 1, sizes);

	int idx0[1] = {0};
	int idx1[1] = {1};
	int idx2[1] = {2};
	tensorfactorset(f, idx0, 1.0);
	tensorfactorset(f, idx1, 2.0);
	tensorfactorset(f, idx2, 3.0);

	tensorfgaddfactor(fg, f);

	Tensor *m = tensorfgmarginal(fg, 0);

	ASSERT_NOT_NULL(m);
	float *data = (float*)m->data;
	/* P(0) = 1/6, P(1) = 2/6, P(2) = 3/6 */
	ASSERT_GT(data[2], data[1]);
	ASSERT_GT(data[1], data[0]);

	tensorfree(m);
	tensorfgfree(fg);
}

TEST(fg_large_domain)
{
	int sizes[2] = {10, 10};
	TensorFactorGraph *fg = tensorfginit(2, sizes);

	int vars[2] = {0, 1};
	TensorFactor *f = tensorfactorcreate("phi", vars, 2, sizes);

	tensorfgaddfactor(fg, f);

	float Z = tensorfgpartition(fg);
	ASSERT_FLOAT_EQ(Z, 100.0);  /* 10 * 10 uniform */

	tensorfgfree(fg);
}

/* ========== Main Test Runner ========== */

void
main(int argc, char *argv[])
{
	print("=== Graphical Models Unit Tests ===\n\n");

	print("Factor Creation Tests:\n");
	RUN_TEST(factor_create_binary);
	RUN_TEST(factor_create_unary);
	RUN_TEST(factor_create_ternary);
	RUN_TEST(factor_free_null);

	print("\nFactor Value Tests:\n");
	RUN_TEST(factor_set_get);
	RUN_TEST(factor_set_all);
	RUN_TEST(factor_default_uniform);

	print("\nFactor Graph Tests:\n");
	RUN_TEST(fg_init);
	RUN_TEST(fg_init_different_sizes);
	RUN_TEST(fg_free_null);
	RUN_TEST(fg_add_factor);
	RUN_TEST(fg_add_multiple_factors);

	print("\nPartition Function Tests:\n");
	RUN_TEST(fg_partition_uniform);
	RUN_TEST(fg_partition_single_factor);
	RUN_TEST(fg_partition_xor_like);

	print("\nMarginal Tests:\n");
	RUN_TEST(fg_marginal_uniform);
	RUN_TEST(fg_marginal_biased);
	RUN_TEST(fg_marginal_normalized);

	print("\nConditional Probability Tests:\n");
	RUN_TEST(fg_prob_basic);
	RUN_TEST(fg_prob_deterministic);
	RUN_TEST(fg_prob_zero);

	print("\nMAP Inference Tests:\n");
	RUN_TEST(fg_map_uniform);
	RUN_TEST(fg_map_biased);
	RUN_TEST(fg_map_chain);

	print("\nBelief Propagation Tests:\n");
	RUN_TEST(fg_bp_basic);
	RUN_TEST(fg_bp_chain);
	RUN_TEST(fg_beliefs);

	print("\nIsing Model Tests:\n");
	RUN_TEST(fg_ising_ferromagnetic);

	print("\nBayesian Network Tests:\n");
	RUN_TEST(fg_bayes_simple);

	print("\nEdge Case Tests:\n");
	RUN_TEST(fg_empty);
	RUN_TEST(fg_single_var);
	RUN_TEST(fg_large_domain);

	print("\n=== Test Summary ===\n");
	print("Tests run: %d\n", tests_run);
	print("Tests passed: %d\n", tests_passed);
	print("Tests failed: %d\n", tests_failed);

	exits(tests_failed > 0 ? "FAIL" : nil);
}
