/*
 * Exhaustive Unit Tests for Einstein Summation
 * Tests einsum operations and tensor equations
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

#define FLOAT_EQ(a, b) (((a) - (b)) < 0.01 && ((b) - (a)) < 0.01)
#define ASSERT_FLOAT_EQ(a, b) ASSERT(FLOAT_EQ(a, b))

/* ========== Basic Einsum Tests ========== */

TEST(einsum_matmul)
{
	/* Matrix multiplication: C[i,k] = A[i,j] * B[j,k] */
	int shape_a[2] = {2, 3};
	int shape_b[2] = {3, 4};
	Tensor *a = tensorones(2, shape_a);
	Tensor *b = tensorones(2, shape_b);

	Tensor *c = tensoreinsum("ij,jk->ik", a, b);

	ASSERT_NOT_NULL(c);
	ASSERT_EQ(c->rank, 2);
	ASSERT_EQ(c->shape[0], 2);
	ASSERT_EQ(c->shape[1], 4);

	/* Each element should be 3 (sum over j dimension of size 3) */
	float sum = tensorsum(c);
	ASSERT_FLOAT_EQ(sum, 24.0);

	tensorfree(a);
	tensorfree(b);
	tensorfree(c);
}

TEST(einsum_inner_product)
{
	/* Inner product: c = sum(A[i] * B[i]) */
	int shape[1] = {5};
	Tensor *a = tensorones(1, shape);
	Tensor *b = tensorones(1, shape);

	Tensor *c = tensoreinsum("i,i->", a, b);

	ASSERT_NOT_NULL(c);
	ASSERT_EQ(c->rank, 0);
	float sum = tensorsum(c);
	ASSERT_FLOAT_EQ(sum, 5.0);

	tensorfree(a);
	tensorfree(b);
	tensorfree(c);
}

TEST(einsum_outer_product)
{
	/* Outer product: C[i,j] = A[i] * B[j] */
	int shape[1] = {3};
	Tensor *a = tensorones(1, shape);
	Tensor *b = tensorones(1, shape);

	Tensor *c = tensoreinsum("i,j->ij", a, b);

	ASSERT_NOT_NULL(c);
	ASSERT_EQ(c->rank, 2);
	ASSERT_EQ(c->shape[0], 3);
	ASSERT_EQ(c->shape[1], 3);

	tensorfree(a);
	tensorfree(b);
	tensorfree(c);
}

TEST(einsum_transpose)
{
	/* Transpose: B[j,i] = A[i,j] */
	int shape[2] = {2, 3};
	Tensor *a = tensorones(2, shape);

	Tensor *b = tensoreinsum("ij->ji", a, nil);

	ASSERT_NOT_NULL(b);
	ASSERT_EQ(b->shape[0], 3);
	ASSERT_EQ(b->shape[1], 2);

	tensorfree(a);
	tensorfree(b);
}

TEST(einsum_trace)
{
	/* Trace: c = sum(A[i,i]) */
	Tensor *I = tensoreye(4);

	Tensor *tr = tensoreinsum("ii->", I, nil);

	ASSERT_NOT_NULL(tr);
	float sum = tensorsum(tr);
	ASSERT_FLOAT_EQ(sum, 4.0);

	tensorfree(I);
	tensorfree(tr);
}

TEST(einsum_diagonal)
{
	/* Extract diagonal: b[i] = A[i,i] */
	Tensor *I = tensoreye(5);

	Tensor *d = tensoreinsum("ii->i", I, nil);

	ASSERT_NOT_NULL(d);
	ASSERT_EQ(d->rank, 1);
	ASSERT_EQ(d->shape[0], 5);

	tensorfree(I);
	tensorfree(d);
}

TEST(einsum_batch_matmul)
{
	/* Batch matrix multiplication: C[b,i,k] = A[b,i,j] * B[b,j,k] */
	int shape_a[3] = {2, 3, 4};
	int shape_b[3] = {2, 4, 5};
	Tensor *a = tensorones(3, shape_a);
	Tensor *b = tensorones(3, shape_b);

	Tensor *c = tensoreinsum("bij,bjk->bik", a, b);

	ASSERT_NOT_NULL(c);
	ASSERT_EQ(c->rank, 3);
	ASSERT_EQ(c->shape[0], 2);
	ASSERT_EQ(c->shape[1], 3);
	ASSERT_EQ(c->shape[2], 5);

	tensorfree(a);
	tensorfree(b);
	tensorfree(c);
}

