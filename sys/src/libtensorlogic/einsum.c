/*
 * Einstein Summation (einsum) Implementation
 * Core tensor contraction operations
 *
 * Format: "indices_A,indices_B->indices_C"
 * Example: "ij,jk->ik" for matrix multiplication
 */

#include <u.h>
#include <libc.h>
#include <plan9cog/tensorlogic.h>

#define MAX_INDICES 26  /* a-z */

/* Index descriptor */
typedef struct IndexDesc IndexDesc;
struct IndexDesc {
	char name;		/* Index name (a-z) */
	int size;		/* Dimension size */
	int contracted;		/* Is summed over? */
	int tensors[8];		/* Which tensors use this index */
	int positions[8];	/* Position in each tensor */
	int ntensors;		/* Number of tensors using index */
};

/* Einsum descriptor */
typedef struct EinsumDesc EinsumDesc;
struct EinsumDesc {
	IndexDesc indices[MAX_INDICES];	/* Index descriptors */
	int nindices;			/* Number of unique indices */

	char *inputidx[8];		/* Input index strings */
	int ninputs;			/* Number of inputs */

	char *outputidx;		/* Output index string */
	int *outputshape;		/* Output shape */
	int outputrank;			/* Output rank */
};

/* Parse index character to index number */
static int
indexnum(char c)
{
	if(c >= 'a' && c <= 'z')
		return c - 'a';
	if(c >= 'A' && c <= 'Z')
		return c - 'A';
	return -1;
}

/* Find or create index descriptor */
static IndexDesc*
findindex(EinsumDesc *desc, char name)
{
	int i;

	for(i = 0; i < desc->nindices; i++){
		if(desc->indices[i].name == name)
			return &desc->indices[i];
	}

	/* Create new index */
	if(desc->nindices >= MAX_INDICES)
		return nil;

	desc->indices[desc->nindices].name = name;
	desc->indices[desc->nindices].size = 0;
	desc->indices[desc->nindices].contracted = 1;  /* Default: contracted */
	desc->indices[desc->nindices].ntensors = 0;

	return &desc->indices[desc->nindices++];
}

/* Parse einsum equation */
static EinsumDesc*
parseeinsum(char *equation, Tensor **tensors, int ntensors)
{
	EinsumDesc *desc;
	char *arrow, *comma, *p;
	int tidx, pidx;
	IndexDesc *idx;
	char idxbuf[64];
	int i, j;

	desc = mallocz(sizeof(EinsumDesc), 1);
	if(desc == nil)
		return nil;

	desc->ninputs = 0;
	desc->nindices = 0;

	/* Find arrow separator */
	arrow = strstr(equation, "->");
	if(arrow == nil){
		free(desc);
		return nil;
	}

	/* Parse input indices */
	p = equation;
	tidx = 0;
	while(p < arrow && tidx < ntensors){
		/* Find comma or arrow */
		comma = strchr(p, ',');
		if(comma == nil || comma > arrow)
			comma = arrow;

		/* Extract index string */
		memset(idxbuf, 0, sizeof(idxbuf));
		strncpy(idxbuf, p, comma - p);
		desc->inputidx[tidx] = strdup(idxbuf);

		/* Register indices */
		pidx = 0;
		for(i = 0; idxbuf[i] != '\0'; i++){
			if(indexnum(idxbuf[i]) < 0)
				continue;

			idx = findindex(desc, idxbuf[i]);
			if(idx == nil)
				continue;

			/* Record tensor and position */
			idx->tensors[idx->ntensors] = tidx;
			idx->positions[idx->ntensors] = pidx;
			idx->ntensors++;

			/* Get size from tensor */
			if(tensors != nil && tensors[tidx] != nil){
				if(idx->size == 0)
					idx->size = tensors[tidx]->shape[pidx];
			}

			pidx++;
		}

		tidx++;
		desc->ninputs++;

		if(comma == arrow)
			break;
		p = comma + 1;
	}

	/* Parse output indices */
	desc->outputidx = strdup(arrow + 2);

	/* Mark output indices as not contracted */
	for(i = 0; desc->outputidx[i] != '\0'; i++){
		if(indexnum(desc->outputidx[i]) < 0)
			continue;

		idx = findindex(desc, desc->outputidx[i]);
		if(idx != nil)
			idx->contracted = 0;
	}

	/* Calculate output shape */
	desc->outputrank = strlen(desc->outputidx);
	desc->outputshape = mallocz(sizeof(int) * desc->outputrank, 1);

	for(i = 0; i < desc->outputrank; i++){
		idx = findindex(desc, desc->outputidx[i]);
		if(idx != nil)
			desc->outputshape[i] = idx->size;
		else
			desc->outputshape[i] = 1;
	}

	return desc;
}

/* Free einsum descriptor */
static void
freedesc(EinsumDesc *desc)
{
	int i;

	if(desc == nil)
		return;

	for(i = 0; i < desc->ninputs; i++)
		free(desc->inputidx[i]);
	free(desc->outputidx);
	free(desc->outputshape);
	free(desc);
}

