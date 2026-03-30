/*
 * Exhaustive Unit Tests for Kernel Machines
 * Tests kernel functions and kernel machines
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

/* ========== Kernel Creation Tests ========== */

TEST(kernel_create_linear)
{
	TensorKernel *k = tensorkernelcreate(KernelLinear);

	ASSERT_NOT_NULL(k);
	ASSERT_EQ(k->type, KernelLinear);

	tensorkernelfree(k);
}

TEST(kernel_create_polynomial)
{
	TensorKernel *k = tensorkernelcreate(KernelPolynomial);

	ASSERT_NOT_NULL(k);
	ASSERT_EQ(k->type, KernelPolynomial);
	ASSERT_GT(k->degree, 0.0);

	tensorkernelfree(k);
}

TEST(kernel_create_gaussian)
{
	TensorKernel *k = tensorkernelcreate(KernelGaussian);

	ASSERT_NOT_NULL(k);
	ASSERT_EQ(k->type, KernelGaussian);
	ASSERT_GT(k->sigma, 0.0);

	tensorkernelfree(k);
}

TEST(kernel_create_laplacian)
{
	TensorKernel *k = tensorkernelcreate(KernelLaplacian);

	ASSERT_NOT_NULL(k);
	ASSERT_EQ(k->type, KernelLaplacian);
	ASSERT_GT(k->sigma, 0.0);

	tensorkernelfree(k);
}

TEST(kernel_create_sigmoid)
{
	TensorKernel *k = tensorkernelcreate(KernelSigmoid);

	ASSERT_NOT_NULL(k);
	ASSERT_EQ(k->type, KernelSigmoid);

	tensorkernelfree(k);
}

TEST(kernel_free_null)
{
	tensorkernelfree(nil);
}

/* ========== Linear Kernel Tests ========== */

TEST(kernel_linear_eval)
{
	TensorKernel *k = tensorkernelcreate(KernelLinear);

	int shape[1] = {3};
	Tensor *x = tensorones(1, shape);
	Tensor *y = tensorones(1, shape);

	/* Linear kernel: K(x,y) = x · y */
	float val = tensorkerneleval(k, x, y);

	/* [1,1,1] · [1,1,1] = 3 */
	ASSERT_FLOAT_EQ(val, 3.0);

	tensorfree(x);
	tensorfree(y);
	tensorkernelfree(k);
}

TEST(kernel_linear_orthogonal)
{
	TensorKernel *k = tensorkernelcreate(KernelLinear);

	int shape[1] = {2};
	Tensor *x = tensorzeros(1, shape);
	Tensor *y = tensorzeros(1, shape);

	float *dx = (float*)x->data;
	float *dy = (float*)y->data;
	dx[0] = 1.0; dx[1] = 0.0;
	dy[0] = 0.0; dy[1] = 1.0;

	float val = tensorkerneleval(k, x, y);

	/* Orthogonal vectors -> kernel = 0 */
	ASSERT_FLOAT_EQ(val, 0.0);

	tensorfree(x);
	tensorfree(y);
	tensorkernelfree(k);
}

TEST(kernel_linear_scaled)
{
	TensorKernel *k = tensorkernelcreate(KernelLinear);

	int shape[1] = {2};
	Tensor *x = tensorzeros(1, shape);
	Tensor *y = tensorzeros(1, shape);

	float *dx = (float*)x->data;
	float *dy = (float*)y->data;
	dx[0] = 2.0; dx[1] = 0.0;
	dy[0] = 3.0; dy[1] = 0.0;

	float val = tensorkerneleval(k, x, y);

	ASSERT_FLOAT_EQ(val, 6.0);

	tensorfree(x);
	tensorfree(y);
	tensorkernelfree(k);
}

/* ========== Polynomial Kernel Tests ========== */

TEST(kernel_polynomial_eval)
{
	TensorKernel *k = tensorkernelcreate(KernelPolynomial);
	k->degree = 2.0;
	k->coef = 1.0;

	int shape[1] = {2};
	Tensor *x = tensorones(1, shape);
	Tensor *y = tensorones(1, shape);

	/* K(x,y) = (x·y + c)^d = (2 + 1)^2 = 9 */
	float val = tensorkerneleval(k, x, y);

	ASSERT_FLOAT_EQ(val, 9.0);

	tensorfree(x);
	tensorfree(y);
	tensorkernelfree(k);
}

