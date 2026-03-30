/*
 * Kernel Machines in Tensor Logic
 * Y[Q] = f(A[i]Y[i]K[Q,i] + B)
 */

#include <u.h>
#include <libc.h>
#include <plan9cog/tensorlogic.h>

/* ============================================================
 * Kernel Functions
 * ============================================================ */

/* Create kernel */
TensorKernel*
tensorkernelcreate(int type)
{
	TensorKernel *k;

	k = mallocz(sizeof(TensorKernel), 1);
	if(k == nil)
		return nil;

	k->type = type;
	k->degree = 3.0;	/* Default polynomial degree */
	k->sigma = 1.0;		/* Default Gaussian sigma */
	k->coef = 0.0;		/* Default coefficient */

	return k;
}

/* Free kernel */
void
tensorkernelfree(TensorKernel *k)
{
	free(k);
}

/* Evaluate kernel between two vectors */
float
tensorkerneleval(TensorKernel *k, Tensor *x, Tensor *y)
{
	float *xd, *yd;
	float dot, diff, result;
	ulong i;

	if(k == nil || x == nil || y == nil)
		return 0.0;

	if(x->nelems != y->nelems)
		return 0.0;

	xd = (float*)x->data;
	yd = (float*)y->data;

	/* Compute dot product */
	dot = 0.0;
	for(i = 0; i < x->nelems; i++)
		dot += xd[i] * yd[i];

	switch(k->type){
	case KernelLinear:
		/* K(x,y) = x · y */
		result = dot;
		break;

	case KernelPolynomial:
		/* K(x,y) = (x · y + c)^d */
		result = pow(dot + k->coef, k->degree);
		break;

	case KernelGaussian:
		/* K(x,y) = exp(-||x-y||² / 2σ²) */
		diff = 0.0;
		for(i = 0; i < x->nelems; i++){
			float d = xd[i] - yd[i];
			diff += d * d;
		}
		result = exp(-diff / (2.0 * k->sigma * k->sigma));
		break;

	case KernelLaplacian:
		/* K(x,y) = exp(-||x-y|| / σ) */
		diff = 0.0;
		for(i = 0; i < x->nelems; i++){
			float d = xd[i] - yd[i];
			diff += d * d;
		}
		result = exp(-sqrt(diff) / k->sigma);
		break;

	case KernelSigmoid:
		/* K(x,y) = tanh(αx·y + c) */
		result = tanh(k->degree * dot + k->coef);
		break;

	default:
		result = dot;
	}

	return result;
}

/* Compute Gram matrix */
Tensor*
tensorkernelgram(TensorKernel *k, Tensor *X)
{
	Tensor *gram;
	float *xd, *gd;
	int n, d, i, j, l;
	int shape[2];
	Tensor *xi, *xj;
	int vshape[1];

	if(k == nil || X == nil)
		return nil;

	if(X->rank != 2)
		return nil;

	n = X->shape[0];
	d = X->shape[1];

	shape[0] = n;
	shape[1] = n;
	gram = tensorcreate(TensorFloat, 2, shape);
	if(gram == nil)
		return nil;

	xd = (float*)X->data;
	gd = (float*)gram->data;

	/* Create temporary vectors */
	vshape[0] = d;
	xi = tensorcreate(TensorFloat, 1, vshape);
	xj = tensorcreate(TensorFloat, 1, vshape);
	if(xi == nil || xj == nil){
		tensorfree(gram);
		tensorfree(xi);
		tensorfree(xj);
		return nil;
	}

	/* Compute all pairs */
	for(i = 0; i < n; i++){
		/* Copy row i to xi */
		for(l = 0; l < d; l++)
			((float*)xi->data)[l] = xd[i * d + l];

		for(j = i; j < n; j++){
			/* Copy row j to xj */
			for(l = 0; l < d; l++)
				((float*)xj->data)[l] = xd[j * d + l];

			float kval = tensorkerneleval(k, xi, xj);
			gd[i * n + j] = kval;
			gd[j * n + i] = kval;  /* Symmetric */
		}
	}

	tensorfree(xi);
	tensorfree(xj);

	return gram;
}

/* ============================================================
 * Kernel Machine
 * ============================================================ */

/* Initialize kernel machine */
TensorKernelMachine*
tensorkminit(TensorKernel *kernel)
{
	TensorKernelMachine *km;

	km = mallocz(sizeof(TensorKernelMachine), 1);
	if(km == nil)
		return nil;

	km->kernel = kernel;
	km->support = nil;
	km->alpha = nil;
	km->bias = 0.0;
	km->nsupport = 0;

	return km;
}

/* Free kernel machine */
void
tensorkmfree(TensorKernelMachine *km)
{
	if(km == nil)
		return;

	tensorfree(km->support);
	tensorfree(km->alpha);
	free(km);
}