/* Convert multi-index to linear index for einsum */
static ulong
einsumlinear(int *indices, int *shape, int rank)
{
	ulong idx = 0;
	ulong stride = 1;
	int i;

	for(i = rank - 1; i >= 0; i--){
		idx += indices[i] * stride;
		stride *= shape[i];
	}

	return idx;
}

/* Increment multi-index */
static int
incrementindex(int *indices, int *shape, int rank)
{
	int i;

	for(i = rank - 1; i >= 0; i--){
		indices[i]++;
		if(indices[i] < shape[i])
			return 1;
		indices[i] = 0;
	}

	return 0;  /* Overflow */
}

/* General einsum for two tensors */
Tensor*
tensoreinsum(char *equation, Tensor *a, Tensor *b)
{
	EinsumDesc *desc;
	Tensor *result;
	Tensor *tensors[2] = {a, b};
	float *da, *db, *dr;
	int outidx[TENSOR_MAX_RANK];
	int sumidx[TENSOR_MAX_RANK];
	int aidx[TENSOR_MAX_RANK];
	int bidx[TENSOR_MAX_RANK];
	int sumshape[TENSOR_MAX_RANK];
	int nsumidx;
	ulong aidxl, bidxl, ridxl;
	float sum, va, vb;
	int i, j;
	IndexDesc *idx;

	if(equation == nil || a == nil || b == nil)
		return nil;

	/* Parse equation */
	desc = parseeinsum(equation, tensors, 2);
	if(desc == nil)
		return nil;

	/* Create output tensor */
	result = tensorcreate(TensorFloat, desc->outputrank, desc->outputshape);
	if(result == nil){
		freedesc(desc);
		return nil;
	}

	da = (float*)a->data;
	db = (float*)b->data;
	dr = (float*)result->data;

	/* Build summation index shape */
	nsumidx = 0;
	for(i = 0; i < desc->nindices; i++){
		if(desc->indices[i].contracted){
			sumshape[nsumidx++] = desc->indices[i].size;
		}
	}

	/* Initialize output indices */
	memset(outidx, 0, sizeof(outidx));

	/* Iterate over all output positions */
	do {
		sum = 0.0;

		/* Initialize summation indices */
		memset(sumidx, 0, sizeof(sumidx));

		/* Iterate over summation indices */
		do {
			/* Build tensor A indices */
			for(i = 0; desc->inputidx[0][i] != '\0'; i++){
				char c = desc->inputidx[0][i];
				idx = findindex(desc, c);
				if(idx == nil)
					continue;

				if(idx->contracted){
					/* Find position in sumidx */
					int pos = 0;
					for(j = 0; j < desc->nindices; j++){
						if(desc->indices[j].contracted){
							if(desc->indices[j].name == c){
								aidx[i] = sumidx[pos];
								break;
							}
							pos++;
						}
					}
				} else {
					/* Find position in outidx */
					for(j = 0; desc->outputidx[j] != '\0'; j++){
						if(desc->outputidx[j] == c){
							aidx[i] = outidx[j];
							break;
						}
					}
				}
			}

			/* Build tensor B indices */
			for(i = 0; desc->inputidx[1][i] != '\0'; i++){
				char c = desc->inputidx[1][i];
				idx = findindex(desc, c);
				if(idx == nil)
					continue;

				if(idx->contracted){
					/* Find position in sumidx */
					int pos = 0;
					for(j = 0; j < desc->nindices; j++){
						if(desc->indices[j].contracted){
							if(desc->indices[j].name == c){
								bidx[i] = sumidx[pos];
								break;
							}
							pos++;
						}
					}
				} else {
					/* Find position in outidx */
					for(j = 0; desc->outputidx[j] != '\0'; j++){
						if(desc->outputidx[j] == c){
							bidx[i] = outidx[j];
							break;
						}
					}
				}
			}

			/* Get values and multiply */
			aidxl = einsumlinear(aidx, a->shape, a->rank);
			bidxl = einsumlinear(bidx, b->shape, b->rank);

			if(aidxl < a->nelems && bidxl < b->nelems){
				va = da[aidxl];
				vb = db[bidxl];
				sum += va * vb;
			}

		} while(nsumidx > 0 && incrementindex(sumidx, sumshape, nsumidx));

		/* Store result */
		ridxl = einsumlinear(outidx, desc->outputshape, desc->outputrank);
		if(ridxl < result->nelems)
			dr[ridxl] = sum;

	} while(incrementindex(outidx, desc->outputshape, desc->outputrank));

	freedesc(desc);
	return result;
}

