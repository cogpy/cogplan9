/*
 * Exhaustive Unit Tests for Tensor Core Operations
 * Tests tensor creation, arithmetic, and reductions
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

/* Float comparison with tolerance */
#define FLOAT_EQ(a, b) (((a) - (b)) < 0.001 && ((b) - (a)) < 0.001)
#define ASSERT_FLOAT_EQ(a, b) ASSERT(FLOAT_EQ(a, b))

/* ========== Tensor Creation Tests ========== */

TEST(tensor_create_1d)
{
	int shape[1] = {10};
	Tensor *t = tensorcreate(TensorFloat, 1, shape);

	ASSERT_NOT_NULL(t);
	ASSERT_EQ(t->rank, 1);
	ASSERT_EQ(t->shape[0], 10);
	ASSERT_EQ(t->nelems, 10);
	ASSERT_EQ(t->type, TensorFloat);

	tensorfree(t);
}

TEST(tensor_create_2d)
{
	int shape[2] = {3, 4};
	Tensor *t = tensorcreate(TensorFloat, 2, shape);

	ASSERT_NOT_NULL(t);
	ASSERT_EQ(t->rank, 2);
	ASSERT_EQ(t->shape[0], 3);
	ASSERT_EQ(t->shape[1], 4);
	ASSERT_EQ(t->nelems, 12);

	tensorfree(t);
}

TEST(tensor_create_3d)
{
	int shape[3] = {2, 3, 4};
	Tensor *t = tensorcreate(TensorFloat, 3, shape);

	ASSERT_NOT_NULL(t);
	ASSERT_EQ(t->rank, 3);
	ASSERT_EQ(t->shape[0], 2);
	ASSERT_EQ(t->shape[1], 3);
	ASSERT_EQ(t->shape[2], 4);
	ASSERT_EQ(t->nelems, 24);

	tensorfree(t);
}

TEST(tensor_create_high_rank)
{
	int shape[6] = {2, 2, 2, 2, 2, 2};
	Tensor *t = tensorcreate(TensorFloat, 6, shape);

	ASSERT_NOT_NULL(t);
	ASSERT_EQ(t->rank, 6);
	ASSERT_EQ(t->nelems, 64);

	tensorfree(t);
}

TEST(tensor_create_types)
{
	int shape[1] = {5};

	Tensor *tf = tensorcreate(TensorFloat, 1, shape);
	ASSERT_NOT_NULL(tf);
	ASSERT_EQ(tf->type, TensorFloat);
	tensorfree(tf);

	Tensor *td = tensorcreate(TensorDouble, 1, shape);
	ASSERT_NOT_NULL(td);
	ASSERT_EQ(td->type, TensorDouble);
	tensorfree(td);

	Tensor *ti = tensorcreate(TensorInt, 1, shape);
	ASSERT_NOT_NULL(ti);
	ASSERT_EQ(ti->type, TensorInt);
	tensorfree(ti);

	Tensor *tb = tensorcreate(TensorBool, 1, shape);
	ASSERT_NOT_NULL(tb);
	ASSERT_EQ(tb->type, TensorBool);
	tensorfree(tb);
}

TEST(tensor_zeros)
{
	int shape[2] = {3, 3};
	Tensor *t = tensorzeros(2, shape);

	ASSERT_NOT_NULL(t);
	float sum = tensorsum(t);
	ASSERT_FLOAT_EQ(sum, 0.0);

	tensorfree(t);
}

TEST(tensor_ones)
{
	int shape[2] = {3, 3};
	Tensor *t = tensorones(2, shape);

	ASSERT_NOT_NULL(t);
	float sum = tensorsum(t);
	ASSERT_FLOAT_EQ(sum, 9.0);

	tensorfree(t);
}

TEST(tensor_eye)
{
	Tensor *t = tensoreye(4);

	ASSERT_NOT_NULL(t);
	ASSERT_EQ(t->rank, 2);
	ASSERT_EQ(t->shape[0], 4);
	ASSERT_EQ(t->shape[1], 4);

	float sum = tensorsum(t);
	ASSERT_FLOAT_EQ(sum, 4.0);

	tensorfree(t);
}