/* Fit kernel machine (simplified kernel ridge regression) */
void
tensorkmfit(TensorKernelMachine *km, Tensor *X, Tensor *Y)
{
	Tensor *gram, *gramreg;
	float *gd, *yd, *ad;
	int n, i, j;
	float lambda = 0.01;  /* Regularization */

	if(km == nil || X == nil || Y == nil)
		return;

	if(X->rank != 2 || Y->rank != 1)
		return;

	n = X->shape[0];

	/* Store support vectors */
	km->support = tensorcopy(X);
	km->nsupport = n;

	/* Compute Gram matrix */
	gram = tensorkernelgram(km->kernel, X);
	if(gram == nil)
		return;

	/* Add regularization: K + λI */
	gd = (float*)gram->data;
	for(i = 0; i < n; i++)
		gd[i * n + i] += lambda;

	/* Solve (K + λI)α = Y */
	/* Simplified: use direct solve via Cholesky or iterative method */
	/* Here we use simple gradient descent */

	int shape[1] = {n};
	km->alpha = tensorcreate(TensorFloat, 1, shape);
	if(km->alpha == nil){
		tensorfree(gram);
		return;
	}

	yd = (float*)Y->data;
	ad = (float*)km->alpha->data;

	/* Initialize alpha to zeros */
	for(i = 0; i < n; i++)
		ad[i] = 0.0;

	/* Gradient descent */
	float lr = 0.01;
	int iters = 1000;
	int iter;

	for(iter = 0; iter < iters; iter++){
		/* Compute gradient: (K + λI)α - Y */
		for(i = 0; i < n; i++){
			float pred = 0.0;
			for(j = 0; j < n; j++)
				pred += gd[i * n + j] * ad[j];

			float grad = pred - yd[i];
			ad[i] -= lr * grad;
		}
	}

	tensorfree(gram);

	/* Compute bias as mean of residuals */
	km->bias = 0.0;
	for(i = 0; i < n; i++){
		float pred = 0.0;
		for(j = 0; j < n; j++){
			/* Compute kernel with support vector j */
			Tensor *xi = tensorembget(nil, i);  /* placeholder */
			pred += ad[j];  /* Simplified */
		}
		km->bias += yd[i] - pred;
	}
	km->bias /= n;
}

/* Predict with kernel machine */
Tensor*
tensorkmpredict(TensorKernelMachine *km, Tensor *X)
{
	Tensor *pred;
	float *xd, *sd, *ad, *pd;
	int ntest, nsup, d, i, j, l;
	int shape[1];
	Tensor *xi, *sj;
	int vshape[1];

	if(km == nil || X == nil || km->support == nil || km->alpha == nil)
		return nil;

	if(X->rank != 2)
		return nil;

	ntest = X->shape[0];
	d = X->shape[1];
	nsup = km->nsupport;

	shape[0] = ntest;
	pred = tensorcreate(TensorFloat, 1, shape);
	if(pred == nil)
		return nil;

	xd = (float*)X->data;
	sd = (float*)km->support->data;
	ad = (float*)km->alpha->data;
	pd = (float*)pred->data;

	/* Create temporary vectors */
	vshape[0] = d;
	xi = tensorcreate(TensorFloat, 1, vshape);
	sj = tensorcreate(TensorFloat, 1, vshape);
	if(xi == nil || sj == nil){
		tensorfree(pred);
		tensorfree(xi);
		tensorfree(sj);
		return nil;
	}

	/* Compute predictions */
	for(i = 0; i < ntest; i++){
		/* Copy test point */
		for(l = 0; l < d; l++)
			((float*)xi->data)[l] = xd[i * d + l];

		float sum = 0.0;
		for(j = 0; j < nsup; j++){
			/* Copy support vector */
			for(l = 0; l < d; l++)
				((float*)sj->data)[l] = sd[j * d + l];

			float kval = tensorkerneleval(km->kernel, xi, sj);
			sum += ad[j] * kval;
		}

		pd[i] = sum + km->bias;
	}

	tensorfree(xi);
	tensorfree(sj);

	return pred;
}

/* ============================================================
 * Kernel PCA
 * ============================================================ */

/* Kernel PCA projection */
Tensor*
tensorkernelpca(TensorKernel *k, Tensor *X, int ncomponents)
{
	Tensor *gram, *centered, *result;
	float *gd, *cd;
	int n, i, j;
	float mean, rowmean, colmean, totalmean;

	if(k == nil || X == nil)
		return nil;

	n = X->shape[0];

	/* Compute Gram matrix */
	gram = tensorkernelgram(k, X);
	if(gram == nil)
		return nil;

	gd = (float*)gram->data;

	/* Center the kernel matrix */
	/* K_centered = K - 1_n K - K 1_n + 1_n K 1_n */

	/* Compute means */
	totalmean = 0.0;
	for(i = 0; i < n * n; i++)
		totalmean += gd[i];
	totalmean /= (n * n);

	/* Center */
	for(i = 0; i < n; i++){
		rowmean = 0.0;
		for(j = 0; j < n; j++)
			rowmean += gd[i * n + j];
		rowmean /= n;

		for(j = 0; j < n; j++){
			colmean = 0.0;
			int l;
			for(l = 0; l < n; l++)
				colmean += gd[l * n + j];
			colmean /= n;

			gd[i * n + j] = gd[i * n + j] - rowmean - colmean + totalmean;
		}
	}

	/* Eigendecomposition would go here */
	/* For simplicity, return centered Gram matrix */
	/* A full implementation would use power iteration or SVD */

	return gram;
}
