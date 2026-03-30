/*
 * Tensor Core Operations
 * Basic tensor creation, access, and arithmetic
 */

#include <u.h>
#include <libc.h>
#include <plan9cog/tensorlogic.h>

/* Calculate total elements from shape */
static ulong
calcnelems(int rank, int *shape)
{
	ulong n = 1;
	int i;

	for(i = 0; i < rank; i++)
		n *= shape[i];

	return n;
}

/* Calculate strides from shape */
static void
calcstrides(int rank, int *shape, int *strides)
{
	int i;

	strides[rank - 1] = 1;
	for(i = rank - 2; i >= 0; i--)
		strides[i] = strides[i + 1] * shape[i + 1];
}

/* Calculate linear index from multi-index */
static ulong
linearindex(Tensor *t, int *indices)
{
	ulong idx = 0;
	int i;

	for(i = 0; i < t->rank; i++)
		idx += indices[i] * t->strides[i];

	return idx;
}

/* Element size for type */
static int
elemsize(int type)
{
	switch(type){
	case TensorFloat:
		return sizeof(float);
	case TensorDouble:
		return sizeof(double);
	case TensorInt:
		return sizeof(int);
	case TensorBool:
		return sizeof(char);
	case TensorComplex:
		return 2 * sizeof(float);
	default:
		return sizeof(float);
	}
}

/* Create tensor with given type, rank, and shape */
Tensor*
tensorcreate(int type, int rank, int *shape)
{
	Tensor *t;
	int i;

	if(rank <= 0 || rank > TENSOR_MAX_RANK)
		return nil;

	t = mallocz(sizeof(Tensor), 1);
	if(t == nil)
		return nil;

	t->type = type;
	t->format = TensorDense;
	t->rank = rank;

	for(i = 0; i < rank; i++)
		t->shape[i] = shape[i];

	t->nelems = calcnelems(rank, shape);
	calcstrides(rank, shape, t->strides);

	t->data = mallocz(t->nelems * elemsize(type), 1);
	if(t->data == nil){
		free(t);
		return nil;
	}

	t->indices = nil;
	t->nnz = 0;
	t->core = nil;
	t->factors = nil;
	t->nfactors = 0;
	t->name = nil;

	return t;
}

/* Create tensor with variadic shape */
Tensor*
tensorcreatef(int rank, ...)
{
	va_list args;
	int shape[TENSOR_MAX_RANK];
	int i;

	va_start(args, rank);
	for(i = 0; i < rank; i++)
		shape[i] = va_arg(args, int);
	va_end(args);

	return tensorcreate(TensorFloat, rank, shape);
}

/* Create zero tensor */
Tensor*
tensorzeros(int rank, int *shape)
{
	return tensorcreate(TensorFloat, rank, shape);
}

/* Create tensor of ones */
Tensor*
tensorones(int rank, int *shape)
{
	Tensor *t = tensorcreate(TensorFloat, rank, shape);
	if(t == nil)
		return nil;

	tensorfill(t, 1.0);
	return t;
}

/* Create identity matrix */
Tensor*
tensoreye(int n)
{
	Tensor *t;
	int i;
	float *data;

	int shape[2] = {n, n};
	t = tensorcreate(TensorFloat, 2, shape);
	if(t == nil)
		return nil;

	data = (float*)t->data;
	for(i = 0; i < n; i++)
		data[i * n + i] = 1.0;

	return t;
}

/* Create random tensor */
Tensor*
tensorrand(int rank, int *shape)
{
	Tensor *t;
	float *data;
	ulong i;

	t = tensorcreate(TensorFloat, rank, shape);
	if(t == nil)
		return nil;

	data = (float*)t->data;
	for(i = 0; i < t->nelems; i++)
		data[i] = (float)frand();

	return t;
}

/* Create sparse tensor */
Tensor*
tensorsparse(int type, int rank, int *shape, int nnz)
{
	Tensor *t;

	t = tensorcreate(type, rank, shape);
	if(t == nil)
		return nil;

	t->format = TensorSparse;
	t->nnz = nnz;

	/* Allocate COO indices: nnz * rank */
	t->indices = mallocz(sizeof(int) * nnz * rank, 1);
	if(t->indices == nil){
		tensorfree(t);
		return nil;
	}

	/* Reallocate data for just nnz elements */
	free(t->data);
	t->data = mallocz(nnz * elemsize(type), 1);
	if(t->data == nil){
		free(t->indices);
		free(t);
		return nil;
	}

	return t;
}