TEST(tensor_rand)
{
	int shape[2] = {10, 10};
	Tensor *t = tensorrand(2, shape);

	ASSERT_NOT_NULL(t);
	ASSERT_EQ(t->nelems, 100);

	/* Random values should be between 0 and 1 */
	float min = tensormin(t);
	float max = tensormax(t);
	ASSERT_GE(min, 0.0);
	ASSERT_LE(max, 1.0);

	tensorfree(t);
}

TEST(tensor_free_null)
{
	/* Should not crash */
	tensorfree(nil);
}

TEST(tensor_copy)
{
	int shape[2] = {3, 3};
	Tensor *t = tensorones(2, shape);
	Tensor *c = tensorcopy(t);

	ASSERT_NOT_NULL(c);
	ASSERT_NE(t, c);
	ASSERT_EQ(t->rank, c->rank);
	ASSERT_EQ(t->nelems, c->nelems);

	float sum1 = tensorsum(t);
	float sum2 = tensorsum(c);
	ASSERT_FLOAT_EQ(sum1, sum2);

	tensorfree(t);
	tensorfree(c);
}

TEST(tensor_reshape)
{
	int shape1[2] = {3, 4};
	Tensor *t = tensorones(2, shape1);

	int shape2[3] = {2, 2, 3};
	Tensor *r = tensorreshape(t, 3, shape2);

	ASSERT_NOT_NULL(r);
	ASSERT_EQ(r->rank, 3);
	ASSERT_EQ(r->nelems, 12);

	tensorfree(t);
	tensorfree(r);
}

/* ========== Element Access Tests ========== */

TEST(tensor_set_get_float)
{
	int shape[2] = {3, 3};
	Tensor *t = tensorzeros(2, shape);

	tensorsetf(t, 5.5, 1, 2);
	float val = tensorgetf(t, 1, 2);

	ASSERT_FLOAT_EQ(val, 5.5);

	tensorfree(t);
}

TEST(tensor_set_get_int)
{
	int shape[2] = {3, 3};
	Tensor *t = tensorcreate(TensorInt, 2, shape);

	tensorseti(t, 42, 2, 1);
	int val = tensorgeti(t, 2, 1);

	ASSERT_EQ(val, 42);

	tensorfree(t);
}

TEST(tensor_fill)
{
	int shape[2] = {4, 4};
	Tensor *t = tensorcreate(TensorFloat, 2, shape);

	tensorfill(t, 3.14);

	float val = tensorgetf(t, 2, 2);
	ASSERT_FLOAT_EQ(val, 3.14);

	float sum = tensorsum(t);
	ASSERT_FLOAT_EQ(sum, 3.14 * 16);

	tensorfree(t);
}

/* ========== Arithmetic Tests ========== */

TEST(tensor_add)
{
	int shape[2] = {2, 2};
	Tensor *a = tensorones(2, shape);
	Tensor *b = tensorones(2, shape);

	Tensor *c = tensoradd(a, b);

	ASSERT_NOT_NULL(c);
	float sum = tensorsum(c);
	ASSERT_FLOAT_EQ(sum, 8.0);

	tensorfree(a);
	tensorfree(b);
	tensorfree(c);
}

TEST(tensor_sub)
{
	int shape[2] = {2, 2};
	Tensor *a = tensorones(2, shape);
	tensorfill(a, 5.0);
	Tensor *b = tensorones(2, shape);
	tensorfill(b, 2.0);

	Tensor *c = tensorsub(a, b);

	ASSERT_NOT_NULL(c);
	float sum = tensorsum(c);
	ASSERT_FLOAT_EQ(sum, 12.0);

	tensorfree(a);
	tensorfree(b);
	tensorfree(c);
}

TEST(tensor_mul_elementwise)
{
	int shape[2] = {2, 2};
	Tensor *a = tensorones(2, shape);
	tensorfill(a, 3.0);
	Tensor *b = tensorones(2, shape);
	tensorfill(b, 2.0);

	Tensor *c = tensormul(a, b);

	ASSERT_NOT_NULL(c);
	float sum = tensorsum(c);
	ASSERT_FLOAT_EQ(sum, 24.0);

	tensorfree(a);
	tensorfree(b);
	tensorfree(c);
}