TEST(einsum_hadamard)
{
	/* Hadamard (element-wise) product: C[i,j] = A[i,j] * B[i,j] */
	int shape[2] = {3, 3};
	Tensor *a = tensorones(2, shape);
	tensorfill(a, 2.0);
	Tensor *b = tensorones(2, shape);
	tensorfill(b, 3.0);

	Tensor *c = tensoreinsum("ij,ij->ij", a, b);

	ASSERT_NOT_NULL(c);
	float sum = tensorsum(c);
	ASSERT_FLOAT_EQ(sum, 54.0);

	tensorfree(a);
	tensorfree(b);
	tensorfree(c);
}

TEST(einsum_sum_all)
{
	/* Sum all elements: c = sum(A[i,j]) */
	int shape[2] = {3, 4};
	Tensor *a = tensorones(2, shape);

	Tensor *c = tensoreinsum("ij->", a, nil);

	ASSERT_NOT_NULL(c);
	float sum = tensorsum(c);
	ASSERT_FLOAT_EQ(sum, 12.0);

	tensorfree(a);
	tensorfree(c);
}

TEST(einsum_sum_row)
{
	/* Sum over rows: b[j] = sum_i(A[i,j]) */
	int shape[2] = {2, 3};
	Tensor *a = tensorones(2, shape);

	Tensor *b = tensoreinsum("ij->j", a, nil);

	ASSERT_NOT_NULL(b);
	ASSERT_EQ(b->rank, 1);
	ASSERT_EQ(b->shape[0], 3);

	float sum = tensorsum(b);
	ASSERT_FLOAT_EQ(sum, 6.0);

	tensorfree(a);
	tensorfree(b);
}

TEST(einsum_sum_col)
{
	/* Sum over columns: b[i] = sum_j(A[i,j]) */
	int shape[2] = {2, 3};
	Tensor *a = tensorones(2, shape);

	Tensor *b = tensoreinsum("ij->i", a, nil);

	ASSERT_NOT_NULL(b);
	ASSERT_EQ(b->rank, 1);
	ASSERT_EQ(b->shape[0], 2);

	float sum = tensorsum(b);
	ASSERT_FLOAT_EQ(sum, 6.0);

	tensorfree(a);
	tensorfree(b);
}

/* ========== Multi-Tensor Einsum Tests ========== */

TEST(einsum3_chain)
{
	/* Triple matrix chain: D[i,l] = A[i,j] * B[j,k] * C[k,l] */
	int shape[2] = {2, 2};
	Tensor *a = tensorones(2, shape);
	Tensor *b = tensorones(2, shape);
	Tensor *c = tensorones(2, shape);

	Tensor *d = tensoreinsum3("ij,jk,kl->il", a, b, c);

	ASSERT_NOT_NULL(d);
	ASSERT_EQ(d->rank, 2);

	tensorfree(a);
	tensorfree(b);
	tensorfree(c);
	tensorfree(d);
}

TEST(einsumv_multiple)
{
	int shape[2] = {2, 2};
	Tensor *tensors[4];
	int i;

	for(i = 0; i < 4; i++)
		tensors[i] = tensorones(2, shape);

	Tensor *r = tensoreinsumv("ij,jk,kl,lm->im", tensors, 4);

	ASSERT_NOT_NULL(r);
	ASSERT_EQ(r->rank, 2);

	for(i = 0; i < 4; i++)
		tensorfree(tensors[i]);
	tensorfree(r);
}

/* ========== Tensor Contraction Tests ========== */