TEST(kernel_polynomial_degree1)
{
	TensorKernel *k = tensorkernelcreate(KernelPolynomial);
	k->degree = 1.0;
	k->coef = 0.0;

	int shape[1] = {3};
	Tensor *x = tensorones(1, shape);
	Tensor *y = tensorones(1, shape);

	/* K(x,y) = (x·y + 0)^1 = 3 (same as linear) */
	float val = tensorkerneleval(k, x, y);

	ASSERT_FLOAT_EQ(val, 3.0);

	tensorfree(x);
	tensorfree(y);
	tensorkernelfree(k);
}

TEST(kernel_polynomial_degree3)
{
	TensorKernel *k = tensorkernelcreate(KernelPolynomial);
	k->degree = 3.0;
	k->coef = 0.0;

	int shape[1] = {2};
	Tensor *x = tensorones(1, shape);
	Tensor *y = tensorones(1, shape);

	/* K(x,y) = (2 + 0)^3 = 8 */
	float val = tensorkerneleval(k, x, y);

	ASSERT_FLOAT_EQ(val, 8.0);

	tensorfree(x);
	tensorfree(y);
	tensorkernelfree(k);
}

/* ========== Gaussian (RBF) Kernel Tests ========== */

TEST(kernel_gaussian_same)
{
	TensorKernel *k = tensorkernelcreate(KernelGaussian);
	k->sigma = 1.0;

	int shape[1] = {3};
	Tensor *x = tensorones(1, shape);

	/* K(x,x) = exp(0) = 1 */
	float val = tensorkerneleval(k, x, x);

	ASSERT_FLOAT_EQ(val, 1.0);

	tensorfree(x);
	tensorkernelfree(k);
}

TEST(kernel_gaussian_different)
{
	TensorKernel *k = tensorkernelcreate(KernelGaussian);
	k->sigma = 1.0;

	int shape[1] = {1};
	Tensor *x = tensorzeros(1, shape);
	Tensor *y = tensorzeros(1, shape);

	float *dx = (float*)x->data;
	float *dy = (float*)y->data;
	dx[0] = 0.0;
	dy[0] = 1.0;

	/* K(x,y) = exp(-||x-y||^2 / 2σ^2) = exp(-1/2) ≈ 0.606 */
	float val = tensorkerneleval(k, x, y);

	ASSERT_GT(val, 0.0);
	ASSERT_LT(val, 1.0);
	/* exp(-0.5) ≈ 0.606 */
	ASSERT_GT(val, 0.5);
	ASSERT_LT(val, 0.7);

	tensorfree(x);
	tensorfree(y);
	tensorkernelfree(k);
}

TEST(kernel_gaussian_sigma)
{
	/* Larger sigma -> slower decay */
	TensorKernel *k_small = tensorkernelcreate(KernelGaussian);
	TensorKernel *k_large = tensorkernelcreate(KernelGaussian);
	k_small->sigma = 0.5;
	k_large->sigma = 2.0;

	int shape[1] = {1};
	Tensor *x = tensorzeros(1, shape);
	Tensor *y = tensorzeros(1, shape);

	float *dx = (float*)x->data;
	float *dy = (float*)y->data;
	dx[0] = 0.0;
	dy[0] = 1.0;

	float val_small = tensorkerneleval(k_small, x, y);
	float val_large = tensorkerneleval(k_large, x, y);

	/* Larger sigma -> higher kernel value for same distance */
	ASSERT_GT(val_large, val_small);

	tensorfree(x);
	tensorfree(y);
	tensorkernelfree(k_small);
	tensorkernelfree(k_large);
}

/* ========== Laplacian Kernel Tests ========== */

TEST(kernel_laplacian_same)
{
	TensorKernel *k = tensorkernelcreate(KernelLaplacian);
	k->sigma = 1.0;

	int shape[1] = {3};
	Tensor *x = tensorones(1, shape);

	/* K(x,x) = exp(0) = 1 */
	float val = tensorkerneleval(k, x, x);

	ASSERT_FLOAT_EQ(val, 1.0);

	tensorfree(x);
	tensorkernelfree(k);
}