/* Einsum for three tensors */
Tensor*
tensoreinsum3(char *equation, Tensor *a, Tensor *b, Tensor *c)
{
	/* For three tensors, compute (A op B) op C */
	Tensor *ab, *result;
	char *arrow, *comma1, *comma2;
	char eq1[128], eq2[128];

	if(equation == nil || a == nil || b == nil || c == nil)
		return nil;

	/* Split into two operations */
	arrow = strstr(equation, "->");
	comma1 = strchr(equation, ',');
	comma2 = strchr(comma1 + 1, ',');

	if(comma2 == nil){
		/* Only two inputs specified, just use regular einsum */
		return tensoreinsum(equation, a, b);
	}

	/* First operation: A,B -> temp */
	memset(eq1, 0, sizeof(eq1));
	strncpy(eq1, equation, comma2 - equation);

	/* Create temp indices for intermediate result */
	strcat(eq1, "->");
	/* Use unique intermediate indices */
	strcat(eq1, "xy");

	ab = tensoreinsum(eq1, a, b);
	if(ab == nil)
		return nil;

	/* Second operation: temp,C -> output */
	snprint(eq2, sizeof(eq2), "xy,%s%s", comma2 + 1, arrow);

	result = tensoreinsum(eq2, ab, c);
	tensorfree(ab);

	return result;
}

/* Variadic einsum */
Tensor*
tensoreinsumv(char *equation, Tensor **tensors, int n)
{
	Tensor *result, *temp;
	int i;

	if(n == 0 || tensors == nil)
		return nil;

	if(n == 1)
		return tensorcopy(tensors[0]);

	if(n == 2)
		return tensoreinsum(equation, tensors[0], tensors[1]);

	/* For more tensors, chain the operations */
	result = tensoreinsum("ij,jk->ik", tensors[0], tensors[1]);
	for(i = 2; i < n; i++){
		temp = tensoreinsum("ij,jk->ik", result, tensors[i]);
		tensorfree(result);
		result = temp;
		if(result == nil)
			return nil;
	}

	return result;
}

/* Tensor contraction on specified axes */
Tensor*
tensorcontract(Tensor *a, Tensor *b, int *axes_a, int *axes_b, int naxes)
{
	char equation[128];
	char aidx[32], bidx[32], oidx[32];
	int i, j, pos;
	int contracted[MAX_INDICES];
	char c;

	if(a == nil || b == nil)
		return nil;

	memset(contracted, 0, sizeof(contracted));

	/* Build index strings */
	pos = 0;
	for(i = 0; i < a->rank; i++){
		c = 'a' + i;
		aidx[pos++] = c;

		/* Check if this axis is contracted */
		for(j = 0; j < naxes; j++){
			if(axes_a[j] == i){
				contracted[i] = 1;
				break;
			}
		}
	}
	aidx[pos] = '\0';

	pos = 0;
	for(i = 0; i < b->rank; i++){
		c = 'a' + a->rank + i;

		/* If contracted, use same index as corresponding A axis */
		for(j = 0; j < naxes; j++){
			if(axes_b[j] == i){
				c = 'a' + axes_a[j];
				break;
			}
		}

		bidx[pos++] = c;
	}
	bidx[pos] = '\0';

	/* Build output indices (non-contracted) */
	pos = 0;
	for(i = 0; i < a->rank; i++){
		if(!contracted[i])
			oidx[pos++] = 'a' + i;
	}
	for(i = 0; i < b->rank; i++){
		int iscontracted = 0;
		for(j = 0; j < naxes; j++){
			if(axes_b[j] == i){
				iscontracted = 1;
				break;
			}
		}
		if(!iscontracted)
			oidx[pos++] = 'a' + a->rank + i;
	}
	oidx[pos] = '\0';

	snprint(equation, sizeof(equation), "%s,%s->%s", aidx, bidx, oidx);
	return tensoreinsum(equation, a, b);
}

/* Trace (sum of diagonal elements) */
Tensor*
tensortrace(Tensor *t)
{
	Tensor *result;
	float *data;
	float sum = 0.0;
	int i, n;
	int shape[1] = {1};

	if(t == nil || t->rank != 2)
		return nil;

	if(t->shape[0] != t->shape[1])
		return nil;

	n = t->shape[0];
	data = (float*)t->data;

	for(i = 0; i < n; i++)
		sum += data[i * n + i];

	result = tensorcreate(TensorFloat, 1, shape);
	if(result != nil){
		((float*)result->data)[0] = sum;
	}

	return result;
}

/* Extract diagonal */
Tensor*
tensordiag(Tensor *t)
{
	Tensor *result;
	float *src, *dst;
	int i, n;
	int shape[1];

	if(t == nil || t->rank != 2)
		return nil;

	n = (t->shape[0] < t->shape[1]) ? t->shape[0] : t->shape[1];
	shape[0] = n;

	result = tensorcreate(TensorFloat, 1, shape);
	if(result == nil)
		return nil;

	src = (float*)t->data;
	dst = (float*)result->data;

	for(i = 0; i < n; i++)
		dst[i] = src[i * t->shape[1] + i];

	return result;
}