TEST(tensor_div)
{
	int shape[2] = {2, 2};
	Tensor *a = tensorones(2, shape);
	tensorfill(a, 6.0);
	Tensor *b = tensorones(2, shape);
	tensorfill(b, 2.0);

	Tensor *c = tensordiv(a, b);

	ASSERT_NOT_NULL(c);
	float sum = tensorsum(c);
	ASSERT_FLOAT_EQ(sum, 12.0);

	tensorfree(a);
	tensorfree(b);
	tensorfree(c);
}

TEST(tensor_scale)
{
	int shape[2] = {2, 2};
	Tensor *a = tensorones(2, shape);

	Tensor *c = tensorscale(a, 3.5);

	ASSERT_NOT_NULL(c);
	float sum = tensorsum(c);
	ASSERT_FLOAT_EQ(sum, 14.0);

	tensorfree(a);
	tensorfree(c);
}

TEST(tensor_neg)
{
	int shape[2] = {2, 2};
	Tensor *a = tensorones(2, shape);

	Tensor *c = tensorneg(a);

	ASSERT_NOT_NULL(c);
	float sum = tensorsum(c);
	ASSERT_FLOAT_EQ(sum, -4.0);

	tensorfree(a);
	tensorfree(c);
}

/* ========== Matrix Operations Tests ========== */

TEST(tensor_matmul_square)
{
	int shape[2] = {2, 2};
	Tensor *a = tensorones(2, shape);
	Tensor *b = tensorones(2, shape);

	Tensor *c = tensormatmul(a, b);

	ASSERT_NOT_NULL(c);
	ASSERT_EQ(c->rank, 2);
	ASSERT_EQ(c->shape[0], 2);
	ASSERT_EQ(c->shape[1], 2);

	/* Each element should be 2 (sum of 1*1 + 1*1) */
	float sum = tensorsum(c);
	ASSERT_FLOAT_EQ(sum, 8.0);

	tensorfree(a);
	tensorfree(b);
	tensorfree(c);
}

TEST(tensor_matmul_rectangular)
{
	int shape_a[2] = {2, 3};
	int shape_b[2] = {3, 4};
	Tensor *a = tensorones(2, shape_a);
	Tensor *b = tensorones(2, shape_b);

	Tensor *c = tensormatmul(a, b);

	ASSERT_NOT_NULL(c);
	ASSERT_EQ(c->shape[0], 2);
	ASSERT_EQ(c->shape[1], 4);

	/* Each element should be 3 */
	float sum = tensorsum(c);
	ASSERT_FLOAT_EQ(sum, 24.0);

	tensorfree(a);
	tensorfree(b);
	tensorfree(c);
}

TEST(tensor_transpose)
{
	int shape[2] = {2, 3};
	Tensor *t = tensorcreate(TensorFloat, 2, shape);
	tensorfill(t, 1.0);

	Tensor *tr = tensortranspose(t);

	ASSERT_NOT_NULL(tr);
	ASSERT_EQ(tr->shape[0], 3);
	ASSERT_EQ(tr->shape[1], 2);

	tensorfree(t);
	tensorfree(tr);
}

TEST(tensor_eye_matmul)
{
	Tensor *I = tensoreye(3);
	int shape[2] = {3, 3};
	Tensor *a = tensorrand(2, shape);

	Tensor *c = tensormatmul(I, a);

	ASSERT_NOT_NULL(c);

	/* I * A should equal A */
	float sum_a = tensorsum(a);
	float sum_c = tensorsum(c);
	ASSERT_FLOAT_EQ(sum_a, sum_c);

	tensorfree(I);
	tensorfree(a);
	tensorfree(c);
}

TEST(tensor_outer)
{
	int shape[1] = {3};
	Tensor *a = tensorones(1, shape);
	Tensor *b = tensorones(1, shape);

	Tensor *c = tensorouter(a, b);

	ASSERT_NOT_NULL(c);
	ASSERT_EQ(c->rank, 2);
	ASSERT_EQ(c->shape[0], 3);
	ASSERT_EQ(c->shape[1], 3);

	tensorfree(a);
	tensorfree(b);
	tensorfree(c);
}

TEST(tensor_kron)
{
	Tensor *I2 = tensoreye(2);
	Tensor *I3 = tensoreye(3);

	Tensor *K = tensorkron(I2, I3);

	ASSERT_NOT_NULL(K);
	ASSERT_EQ(K->shape[0], 6);
	ASSERT_EQ(K->shape[1], 6);

	tensorfree(I2);
	tensorfree(I3);
	tensorfree(K);
}