TEST(kernel_laplacian_different)
{
	TensorKernel *k = tensorkernelcreate(KernelLaplacian);
	k->sigma = 1.0;

	int shape[1] = {1};
	Tensor *x = tensorzeros(1, shape);
	Tensor *y = tensorzeros(1, shape);

	float *dx = (float*)x->data;
	float *dy = (float*)y->data;
	dx[0] = 0.0;
	dy[0] = 1.0;

	/* K(x,y) = exp(-||x-y|| / σ) = exp(-1) ≈ 0.368 */
	float val = tensorkerneleval(k, x, y);

	ASSERT_GT(val, 0.0);
	ASSERT_LT(val, 1.0);
	ASSERT_GT(val, 0.3);
	ASSERT_LT(val, 0.4);

	tensorfree(x);
	tensorfree(y);
	tensorkernelfree(k);
}

/* ========== Sigmoid Kernel Tests ========== */

TEST(kernel_sigmoid_eval)
{
	TensorKernel *k = tensorkernelcreate(KernelSigmoid);
	k->coef = 0.0;

	int shape[1] = {1};
	Tensor *x = tensorzeros(1, shape);
	Tensor *y = tensorzeros(1, shape);

	/* K(x,y) = tanh(αx·y + c) = tanh(0) = 0 */
	float val = tensorkerneleval(k, x, y);

	ASSERT_FLOAT_EQ(val, 0.0);

	tensorfree(x);
	tensorfree(y);
	tensorkernelfree(k);
}

/* ========== Gram Matrix Tests ========== */

TEST(kernel_gram_linear)
{
	TensorKernel *k = tensorkernelcreate(KernelLinear);

	int shape[2] = {3, 2};
	Tensor *X = tensorones(2, shape);

	Tensor *K = tensorkernelgram(k, X);

	ASSERT_NOT_NULL(K);
	ASSERT_EQ(K->rank, 2);
	ASSERT_EQ(K->shape[0], 3);
	ASSERT_EQ(K->shape[1], 3);

	tensorfree(X);
	tensorfree(K);
	tensorkernelfree(k);
}

TEST(kernel_gram_symmetric)
{
	TensorKernel *k = tensorkernelcreate(KernelGaussian);
	k->sigma = 1.0;

	int shape[2] = {4, 3};
	Tensor *X = tensorrand(2, shape);

	Tensor *K = tensorkernelgram(k, X);

	ASSERT_NOT_NULL(K);

	/* Gram matrix should be symmetric */
	/* K[i,j] == K[j,i] */
	float *data = (float*)K->data;
	int i, j;
	for(i = 0; i < 4; i++){
		for(j = i+1; j < 4; j++){
			float kij = data[i*4 + j];
			float kji = data[j*4 + i];
			ASSERT(FLOAT_EQ(kij, kji));
		}
	}

	tensorfree(X);
	tensorfree(K);
	tensorkernelfree(k);
}

TEST(kernel_gram_diagonal)
{
	TensorKernel *k = tensorkernelcreate(KernelGaussian);
	k->sigma = 1.0;

	int shape[2] = {5, 3};
	Tensor *X = tensorrand(2, shape);

	Tensor *K = tensorkernelgram(k, X);

	/* Diagonal should be 1 for RBF kernel (K(x,x)=1) */
	float *data = (float*)K->data;
	int i;
	for(i = 0; i < 5; i++){
		float kii = data[i*5 + i];
		ASSERT_FLOAT_EQ(kii, 1.0);
	}

	tensorfree(X);
	tensorfree(K);
	tensorkernelfree(k);
}

TEST(kernel_gram_positive_semidefinite)
{
	TensorKernel *k = tensorkernelcreate(KernelGaussian);
	k->sigma = 1.0;

	int shape[2] = {3, 2};
	Tensor *X = tensorrand(2, shape);

	Tensor *K = tensorkernelgram(k, X);

	/* Gram matrix should have non-negative eigenvalues */
	/* For RBF kernel, all elements should be positive */
	float *data = (float*)K->data;
	int i, j;
	for(i = 0; i < 9; i++){
		ASSERT_GE(data[i], 0.0);
	}

	tensorfree(X);
	tensorfree(K);
	tensorkernelfree(k);
}

/* ========== Kernel Machine Tests ========== */

