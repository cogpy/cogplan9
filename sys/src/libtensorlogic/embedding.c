/*
 * Embedding Space Reasoning
 * Sound reasoning with learned embeddings
 *
 * Objects stored as rows in embedding matrix
 * Relations become tensor products of embeddings
 * Enables analogical reasoning with similarity
 */

#include <u.h>
#include <libc.h>
#include <plan9cog/tensorlogic.h>

/* ============================================================
 * Tensor Embeddings
 * ============================================================ */

/* Create embedding matrix */
TensorEmbedding*
tensorembcreate(int nobjects, int ndims)
{
	TensorEmbedding *emb;
	int shape[2];

	emb = mallocz(sizeof(TensorEmbedding), 1);
	if(emb == nil)
		return nil;

	emb->nobjects = nobjects;
	emb->ndims = ndims;

	shape[0] = nobjects;
	shape[1] = ndims;
	emb->matrix = tensorcreate(TensorFloat, 2, shape);
	if(emb->matrix == nil){
		free(emb);
		return nil;
	}

	emb->names = mallocz(sizeof(char*) * nobjects, 1);
	if(emb->names == nil){
		tensorfree(emb->matrix);
		free(emb);
		return nil;
	}

	/* Initialize with random vectors (normalized) */
	float *data = (float*)emb->matrix->data;
	int i, j;
	float norm;

	for(i = 0; i < nobjects; i++){
		norm = 0.0;
		for(j = 0; j < ndims; j++){
			data[i * ndims + j] = (float)(frand() - 0.5) * 2.0;
			norm += data[i * ndims + j] * data[i * ndims + j];
		}
		norm = sqrt(norm);
		if(norm > 0.0){
			for(j = 0; j < ndims; j++)
				data[i * ndims + j] /= norm;
		}
	}

	return emb;
}

/* Free embedding */
void
tensorembfree(TensorEmbedding *emb)
{
	int i;

	if(emb == nil)
		return;

	for(i = 0; i < emb->nobjects; i++)
		free(emb->names[i]);
	free(emb->names);
	tensorfree(emb->matrix);
	free(emb);
}

/* Set embedding vector for object */
void
tensorembset(TensorEmbedding *emb, int idx, Tensor *vec)
{
	float *dst, *src;
	int j;

	if(emb == nil || vec == nil)
		return;

	if(idx < 0 || idx >= emb->nobjects)
		return;

	if(vec->nelems != (ulong)emb->ndims)
		return;

	dst = (float*)emb->matrix->data;
	src = (float*)vec->data;

	for(j = 0; j < emb->ndims; j++)
		dst[idx * emb->ndims + j] = src[j];
}

/* Set name for object */
void
tensorembsetname(TensorEmbedding *emb, int idx, char *name)
{
	if(emb == nil)
		return;

	if(idx < 0 || idx >= emb->nobjects)
		return;

	free(emb->names[idx]);
	emb->names[idx] = strdup(name);
}

/* Get embedding vector for object */
Tensor*
tensorembget(TensorEmbedding *emb, int idx)
{
	Tensor *vec;
	float *src, *dst;
	int shape[1];
	int j;

	if(emb == nil)
		return nil;

	if(idx < 0 || idx >= emb->nobjects)
		return nil;

	shape[0] = emb->ndims;
	vec = tensorcreate(TensorFloat, 1, shape);
	if(vec == nil)
		return nil;

	src = (float*)emb->matrix->data;
	dst = (float*)vec->data;

	for(j = 0; j < emb->ndims; j++)
		dst[j] = src[idx * emb->ndims + j];

	return vec;
}

/* Find object by name */
int
tensorembfind(TensorEmbedding *emb, char *name)
{
	int i;

	if(emb == nil || name == nil)
		return -1;

	for(i = 0; i < emb->nobjects; i++){
		if(emb->names[i] != nil && strcmp(emb->names[i], name) == 0)
			return i;
	}

	return -1;
}

/* Compute similarity (Gram) matrix */
Tensor*
tensorembsim(TensorEmbedding *emb)
{
	Tensor *result;
	Tensor *trans;
	int shape[2];
	float *data, *trans_data, *result_data;
	int i, j, k;
	float dot;

	if(emb == nil)
		return nil;

	/* Sim = Emb @ Emb^T */
	shape[0] = emb->nobjects;
	shape[1] = emb->nobjects;
	result = tensorcreate(TensorFloat, 2, shape);
	if(result == nil)
		return nil;

	data = (float*)emb->matrix->data;
	result_data = (float*)result->data;

	for(i = 0; i < emb->nobjects; i++){
		for(j = 0; j < emb->nobjects; j++){
			dot = 0.0;
			for(k = 0; k < emb->ndims; k++){
				dot += data[i * emb->ndims + k] * data[j * emb->ndims + k];
			}
			result_data[i * emb->nobjects + j] = dot;
		}
	}

	return result;
}