/* Free tensor */
void
tensorfree(Tensor *t)
{
	int i;

	if(t == nil)
		return;

	lock(t);

	free(t->data);
	free(t->indices);
	free(t->name);

	if(t->core != nil)
		tensorfree(t->core);

	if(t->factors != nil){
		for(i = 0; i < t->nfactors; i++)
			tensorfree(t->factors[i]);
		free(t->factors);
	}

	unlock(t);
	free(t);
}

/* Copy tensor */
Tensor*
tensorcopy(Tensor *t)
{
	Tensor *copy;

	if(t == nil)
		return nil;

	copy = tensorcreate(t->type, t->rank, t->shape);
	if(copy == nil)
		return nil;

	tensorcopydata(copy, t);

	if(t->name != nil)
		copy->name = strdup(t->name);

	return copy;
}

/* Reshape tensor */
Tensor*
tensorreshape(Tensor *t, int rank, int *shape)
{
	Tensor *result;
	ulong newelems;

	if(t == nil)
		return nil;

	newelems = calcnelems(rank, shape);
	if(newelems != t->nelems)
		return nil;  /* Must have same number of elements */

	result = tensorcreate(t->type, rank, shape);
	if(result == nil)
		return nil;

	tensorcopydata(result, t);
	return result;
}

/* Fill tensor with value */
void
tensorfill(Tensor *t, float val)
{
	float *data;
	ulong i;

	if(t == nil || t->type != TensorFloat)
		return;

	data = (float*)t->data;
	for(i = 0; i < t->nelems; i++)
		data[i] = val;
}

/* Copy data between tensors */
void
tensorcopydata(Tensor *dst, Tensor *src)
{
	if(dst == nil || src == nil)
		return;

	if(dst->nelems != src->nelems)
		return;

	memcpy(dst->data, src->data, src->nelems * elemsize(src->type));
}

/* Get float element (variadic indices) */
float
tensorgetf(Tensor *t, ...)
{
	va_list args;
	int indices[TENSOR_MAX_RANK];
	int i;
	ulong idx;
	float *data;

	if(t == nil)
		return 0.0;

	va_start(args, t);
	for(i = 0; i < t->rank; i++)
		indices[i] = va_arg(args, int);
	va_end(args);

	idx = linearindex(t, indices);
	if(idx >= t->nelems)
		return 0.0;

	data = (float*)t->data;
	return data[idx];
}

/* Set float element (variadic indices) */
void
tensorsetf(Tensor *t, float val, ...)
{
	va_list args;
	int indices[TENSOR_MAX_RANK];
	int i;
	ulong idx;
	float *data;

	if(t == nil)
		return;

	va_start(args, val);
	for(i = 0; i < t->rank; i++)
		indices[i] = va_arg(args, int);
	va_end(args);

	idx = linearindex(t, indices);
	if(idx >= t->nelems)
		return;

	data = (float*)t->data;
	data[idx] = val;
}

/* Get double element */
double
tensorgetd(Tensor *t, ...)
{
	va_list args;
	int indices[TENSOR_MAX_RANK];
	int i;
	ulong idx;
	double *data;

	if(t == nil)
		return 0.0;

	va_start(args, t);
	for(i = 0; i < t->rank; i++)
		indices[i] = va_arg(args, int);
	va_end(args);

	idx = linearindex(t, indices);
	data = (double*)t->data;
	return data[idx];
}

/* Set double element */
void
tensorsetd(Tensor *t, double val, ...)
{
	va_list args;
	int indices[TENSOR_MAX_RANK];
	int i;
	ulong idx;
	double *data;

	if(t == nil)
		return;

	va_start(args, val);
	for(i = 0; i < t->rank; i++)
		indices[i] = va_arg(args, int);
	va_end(args);

	idx = linearindex(t, indices);
	data = (double*)t->data;
	data[idx] = val;
}