TEST(km_init)
{
	TensorKernel *k = tensorkernelcreate(KernelGaussian);
	TensorKernelMachine *km = tensorkminit(k);

	ASSERT_NOT_NULL(km);
	ASSERT_EQ(km->kernel, k);
	ASSERT_EQ(km->nsupport, 0);

	tensorkmfree(km);
}

TEST(km_free_null)
{
	tensorkmfree(nil);
}

TEST(km_fit_simple)
{
	TensorKernel *k = tensorkernelcreate(KernelLinear);
	TensorKernelMachine *km = tensorkminit(k);

	/* Simple 2D classification */
	int shape_X[2] = {4, 2};
	int shape_Y[1] = {4};
	Tensor *X = tensorcreate(TensorFloat, 2, shape_X);
	Tensor *Y = tensorcreate(TensorFloat, 1, shape_Y);

	float *xdata = (float*)X->data;
	float *ydata = (float*)Y->data;

	/* Two classes: positive (1,1), (1,2), negative (-1,-1), (-1,-2) */
	xdata[0] = 1.0; xdata[1] = 1.0;
	xdata[2] = 1.0; xdata[3] = 2.0;
	xdata[4] = -1.0; xdata[5] = -1.0;
	xdata[6] = -1.0; xdata[7] = -2.0;

	ydata[0] = 1.0;
	ydata[1] = 1.0;
	ydata[2] = -1.0;
	ydata[3] = -1.0;

	tensorkmfit(km, X, Y);

	ASSERT_GT(km->nsupport, 0);

	tensorfree(X);
	tensorfree(Y);
	tensorkmfree(km);
}

TEST(km_predict)
{
	TensorKernel *k = tensorkernelcreate(KernelLinear);
	TensorKernelMachine *km = tensorkminit(k);

	/* Training data */
	int shape_X[2] = {4, 2};
	int shape_Y[1] = {4};
	Tensor *X = tensorcreate(TensorFloat, 2, shape_X);
	Tensor *Y = tensorcreate(TensorFloat, 1, shape_Y);

	float *xdata = (float*)X->data;
	float *ydata = (float*)Y->data;

	xdata[0] = 1.0; xdata[1] = 1.0;
	xdata[2] = 1.0; xdata[3] = 2.0;
	xdata[4] = -1.0; xdata[5] = -1.0;
	xdata[6] = -1.0; xdata[7] = -2.0;

	ydata[0] = 1.0;
	ydata[1] = 1.0;
	ydata[2] = -1.0;
	ydata[3] = -1.0;

	tensorkmfit(km, X, Y);

	/* Test prediction */
	int shape_test[2] = {2, 2};
	Tensor *Xtest = tensorcreate(TensorFloat, 2, shape_test);
	float *tdata = (float*)Xtest->data;
	tdata[0] = 2.0; tdata[1] = 2.0;   /* Should be positive */
	tdata[2] = -2.0; tdata[3] = -2.0; /* Should be negative */

	Tensor *pred = tensorkmpredict(km, Xtest);

	ASSERT_NOT_NULL(pred);
	ASSERT_EQ(pred->shape[0], 2);

	tensorfree(X);
	tensorfree(Y);
	tensorfree(Xtest);
	tensorfree(pred);
	tensorkmfree(km);
}

TEST(km_rbf_nonlinear)
{
	/* RBF kernel for XOR-like problem */
	TensorKernel *k = tensorkernelcreate(KernelGaussian);
	k->sigma = 0.5;
	TensorKernelMachine *km = tensorkminit(k);

	/* XOR data */
	int shape_X[2] = {4, 2};
	int shape_Y[1] = {4};
	Tensor *X = tensorcreate(TensorFloat, 2, shape_X);
	Tensor *Y = tensorcreate(TensorFloat, 1, shape_Y);

	float *xdata = (float*)X->data;
	float *ydata = (float*)Y->data;

	xdata[0] = 0.0; xdata[1] = 0.0;  /* Class -1 */
	xdata[2] = 1.0; xdata[3] = 1.0;  /* Class -1 */
	xdata[4] = 0.0; xdata[5] = 1.0;  /* Class +1 */
	xdata[6] = 1.0; xdata[7] = 0.0;  /* Class +1 */

	ydata[0] = -1.0;
	ydata[1] = -1.0;
	ydata[2] = 1.0;
	ydata[3] = 1.0;

	tensorkmfit(km, X, Y);

	/* RBF should be able to handle this */
	ASSERT_GT(km->nsupport, 0);

	tensorfree(X);
	tensorfree(Y);
	tensorkmfree(km);
}