/* Compute distance between two objects */
float
tensorembdist(TensorEmbedding *emb, int i, int j)
{
	float *data;
	float dist = 0.0;
	int k;

	if(emb == nil)
		return 0.0;

	if(i < 0 || i >= emb->nobjects || j < 0 || j >= emb->nobjects)
		return 0.0;

	data = (float*)emb->matrix->data;

	for(k = 0; k < emb->ndims; k++){
		float diff = data[i * emb->ndims + k] - data[j * emb->ndims + k];
		dist += diff * diff;
	}

	return sqrt(dist);
}

/* ============================================================
 * Embedded Relations
 * ============================================================ */

/* Create embedded relation */
TensorEmbRel*
tensorembrelcreate(char *name, int arity, TensorEmbedding *emb)
{
	TensorEmbRel *rel;
	int shape[TENSOR_MAX_RANK];
	int i;

	if(emb == nil || arity < 1)
		return nil;

	rel = mallocz(sizeof(TensorEmbRel), 1);
	if(rel == nil)
		return nil;

	rel->name = strdup(name);
	rel->arity = arity;
	rel->embedding = emb;

	/* Embedded relation tensor has shape [d, d, ...] for arity dimensions */
	for(i = 0; i < arity; i++)
		shape[i] = emb->ndims;

	rel->tensor = tensorcreate(TensorFloat, arity, shape);
	if(rel->tensor == nil){
		free(rel->name);
		free(rel);
		return nil;
	}

	return rel;
}

/* Free embedded relation */
void
tensorembrelfree(TensorEmbRel *rel)
{
	if(rel == nil)
		return;

	free(rel->name);
	tensorfree(rel->tensor);
	free(rel);
}

/* Assert tuple in embedded relation */
void
tensorembrelassert(TensorEmbRel *rel, int *indices)
{
	float *emb_data, *rel_data;
	int i, j, k;
	ulong idx;
	int strides[TENSOR_MAX_RANK];
	float *vecs[TENSOR_MAX_RANK];
	int d;

	if(rel == nil || indices == nil)
		return;

	emb_data = (float*)rel->embedding->matrix->data;
	rel_data = (float*)rel->tensor->data;
	d = rel->embedding->ndims;

	/* Get embedding vectors for each argument */
	for(i = 0; i < rel->arity; i++){
		if(indices[i] < 0 || indices[i] >= rel->embedding->nobjects)
			return;
		vecs[i] = &emb_data[indices[i] * d];
	}

	/* Add outer product to relation tensor */
	/* For binary: EmbR[i,j] += v1[i] * v2[j] */
	if(rel->arity == 2){
		for(i = 0; i < d; i++){
			for(j = 0; j < d; j++){
				rel_data[i * d + j] += vecs[0][i] * vecs[1][j];
			}
		}
	} else if(rel->arity == 1){
		for(i = 0; i < d; i++){
			rel_data[i] += vecs[0][i];
		}
	}
	/* Higher arities would need general tensor product */
}

/* Query embedded relation */
float
tensorembrelquery(TensorEmbRel *rel, int *indices)
{
	float *emb_data, *rel_data;
	int i, j;
	float *vecs[TENSOR_MAX_RANK];
	int d;
	float result = 0.0;

	if(rel == nil || indices == nil)
		return 0.0;

	emb_data = (float*)rel->embedding->matrix->data;
	rel_data = (float*)rel->tensor->data;
	d = rel->embedding->ndims;

	/* Get embedding vectors */
	for(i = 0; i < rel->arity; i++){
		if(indices[i] < 0 || indices[i] >= rel->embedding->nobjects)
			return 0.0;
		vecs[i] = &emb_data[indices[i] * d];
	}

	/* Compute dot product: D[A,B] = EmbR[i,j] * v1[i] * v2[j] */
	if(rel->arity == 2){
		for(i = 0; i < d; i++){
			for(j = 0; j < d; j++){
				result += rel_data[i * d + j] * vecs[0][i] * vecs[1][j];
			}
		}
	} else if(rel->arity == 1){
		for(i = 0; i < d; i++){
			result += rel_data[i] * vecs[0][i];
		}
	}

	return result;
}