/* ========== Reduction Tests ========== */

TEST(tensor_sum)
{
	int shape[2] = {3, 3};
	Tensor *t = tensorones(2, shape);

	float sum = tensorsum(t);
	ASSERT_FLOAT_EQ(sum, 9.0);

	tensorfree(t);
}

TEST(tensor_sum_axis)
{
	int shape[2] = {2, 3};
	Tensor *t = tensorones(2, shape);

	Tensor *s0 = tensorsumaxis(t, 0);
	ASSERT_NOT_NULL(s0);
	ASSERT_EQ(s0->rank, 1);
	ASSERT_EQ(s0->shape[0], 3);

	Tensor *s1 = tensorsumaxis(t, 1);
	ASSERT_NOT_NULL(s1);
	ASSERT_EQ(s1->rank, 1);
	ASSERT_EQ(s1->shape[0], 2);

	tensorfree(t);
	tensorfree(s0);
	tensorfree(s1);
}

TEST(tensor_mean)
{
	int shape[2] = {2, 2};
	Tensor *t = tensorones(2, shape);
	tensorfill(t, 4.0);

	float mean = tensormean(t);
	ASSERT_FLOAT_EQ(mean, 4.0);

	tensorfree(t);
}

TEST(tensor_max_min)
{
	int shape[1] = {5};
	Tensor *t = tensorcreate(TensorFloat, 1, shape);

	float *data = (float*)t->data;
	data[0] = 1.0;
	data[1] = 5.0;
	data[2] = 3.0;
	data[3] = -2.0;
	data[4] = 4.0;

	float max = tensormax(t);
	float min = tensormin(t);

	ASSERT_FLOAT_EQ(max, 5.0);
	ASSERT_FLOAT_EQ(min, -2.0);

	tensorfree(t);
}

TEST(tensor_argmax)
{
	int shape[2] = {2, 3};
	Tensor *t = tensorcreate(TensorFloat, 2, shape);

	float *data = (float*)t->data;
	data[0] = 1.0; data[1] = 2.0; data[2] = 3.0;
	data[3] = 6.0; data[4] = 5.0; data[5] = 4.0;

	Tensor *am = tensorargmax(t, 1);

	ASSERT_NOT_NULL(am);
	ASSERT_EQ(am->rank, 1);
	ASSERT_EQ(am->shape[0], 2);

	tensorfree(t);
	tensorfree(am);
}

/* ========== Norm Tests ========== */

TEST(tensor_norm_l2)
{
	int shape[1] = {3};
	Tensor *t = tensorcreate(TensorFloat, 1, shape);

	float *data = (float*)t->data;
	data[0] = 3.0;
	data[1] = 4.0;
	data[2] = 0.0;

	float norm = tensornorm(t);
	ASSERT_FLOAT_EQ(norm, 5.0);

	tensorfree(t);
}

TEST(tensor_norm_lp)
{
	int shape[1] = {2};
	Tensor *t = tensorcreate(TensorFloat, 1, shape);

	float *data = (float*)t->data;
	data[0] = 3.0;
	data[1] = 4.0;

	float norm1 = tensornormp(t, 1.0);
	ASSERT_FLOAT_EQ(norm1, 7.0);

	float norm2 = tensornormp(t, 2.0);
	ASSERT_FLOAT_EQ(norm2, 5.0);

	tensorfree(t);
}

TEST(tensor_frobenius)
{
	int shape[2] = {2, 2};
	Tensor *t = tensorones(2, shape);

	float frob = tensorfrobenius(t);
	ASSERT_FLOAT_EQ(frob, 2.0);

	tensorfree(t);
}

/* ========== Sparse Tensor Tests ========== */

TEST(tensor_sparse_create)
{
	int shape[2] = {100, 100};
	Tensor *t = tensorsparse(TensorFloat, 2, shape, 10);

	ASSERT_NOT_NULL(t);
	ASSERT_EQ(t->format, TensorSparse);
	ASSERT_EQ(t->nnz, 10);

	tensorfree(t);
}

/* ========== Trace and Diagonal Tests ========== */

TEST(tensor_trace)
{
	Tensor *I = tensoreye(5);

	Tensor *tr = tensortrace(I);

	ASSERT_NOT_NULL(tr);
	float sum = tensorsum(tr);
	ASSERT_FLOAT_EQ(sum, 5.0);

	tensorfree(I);
	tensorfree(tr);
}