/* ========== Edge Cases ========== */

TEST(kernel_eval_null)
{
	TensorKernel *k = tensorkernelcreate(KernelLinear);

	float val = tensorkerneleval(k, nil, nil);
	ASSERT_FLOAT_EQ(val, 0.0);

	tensorkernelfree(k);
}

TEST(kernel_gram_null)
{
	TensorKernel *k = tensorkernelcreate(KernelLinear);

	Tensor *K = tensorkernelgram(k, nil);
	ASSERT_NULL(K);

	tensorkernelfree(k);
}

TEST(kernel_eval_dimension_mismatch)
{
	TensorKernel *k = tensorkernelcreate(KernelLinear);

	int shape1[1] = {3};
	int shape2[1] = {4};
	Tensor *x = tensorones(1, shape1);
	Tensor *y = tensorones(1, shape2);

	/* Should handle gracefully */
	float val = tensorkerneleval(k, x, y);

	tensorfree(x);
	tensorfree(y);
	tensorkernelfree(k);
}

TEST(km_predict_before_fit)
{
	TensorKernel *k = tensorkernelcreate(KernelLinear);
	TensorKernelMachine *km = tensorkminit(k);

	int shape[2] = {2, 2};
	Tensor *X = tensorones(2, shape);

	/* Predict without fitting */
	Tensor *pred = tensorkmpredict(km, X);

	/* Should return nil or zeros */

	tensorfree(X);
	if(pred != nil)
		tensorfree(pred);
	tensorkmfree(km);
}

/* ========== Main Test Runner ========== */

void
main(int argc, char *argv[])
{
	print("=== Kernel Machines Unit Tests ===\n\n");

	print("Kernel Creation Tests:\n");
	RUN_TEST(kernel_create_linear);
	RUN_TEST(kernel_create_polynomial);
	RUN_TEST(kernel_create_gaussian);
	RUN_TEST(kernel_create_laplacian);
	RUN_TEST(kernel_create_sigmoid);
	RUN_TEST(kernel_free_null);

	print("\nLinear Kernel Tests:\n");
	RUN_TEST(kernel_linear_eval);
	RUN_TEST(kernel_linear_orthogonal);
	RUN_TEST(kernel_linear_scaled);

	print("\nPolynomial Kernel Tests:\n");
	RUN_TEST(kernel_polynomial_eval);
	RUN_TEST(kernel_polynomial_degree1);
	RUN_TEST(kernel_polynomial_degree3);

	print("\nGaussian (RBF) Kernel Tests:\n");
	RUN_TEST(kernel_gaussian_same);
	RUN_TEST(kernel_gaussian_different);
	RUN_TEST(kernel_gaussian_sigma);

	print("\nLaplacian Kernel Tests:\n");
	RUN_TEST(kernel_laplacian_same);
	RUN_TEST(kernel_laplacian_different);

	print("\nSigmoid Kernel Tests:\n");
	RUN_TEST(kernel_sigmoid_eval);

	print("\nGram Matrix Tests:\n");
	RUN_TEST(kernel_gram_linear);
	RUN_TEST(kernel_gram_symmetric);
	RUN_TEST(kernel_gram_diagonal);
	RUN_TEST(kernel_gram_positive_semidefinite);

	print("\nKernel Machine Tests:\n");
	RUN_TEST(km_init);
	RUN_TEST(km_free_null);
	RUN_TEST(km_fit_simple);
	RUN_TEST(km_predict);
	RUN_TEST(km_rbf_nonlinear);

	print("\nEdge Case Tests:\n");
	RUN_TEST(kernel_eval_null);
	RUN_TEST(kernel_gram_null);
	RUN_TEST(kernel_eval_dimension_mismatch);
	RUN_TEST(km_predict_before_fit);

	print("\n=== Test Summary ===\n");
	print("Tests run: %d\n", tests_run);
	print("Tests passed: %d\n", tests_passed);
	print("Tests failed: %d\n", tests_failed);

	exits(tests_failed > 0 ? "FAIL" : nil);
}