TEST(contract_single_axis)
{
	int shape_a[2] = {3, 4};
	int shape_b[2] = {4, 5};
	Tensor *a = tensorones(2, shape_a);
	Tensor *b = tensorones(2, shape_b);

	int axes_a[1] = {1};
	int axes_b[1] = {0};

	Tensor *c = tensorcontract(a, b, axes_a, axes_b, 1);

	ASSERT_NOT_NULL(c);
	ASSERT_EQ(c->shape[0], 3);
	ASSERT_EQ(c->shape[1], 5);

	tensorfree(a);
	tensorfree(b);
	tensorfree(c);
}

TEST(contract_multiple_axes)
{
	int shape_a[3] = {2, 3, 4};
	int shape_b[3] = {3, 4, 5};
	Tensor *a = tensorones(3, shape_a);
	Tensor *b = tensorones(3, shape_b);

	int axes_a[2] = {1, 2};
	int axes_b[2] = {0, 1};

	Tensor *c = tensorcontract(a, b, axes_a, axes_b, 2);

	ASSERT_NOT_NULL(c);
	ASSERT_EQ(c->shape[0], 2);
	ASSERT_EQ(c->shape[1], 5);

	tensorfree(a);
	tensorfree(b);
	tensorfree(c);
}

/* ========== Nonlinearity Tests ========== */

TEST(tensor_step)
{
	int shape[1] = {5};
	Tensor *t = tensorcreate(TensorFloat, 1, shape);

	float *data = (float*)t->data;
	data[0] = -2.0;
	data[1] = -0.5;
	data[2] = 0.0;
	data[3] = 0.5;
	data[4] = 2.0;

	Tensor *s = tensorstep(t);

	ASSERT_NOT_NULL(s);

	float *sdata = (float*)s->data;
	ASSERT_FLOAT_EQ(sdata[0], 0.0);
	ASSERT_FLOAT_EQ(sdata[1], 0.0);
	ASSERT_FLOAT_EQ(sdata[2], 0.0);
	ASSERT_FLOAT_EQ(sdata[3], 1.0);
	ASSERT_FLOAT_EQ(sdata[4], 1.0);

	tensorfree(t);
	tensorfree(s);
}

TEST(tensor_sigmoid)
{
	int shape[1] = {3};
	Tensor *t = tensorzeros(1, shape);

	Tensor *s = tensorsigmoid(t);

	ASSERT_NOT_NULL(s);

	/* sigmoid(0) = 0.5 */
	float *sdata = (float*)s->data;
	ASSERT_FLOAT_EQ(sdata[0], 0.5);
	ASSERT_FLOAT_EQ(sdata[1], 0.5);
	ASSERT_FLOAT_EQ(sdata[2], 0.5);

	tensorfree(t);
	tensorfree(s);
}

TEST(tensor_sigmoid_temp)
{
	int shape[1] = {3};
	Tensor *t = tensorones(1, shape);

	/* Low temperature -> sharper */
	Tensor *s_low = tensorsigmoidt(t, 0.1);
	/* High temperature -> softer */
	Tensor *s_high = tensorsigmoidt(t, 10.0);

	ASSERT_NOT_NULL(s_low);
	ASSERT_NOT_NULL(s_high);

	/* Low temp should be closer to 1.0 */
	float *slow = (float*)s_low->data;
	float *shigh = (float*)s_high->data;
	ASSERT_GT(slow[0], shigh[0]);

	tensorfree(t);
	tensorfree(s_low);
	tensorfree(s_high);
}

TEST(tensor_tanh)
{
	int shape[1] = {1};
	Tensor *t = tensorzeros(1, shape);

	Tensor *s = tensortanh(t);

	ASSERT_NOT_NULL(s);

	/* tanh(0) = 0 */
	float *sdata = (float*)s->data;
	ASSERT_FLOAT_EQ(sdata[0], 0.0);

	tensorfree(t);
	tensorfree(s);
}

TEST(tensor_relu)
{
	int shape[1] = {5};
	Tensor *t = tensorcreate(TensorFloat, 1, shape);

	float *data = (float*)t->data;
	data[0] = -2.0;
	data[1] = -0.5;
	data[2] = 0.0;
	data[3] = 0.5;
	data[4] = 2.0;

	Tensor *r = tensorrelu(t);

	ASSERT_NOT_NULL(r);

	float *rdata = (float*)r->data;
	ASSERT_FLOAT_EQ(rdata[0], 0.0);
	ASSERT_FLOAT_EQ(rdata[1], 0.0);
	ASSERT_FLOAT_EQ(rdata[2], 0.0);
	ASSERT_FLOAT_EQ(rdata[3], 0.5);
	ASSERT_FLOAT_EQ(rdata[4], 2.0);

	tensorfree(t);
	tensorfree(r);
}