TEST(tensor_diag)
{
	Tensor *I = tensoreye(4);

	Tensor *d = tensordiag(I);

	ASSERT_NOT_NULL(d);
	ASSERT_EQ(d->rank, 1);
	ASSERT_EQ(d->shape[0], 4);

	float sum = tensorsum(d);
	ASSERT_FLOAT_EQ(sum, 4.0);

	tensorfree(I);
	tensorfree(d);
}

/* ========== Edge Cases ========== */

TEST(tensor_empty_shape)
{
	int shape[1] = {0};
	Tensor *t = tensorcreate(TensorFloat, 1, shape);

	/* Should handle gracefully */
	ASSERT_EQ(t->nelems, 0);

	tensorfree(t);
}

TEST(tensor_single_element)
{
	int shape[1] = {1};
	Tensor *t = tensorcreate(TensorFloat, 1, shape);

	ASSERT_NOT_NULL(t);
	ASSERT_EQ(t->nelems, 1);

	tensorsetf(t, 42.0, 0);
	float val = tensorgetf(t, 0);
	ASSERT_FLOAT_EQ(val, 42.0);

	tensorfree(t);
}

TEST(tensor_large)
{
	int shape[2] = {1000, 1000};
	Tensor *t = tensorzeros(2, shape);

	ASSERT_NOT_NULL(t);
	ASSERT_EQ(t->nelems, 1000000);

	tensorfree(t);
}

/* ========== Main Test Runner ========== */

void
main(int argc, char *argv[])
{
	print("=== Tensor Core Unit Tests ===\n\n");

	print("Tensor Creation Tests:\n");
	RUN_TEST(tensor_create_1d);
	RUN_TEST(tensor_create_2d);
	RUN_TEST(tensor_create_3d);
	RUN_TEST(tensor_create_high_rank);
	RUN_TEST(tensor_create_types);
	RUN_TEST(tensor_zeros);
	RUN_TEST(tensor_ones);
	RUN_TEST(tensor_eye);
	RUN_TEST(tensor_rand);
	RUN_TEST(tensor_free_null);
	RUN_TEST(tensor_copy);
	RUN_TEST(tensor_reshape);

	print("\nElement Access Tests:\n");
	RUN_TEST(tensor_set_get_float);
	RUN_TEST(tensor_set_get_int);
	RUN_TEST(tensor_fill);

	print("\nArithmetic Tests:\n");
	RUN_TEST(tensor_add);
	RUN_TEST(tensor_sub);
	RUN_TEST(tensor_mul_elementwise);
	RUN_TEST(tensor_div);
	RUN_TEST(tensor_scale);
	RUN_TEST(tensor_neg);

	print("\nMatrix Operations Tests:\n");
	RUN_TEST(tensor_matmul_square);
	RUN_TEST(tensor_matmul_rectangular);
	RUN_TEST(tensor_transpose);
	RUN_TEST(tensor_eye_matmul);
	RUN_TEST(tensor_outer);
	RUN_TEST(tensor_kron);

	print("\nReduction Tests:\n");
	RUN_TEST(tensor_sum);
	RUN_TEST(tensor_sum_axis);
	RUN_TEST(tensor_mean);
	RUN_TEST(tensor_max_min);
	RUN_TEST(tensor_argmax);

	print("\nNorm Tests:\n");
	RUN_TEST(tensor_norm_l2);
	RUN_TEST(tensor_norm_lp);
	RUN_TEST(tensor_frobenius);

	print("\nSparse Tensor Tests:\n");
	RUN_TEST(tensor_sparse_create);

	print("\nTrace and Diagonal Tests:\n");
	RUN_TEST(tensor_trace);
	RUN_TEST(tensor_diag);

	print("\nEdge Case Tests:\n");
	RUN_TEST(tensor_empty_shape);
	RUN_TEST(tensor_single_element);
	RUN_TEST(tensor_large);

	print("\n=== Test Summary ===\n");
	print("Tests run: %d\n", tests_run);
	print("Tests passed: %d\n", tests_passed);
	print("Tests failed: %d\n", tests_failed);

	exits(tests_failed > 0 ? "FAIL" : nil);
}