/* Get integer element */
int
tensorgeti(Tensor *t, ...)
{
	va_list args;
	int indices[TENSOR_MAX_RANK];
	int i;
	ulong idx;
	int *data;

	if(t == nil)
		return 0;

	va_start(args, t);
	for(i = 0; i < t->rank; i++)
		indices[i] = va_arg(args, int);
	va_end(args);

	idx = linearindex(t, indices);
	data = (int*)t->data;
	return data[idx];
}

/* Set integer element */
void
tensorseti(Tensor *t, int val, ...)
{
	va_list args;
	int indices[TENSOR_MAX_RANK];
	int i;
	ulong idx;
	int *data;

	if(t == nil)
		return;

	va_start(args, val);
	for(i = 0; i < t->rank; i++)
		indices[i] = va_arg(args, int);
	va_end(args);

	idx = linearindex(t, indices);
	data = (int*)t->data;
	data[idx] = val;
}

/* ============================================================
 * Tensor Arithmetic
 * ============================================================ */

/* Element-wise addition */
Tensor*
tensoradd(Tensor *a, Tensor *b)
{
	Tensor *result;
	float *da, *db, *dr;
	ulong i;

	if(a == nil || b == nil)
		return nil;

	if(a->nelems != b->nelems)
		return nil;

	result = tensorcopy(a);
	if(result == nil)
		return nil;

	da = (float*)a->data;
	db = (float*)b->data;
	dr = (float*)result->data;

	for(i = 0; i < result->nelems; i++)
		dr[i] = da[i] + db[i];

	return result;
}

/* Element-wise subtraction */
Tensor*
tensorsub(Tensor *a, Tensor *b)
{
	Tensor *result;
	float *da, *db, *dr;
	ulong i;

	if(a == nil || b == nil)
		return nil;

	if(a->nelems != b->nelems)
		return nil;

	result = tensorcopy(a);
	if(result == nil)
		return nil;

	da = (float*)a->data;
	db = (float*)b->data;
	dr = (float*)result->data;

	for(i = 0; i < result->nelems; i++)
		dr[i] = da[i] - db[i];

	return result;
}

/* Element-wise multiplication */
Tensor*
tensormul(Tensor *a, Tensor *b)
{
	Tensor *result;
	float *da, *db, *dr;
	ulong i;

	if(a == nil || b == nil)
		return nil;

	if(a->nelems != b->nelems)
		return nil;

	result = tensorcopy(a);
	if(result == nil)
		return nil;

	da = (float*)a->data;
	db = (float*)b->data;
	dr = (float*)result->data;

	for(i = 0; i < result->nelems; i++)
		dr[i] = da[i] * db[i];

	return result;
}

/* Element-wise division */
Tensor*
tensordiv(Tensor *a, Tensor *b)
{
	Tensor *result;
	float *da, *db, *dr;
	ulong i;

	if(a == nil || b == nil)
		return nil;

	if(a->nelems != b->nelems)
		return nil;

	result = tensorcopy(a);
	if(result == nil)
		return nil;

	da = (float*)a->data;
	db = (float*)b->data;
	dr = (float*)result->data;

	for(i = 0; i < result->nelems; i++){
		if(db[i] != 0.0)
			dr[i] = da[i] / db[i];
		else
			dr[i] = 0.0;
	}

	return result;
}

/* Scalar multiplication */
Tensor*
tensorscale(Tensor *t, float s)
{
	Tensor *result;
	float *data;
	ulong i;

	if(t == nil)
		return nil;

	result = tensorcopy(t);
	if(result == nil)
		return nil;

	data = (float*)result->data;
	for(i = 0; i < result->nelems; i++)
		data[i] *= s;

	return result;
}

/* Negation */
Tensor*
tensorneg(Tensor *t)
{
	return tensorscale(t, -1.0);
}