/* ============================================================
 * Analogical Reasoning
 * ============================================================ */

/*
 * Analogical inference with temperature control
 *
 * T -> 0: deductive reasoning (exact matches only)
 * T > 0: analogical reasoning (similar objects contribute)
 */
Tensor*
tensoranalogical(TensorEmbedding *emb, TensorEmbRel *rel, int *query, float temperature)
{
	Tensor *result, *sim;
	float *sim_data, *result_data;
	int i, j;
	float weight, total;

	if(emb == nil || rel == nil || query == nil)
		return nil;

	/* Get similarity matrix */
	sim = tensorembsim(emb);
	if(sim == nil)
		return nil;

	sim_data = (float*)sim->data;

	/* Apply temperature to similarities */
	if(temperature > 0.0){
		for(i = 0; i < emb->nobjects * emb->nobjects; i++){
			sim_data[i] = exp(sim_data[i] / temperature);
		}

		/* Normalize rows */
		for(i = 0; i < emb->nobjects; i++){
			total = 0.0;
			for(j = 0; j < emb->nobjects; j++)
				total += sim_data[i * emb->nobjects + j];
			if(total > 0.0){
				for(j = 0; j < emb->nobjects; j++)
					sim_data[i * emb->nobjects + j] /= total;
			}
		}
	} else {
		/* T -> 0: identity matrix (exact matches only) */
		for(i = 0; i < emb->nobjects; i++){
			for(j = 0; j < emb->nobjects; j++){
				sim_data[i * emb->nobjects + j] = (i == j) ? 1.0 : 0.0;
			}
		}
	}

	/* Compute weighted query results */
	int shape[1] = {emb->nobjects};
	result = tensorcreate(TensorFloat, 1, shape);
	if(result == nil){
		tensorfree(sim);
		return nil;
	}

	result_data = (float*)result->data;

	/* For each possible result object */
	for(i = 0; i < emb->nobjects; i++){
		float score = 0.0;

		/* Weight by similarity to query */
		for(j = 0; j < emb->nobjects; j++){
			weight = sim_data[query[0] * emb->nobjects + j];

			/* Get relation score for this similar object */
			int test_query[2] = {j, query[1]};
			float rel_score = tensorembrelquery(rel, test_query);

			score += weight * rel_score;
		}

		result_data[i] = score;
	}

	tensorfree(sim);
	return result;
}

/* ============================================================
 * Embedding Learning
 * ============================================================ */

/* Update embeddings to minimize loss */
void
tensorembtrain(TensorEmbedding *emb, TensorEmbRel *rel, int *pos_examples, int npos,
	       int *neg_examples, int nneg, float lr)
{
	float *emb_data;
	int i, j;
	float pos_score, neg_score, margin, grad;
	int d;

	if(emb == nil || rel == nil)
		return;

	emb_data = (float*)emb->matrix->data;
	d = emb->ndims;

	/* Margin-based ranking loss */
	margin = 1.0;

	/* For each positive example */
	for(i = 0; i < npos; i++){
		pos_score = tensorembrelquery(rel, &pos_examples[i * rel->arity]);

		/* Sample negative examples */
		for(j = 0; j < nneg; j++){
			neg_score = tensorembrelquery(rel, &neg_examples[j * rel->arity]);

			/* Hinge loss: max(0, margin - pos_score + neg_score) */
			if(margin - pos_score + neg_score > 0.0){
				/* Gradient update */
				/* Increase positive score, decrease negative */
				/* Simplified: adjust embeddings toward/away */
				int *pos = &pos_examples[i * rel->arity];
				int *neg = &neg_examples[j * rel->arity];
				int k;

				for(k = 0; k < d; k++){
					/* Push positive embeddings together */
					emb_data[pos[0] * d + k] += lr * emb_data[pos[1] * d + k];
					/* Push negative embeddings apart */
					emb_data[neg[0] * d + k] -= lr * emb_data[neg[1] * d + k];
				}
			}
		}
	}

	/* Renormalize embeddings */
	for(i = 0; i < emb->nobjects; i++){
		float norm = 0.0;
		for(j = 0; j < d; j++){
			norm += emb_data[i * d + j] * emb_data[i * d + j];
		}
		norm = sqrt(norm);
		if(norm > 0.0){
			for(j = 0; j < d; j++)
				emb_data[i * d + j] /= norm;
		}
	}
}