TEST(tensor_softmax)
{
	int shape[1] = {3};
	Tensor *t = tensorones(1, shape);

	Tensor *s = tensorsoftmax(t, 0);

	ASSERT_NOT_NULL(s);

	/* Softmax of equal values should give uniform distribution */
	float *sdata = (float*)s->data;
	ASSERT_FLOAT_EQ(sdata[0], 0.333);
	ASSERT_FLOAT_EQ(sdata[1], 0.333);
	ASSERT_FLOAT_EQ(sdata[2], 0.333);

	/* Sum should be 1.0 */
	float sum = tensorsum(s);
	ASSERT_FLOAT_EQ(sum, 1.0);

	tensorfree(t);
	tensorfree(s);
}

TEST(tensor_softmax_temp)
{
	int shape[1] = {2};
	Tensor *t = tensorcreate(TensorFloat, 1, shape);
	float *data = (float*)t->data;
	data[0] = 1.0;
	data[1] = 2.0;

	Tensor *s_cold = tensorsoftmaxt(t, 0, 0.1);
	Tensor *s_hot = tensorsoftmaxt(t, 0, 10.0);

	ASSERT_NOT_NULL(s_cold);
	ASSERT_NOT_NULL(s_hot);

	/* Cold (low temp) -> more peaked */
	float *cold = (float*)s_cold->data;
	float *hot = (float*)s_hot->data;

	/* Higher value should have higher probability in cold case */
	ASSERT_GT(cold[1], hot[1]);

	tensorfree(t);
	tensorfree(s_cold);
	tensorfree(s_hot);
}

TEST(tensor_apply)
{
	int shape[1] = {3};
	Tensor *t = tensorzeros(1, shape);

	Tensor *r1 = tensorapply(t, TensorSigmoid);
	ASSERT_NOT_NULL(r1);

	Tensor *r2 = tensorapply(t, TensorRelu);
	ASSERT_NOT_NULL(r2);

	Tensor *r3 = tensorapply(t, TensorNoOp);
	ASSERT_NOT_NULL(r3);

	tensorfree(t);
	tensorfree(r1);
	tensorfree(r2);
	tensorfree(r3);
}

/* ========== Tensor Equation Tests ========== */

TEST(tenseq_create)
{
	TensorEquation *eq = tenseqcreate("ik", "ij,jk", TensorNoOp);

	ASSERT_NOT_NULL(eq);
	ASSERT_EQ(eq->nonlinearity, TensorNoOp);

	tenseqfree(eq);
}

TEST(tenseq_create_with_nonlin)
{
	TensorEquation *eq = tenseqcreate("i", "ij,j", TensorSigmoid);

	ASSERT_NOT_NULL(eq);
	ASSERT_EQ(eq->nonlinearity, TensorSigmoid);

	tenseqfree(eq);
}

TEST(tenseq_eval_matmul)
{
	TensorEquation *eq = tenseqcreate("ik", "ij,jk", TensorNoOp);

	int shape_a[2] = {2, 3};
	int shape_b[2] = {3, 4};
	Tensor *a = tensorones(2, shape_a);
	Tensor *b = tensorones(2, shape_b);

	Tensor *inputs[2] = {a, b};
	Tensor *c = tenseqeval(eq, inputs, 2);

	ASSERT_NOT_NULL(c);
	ASSERT_EQ(c->shape[0], 2);
	ASSERT_EQ(c->shape[1], 4);

	tensorfree(a);
	tensorfree(b);
	tensorfree(c);
	tenseqfree(eq);
}