/* Matrix multiplication (2D tensors) */
Tensor*
tensormatmul(Tensor *a, Tensor *b)
{
	Tensor *result;
	float *da, *db, *dr;
	int m, k, n;
	int i, j, l;
	float sum;
	int shape[2];

	if(a == nil || b == nil)
		return nil;

	if(a->rank != 2 || b->rank != 2)
		return nil;

	if(a->shape[1] != b->shape[0])
		return nil;

	m = a->shape[0];
	k = a->shape[1];
	n = b->shape[1];

	shape[0] = m;
	shape[1] = n;
	result = tensorcreate(TensorFloat, 2, shape);
	if(result == nil)
		return nil;

	da = (float*)a->data;
	db = (float*)b->data;
	dr = (float*)result->data;

	for(i = 0; i < m; i++){
		for(j = 0; j < n; j++){
			sum = 0.0;
			for(l = 0; l < k; l++){
				sum += da[i * k + l] * db[l * n + j];
			}
			dr[i * n + j] = sum;
		}
	}

	return result;
}

/* Transpose (2D) */
Tensor*
tensortranspose(Tensor *t)
{
	Tensor *result;
	float *src, *dst;
	int i, j;
	int shape[2];

	if(t == nil || t->rank != 2)
		return nil;

	shape[0] = t->shape[1];
	shape[1] = t->shape[0];
	result = tensorcreate(t->type, 2, shape);
	if(result == nil)
		return nil;

	src = (float*)t->data;
	dst = (float*)result->data;

	for(i = 0; i < t->shape[0]; i++){
		for(j = 0; j < t->shape[1]; j++){
			dst[j * t->shape[0] + i] = src[i * t->shape[1] + j];
		}
	}

	return result;
}

/* Outer product */
Tensor*
tensorouter(Tensor *a, Tensor *b)
{
	Tensor *result;
	float *da, *db, *dr;
	int shape[TENSOR_MAX_RANK];
	int rank, i;
	ulong ai, bi;

	if(a == nil || b == nil)
		return nil;

	rank = a->rank + b->rank;
	if(rank > TENSOR_MAX_RANK)
		return nil;

	for(i = 0; i < a->rank; i++)
		shape[i] = a->shape[i];
	for(i = 0; i < b->rank; i++)
		shape[a->rank + i] = b->shape[i];

	result = tensorcreate(TensorFloat, rank, shape);
	if(result == nil)
		return nil;

	da = (float*)a->data;
	db = (float*)b->data;
	dr = (float*)result->data;

	for(ai = 0; ai < a->nelems; ai++){
		for(bi = 0; bi < b->nelems; bi++){
			dr[ai * b->nelems + bi] = da[ai] * db[bi];
		}
	}

	return result;
}

/* ============================================================
 * Tensor Reductions
 * ============================================================ */

/* Sum all elements */
float
tensorsum(Tensor *t)
{
	float *data;
	float sum = 0.0;
	ulong i;

	if(t == nil)
		return 0.0;

	data = (float*)t->data;
	for(i = 0; i < t->nelems; i++)
		sum += data[i];

	return sum;
}

/* Mean of all elements */
float
tensormean(Tensor *t)
{
	if(t == nil || t->nelems == 0)
		return 0.0;

	return tensorsum(t) / t->nelems;
}

/* Maximum element */
float
tensormax(Tensor *t)
{
	float *data;
	float max;
	ulong i;

	if(t == nil || t->nelems == 0)
		return 0.0;

	data = (float*)t->data;
	max = data[0];
	for(i = 1; i < t->nelems; i++){
		if(data[i] > max)
			max = data[i];
	}

	return max;
}

/* Minimum element */
float
tensormin(Tensor *t)
{
	float *data;
	float min;
	ulong i;

	if(t == nil || t->nelems == 0)
		return 0.0;

	data = (float*)t->data;
	min = data[0];
	for(i = 1; i < t->nelems; i++){
		if(data[i] < min)
			min = data[i];
	}

	return min;
}

/* L2 norm */
float
tensornorm(Tensor *t)
{
	float *data;
	float sum = 0.0;
	ulong i;

	if(t == nil)
		return 0.0;

	data = (float*)t->data;
	for(i = 0; i < t->nelems; i++)
		sum += data[i] * data[i];

	return sqrt(sum);
}

/* Frobenius norm (same as L2 for tensors) */
float
tensorfrobenius(Tensor *t)
{
	return tensornorm(t);
}