TEST(tenseq_eval_with_step)
{
	TensorEquation *eq = tenseqcreate("i", "ij,j", TensorStep);

	int shape_a[2] = {2, 2};
	int shape_b[1] = {2};
	Tensor *a = tensorones(2, shape_a);
	Tensor *b = tensorones(1, shape_b);

	Tensor *inputs[2] = {a, b};
	Tensor *c = tenseqeval(eq, inputs, 2);

	ASSERT_NOT_NULL(c);

	/* Step should produce 0 or 1 */
	float max = tensormax(c);
	float min = tensormin(c);
	ASSERT_LE(max, 1.0);
	ASSERT_GE(min, 0.0);

	tensorfree(a);
	tensorfree(b);
	tensorfree(c);
	tenseqfree(eq);
}

TEST(tenseq_free_null)
{
	/* Should not crash */
	tenseqfree(nil);
}

/* ========== Edge Cases ========== */

TEST(einsum_null_tensor)
{
	Tensor *c = tensoreinsum("ij,jk->ik", nil, nil);
	ASSERT_NULL(c);
}

TEST(einsum_mismatched_dims)
{
	/* Should handle dimension mismatch gracefully */
	int shape_a[2] = {2, 3};
	int shape_b[2] = {4, 5};  /* Incompatible with "ij,jk->ik" */
	Tensor *a = tensorones(2, shape_a);
	Tensor *b = tensorones(2, shape_b);

	/* May return nil or handle error gracefully */
	Tensor *c = tensoreinsum("ij,jk->ik", a, b);
	/* Either nil or some error handling */

	tensorfree(a);
	tensorfree(b);
	if(c != nil)
		tensorfree(c);
}

TEST(einsum_single_tensor)
{
	int shape[2] = {3, 3};
	Tensor *a = tensorones(2, shape);

	/* Single tensor sum */
	Tensor *c = tensoreinsum("ij->", a, nil);

	ASSERT_NOT_NULL(c);
	float sum = tensorsum(c);
	ASSERT_FLOAT_EQ(sum, 9.0);

	tensorfree(a);
	tensorfree(c);
}

/* ========== Main Test Runner ========== */

void
main(int argc, char *argv[])
{
	print("=== Einstein Summation Unit Tests ===\n\n");

	print("Basic Einsum Tests:\n");
	RUN_TEST(einsum_matmul);
	RUN_TEST(einsum_inner_product);
	RUN_TEST(einsum_outer_product);
	RUN_TEST(einsum_transpose);
	RUN_TEST(einsum_trace);
	RUN_TEST(einsum_diagonal);
	RUN_TEST(einsum_batch_matmul);
	RUN_TEST(einsum_hadamard);
	RUN_TEST(einsum_sum_all);
	RUN_TEST(einsum_sum_row);
	RUN_TEST(einsum_sum_col);

	print("\nMulti-Tensor Einsum Tests:\n");
	RUN_TEST(einsum3_chain);
	RUN_TEST(einsumv_multiple);

	print("\nTensor Contraction Tests:\n");
	RUN_TEST(contract_single_axis);
	RUN_TEST(contract_multiple_axes);

	print("\nNonlinearity Tests:\n");
	RUN_TEST(tensor_step);
	RUN_TEST(tensor_sigmoid);
	RUN_TEST(tensor_sigmoid_temp);
	RUN_TEST(tensor_tanh);
	RUN_TEST(tensor_relu);
	RUN_TEST(tensor_softmax);
	RUN_TEST(tensor_softmax_temp);
	RUN_TEST(tensor_apply);

	print("\nTensor Equation Tests:\n");
	RUN_TEST(tenseq_create);
	RUN_TEST(tenseq_create_with_nonlin);
	RUN_TEST(tenseq_eval_matmul);
	RUN_TEST(tenseq_eval_with_step);
	RUN_TEST(tenseq_free_null);

	print("\nEdge Case Tests:\n");
	RUN_TEST(einsum_null_tensor);
	RUN_TEST(einsum_mismatched_dims);
	RUN_TEST(einsum_single_tensor);

	print("\n=== Test Summary ===\n");
	print("Tests run: %d\n", tests_run);
	print("Tests passed: %d\n", tests_passed);
	print("Tests failed: %d\n", tests_failed);

	exits(tests_failed > 0 ? "FAIL" : nil);
}
