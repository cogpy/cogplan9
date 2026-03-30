/*
 * Graphical Models as Tensor Networks
 * Factor graphs with tensor-based inference
 */

#include <u.h>
#include <libc.h>
#include <plan9cog/tensorlogic.h>

/* ============================================================
 * Factor Operations
 * ============================================================ */

/* Create factor */
TensorFactor*
tensorfactorcreate(char *name, int *vars, int nvars, int *sizes)
{
	TensorFactor *f;
	int i;
	int shape[TENSOR_MAX_RANK];

	if(nvars <= 0 || nvars > TENSOR_MAX_RANK)
		return nil;

	f = mallocz(sizeof(TensorFactor), 1);
	if(f == nil)
		return nil;

	f->name = strdup(name);
	f->nvars = nvars;

	f->vars = mallocz(sizeof(int) * nvars, 1);
	if(f->vars == nil){
		free(f->name);
		free(f);
		return nil;
	}

	for(i = 0; i < nvars; i++){
		f->vars[i] = vars[i];
		shape[i] = sizes[i];
	}

	f->values = tensorcreate(TensorFloat, nvars, shape);
	if(f->values == nil){
		free(f->vars);
		free(f->name);
		free(f);
		return nil;
	}

	/* Initialize to uniform */
	tensorfill(f->values, 1.0);

	return f;
}

/* Free factor */
void
tensorfactorfree(TensorFactor *f)
{
	if(f == nil)
		return;

	free(f->name);
	free(f->vars);
	tensorfree(f->values);
	free(f);
}

/* Set factor value */
void
tensorfactorset(TensorFactor *f, int *indices, float val)
{
	ulong idx = 0;
	int stride = 1;
	int i;
	float *data;

	if(f == nil || f->values == nil)
		return;

	/* Calculate linear index */
	for(i = f->nvars - 1; i >= 0; i--){
		idx += indices[i] * stride;
		stride *= f->values->shape[i];
	}

	if(idx < f->values->nelems){
		data = (float*)f->values->data;
		data[idx] = val;
	}
}

/* Get factor value */
float
tensorfactorget(TensorFactor *f, int *indices)
{
	ulong idx = 0;
	int stride = 1;
	int i;
	float *data;

	if(f == nil || f->values == nil)
		return 0.0;

	/* Calculate linear index */
	for(i = f->nvars - 1; i >= 0; i--){
		idx += indices[i] * stride;
		stride *= f->values->shape[i];
	}

	if(idx < f->values->nelems){
		data = (float*)f->values->data;
		return data[idx];
	}

	return 0.0;
}

/* ============================================================
 * Factor Graph Operations
 * ============================================================ */

/* Initialize factor graph */
TensorFactorGraph*
tensorfginit(int nvars, int *sizes)
{
	TensorFactorGraph *fg;
	int i;

	fg = mallocz(sizeof(TensorFactorGraph), 1);
	if(fg == nil)
		return nil;

	fg->nvars = nvars;
	fg->nfactors = 0;
	fg->factors = nil;

	fg->varsizes = mallocz(sizeof(int) * nvars, 1);
	fg->varnames = mallocz(sizeof(char*) * nvars, 1);

	if(fg->varsizes == nil || fg->varnames == nil){
		free(fg->varsizes);
		free(fg->varnames);
		free(fg);
		return nil;
	}

	for(i = 0; i < nvars; i++){
		fg->varsizes[i] = sizes[i];
		fg->varnames[i] = nil;
	}

	return fg;
}

/* Free factor graph */
void
tensorfgfree(TensorFactorGraph *fg)
{
	int i;

	if(fg == nil)
		return;

	for(i = 0; i < fg->nfactors; i++)
		tensorfactorfree(fg->factors[i]);
	free(fg->factors);

	for(i = 0; i < fg->nvars; i++)
		free(fg->varnames[i]);
	free(fg->varnames);
	free(fg->varsizes);
	free(fg);
}

/* Add factor to graph */
void
tensorfgaddfactor(TensorFactorGraph *fg, TensorFactor *f)
{
	TensorFactor **newfactors;

	if(fg == nil || f == nil)
		return;

	newfactors = realloc(fg->factors, sizeof(TensorFactor*) * (fg->nfactors + 1));
	if(newfactors == nil)
		return;

	fg->factors = newfactors;
	fg->factors[fg->nfactors++] = f;
}

/* ============================================================
 * Exact Inference
 * ============================================================ */

/* Compute partition function (sum over all configurations) */
float
tensorfgpartition(TensorFactorGraph *fg)
{
	float Z = 0.0;
	int *config;
	int i, j;
	float prod;
	int overflow;

	if(fg == nil || fg->nvars == 0)
		return 1.0;

	config = mallocz(sizeof(int) * fg->nvars, 1);
	if(config == nil)
		return 0.0;

	/* Enumerate all configurations */
	do {
		/* Compute product of all factors */
		prod = 1.0;
		for(i = 0; i < fg->nfactors; i++){
			TensorFactor *f = fg->factors[i];
			int *fconfig = mallocz(sizeof(int) * f->nvars, 1);
			if(fconfig == nil)
				continue;

			/* Extract relevant config values */
			for(j = 0; j < f->nvars; j++)
				fconfig[j] = config[f->vars[j]];

			prod *= tensorfactorget(f, fconfig);
			free(fconfig);
		}

		Z += prod;

		/* Increment configuration */
		overflow = 0;
		for(i = fg->nvars - 1; i >= 0; i--){
			config[i]++;
			if(config[i] < fg->varsizes[i])
				break;
			config[i] = 0;
			if(i == 0)
				overflow = 1;
		}

	} while(!overflow);

	free(config);
	return Z;
}

/* Compute marginal for a variable */
Tensor*
tensorfgmarginal(TensorFactorGraph *fg, int var)
{
	Tensor *marginal;
	int *config;
	int i, j;
	float prod, Z;
	int overflow;
	int shape[1];
	float *mdata;

	if(fg == nil || var < 0 || var >= fg->nvars)
		return nil;

	shape[0] = fg->varsizes[var];
	marginal = tensorcreate(TensorFloat, 1, shape);
	if(marginal == nil)
		return nil;

	mdata = (float*)marginal->data;

	config = mallocz(sizeof(int) * fg->nvars, 1);
	if(config == nil){
		tensorfree(marginal);
		return nil;
	}

	/* Enumerate all configurations */
	do {
		/* Compute product of all factors */
		prod = 1.0;
		for(i = 0; i < fg->nfactors; i++){
			TensorFactor *f = fg->factors[i];
			int *fconfig = mallocz(sizeof(int) * f->nvars, 1);
			if(fconfig == nil)
				continue;

			for(j = 0; j < f->nvars; j++)
				fconfig[j] = config[f->vars[j]];

			prod *= tensorfactorget(f, fconfig);
			free(fconfig);
		}

		/* Add to marginal for current variable value */
		mdata[config[var]] += prod;

		/* Increment configuration */
		overflow = 0;
		for(i = fg->nvars - 1; i >= 0; i--){
			config[i]++;
			if(config[i] < fg->varsizes[i])
				break;
			config[i] = 0;
			if(i == 0)
				overflow = 1;
		}

	} while(!overflow);

	free(config);

	/* Normalize */
	Z = tensorsum(marginal);
	if(Z > 0.0){
		for(i = 0; i < fg->varsizes[var]; i++)
			mdata[i] /= Z;
	}

	return marginal;
}

/* Compute conditional probability */
float
tensorfgprob(TensorFactorGraph *fg, int *evidence, int nevidence,
	     int *query, int nquery)
{
	float prob_joint = 0.0;
	float prob_evidence = 0.0;
	int *config;
	int i, j, k;
	float prod;
	int overflow;
	int matches_evidence, matches_query;

	if(fg == nil)
		return 0.0;

	config = mallocz(sizeof(int) * fg->nvars, 1);
	if(config == nil)
		return 0.0;

	/* Enumerate all configurations */
	do {
		/* Check if matches evidence */
		matches_evidence = 1;
		for(i = 0; i < nevidence; i += 2){
			if(config[evidence[i]] != evidence[i + 1]){
				matches_evidence = 0;
				break;
			}
		}

		if(!matches_evidence){
			/* Skip non-matching configurations */
			goto next;
		}

		/* Compute product of all factors */
		prod = 1.0;
		for(i = 0; i < fg->nfactors; i++){
			TensorFactor *f = fg->factors[i];
			int *fconfig = mallocz(sizeof(int) * f->nvars, 1);
			if(fconfig == nil)
				continue;

			for(j = 0; j < f->nvars; j++)
				fconfig[j] = config[f->vars[j]];

			prod *= tensorfactorget(f, fconfig);
			free(fconfig);
		}

		prob_evidence += prod;

		/* Check if also matches query */
		matches_query = 1;
		for(i = 0; i < nquery; i += 2){
			if(config[query[i]] != query[i + 1]){
				matches_query = 0;
				break;
			}
		}

		if(matches_query)
			prob_joint += prod;

next:
		/* Increment configuration */
		overflow = 0;
		for(i = fg->nvars - 1; i >= 0; i--){
			config[i]++;
			if(config[i] < fg->varsizes[i])
				break;
			config[i] = 0;
			if(i == 0)
				overflow = 1;
		}

	} while(!overflow);

	free(config);

	if(prob_evidence > 0.0)
		return prob_joint / prob_evidence;
	return 0.0;
}

/* MAP inference (find most probable configuration) */
Tensor*
tensorfgmap(TensorFactorGraph *fg)
{
	Tensor *result;
	int *config, *best_config;
	int i, j;
	float prod, best_prod;
	int overflow;
	int shape[1];
	float *rdata;

	if(fg == nil)
		return nil;

	config = mallocz(sizeof(int) * fg->nvars, 1);
	best_config = mallocz(sizeof(int) * fg->nvars, 1);
	if(config == nil || best_config == nil){
		free(config);
		free(best_config);
		return nil;
	}

	best_prod = -1.0;

	/* Enumerate all configurations */
	do {
		/* Compute product of all factors */
		prod = 1.0;
		for(i = 0; i < fg->nfactors; i++){
			TensorFactor *f = fg->factors[i];
			int *fconfig = mallocz(sizeof(int) * f->nvars, 1);
			if(fconfig == nil)
				continue;

			for(j = 0; j < f->nvars; j++)
				fconfig[j] = config[f->vars[j]];

			prod *= tensorfactorget(f, fconfig);
			free(fconfig);
		}

		if(prod > best_prod){
			best_prod = prod;
			memcpy(best_config, config, sizeof(int) * fg->nvars);
		}

		/* Increment configuration */
		overflow = 0;
		for(i = fg->nvars - 1; i >= 0; i--){
			config[i]++;
			if(config[i] < fg->varsizes[i])
				break;
			config[i] = 0;
			if(i == 0)
				overflow = 1;
		}

	} while(!overflow);

	free(config);

	/* Return best configuration as tensor */
	shape[0] = fg->nvars;
	result = tensorcreate(TensorFloat, 1, shape);
	if(result != nil){
		rdata = (float*)result->data;
		for(i = 0; i < fg->nvars; i++)
			rdata[i] = (float)best_config[i];
	}

	free(best_config);
	return result;
}

/* ============================================================
 * Belief Propagation
 * ============================================================ */

/* Run belief propagation (loopy BP) */
void
tensorfgbp(TensorFactorGraph *fg, int iterations)
{
	/* Messages from factors to variables */
	Tensor ***f2v;
	/* Messages from variables to factors */
	Tensor ***v2f;
	int i, j, k, iter;
	int fi, vi;
	float sum;

	if(fg == nil || fg->nfactors == 0)
		return;

	/* Allocate message storage */
	/* For simplicity, this is a basic implementation */
	/* A full implementation would properly handle factor-variable edges */

	for(iter = 0; iter < iterations; iter++){
		/* Update factor to variable messages */
		for(fi = 0; fi < fg->nfactors; fi++){
			TensorFactor *f = fg->factors[fi];
			/* For each variable in this factor */
			for(vi = 0; vi < f->nvars; vi++){
				/* Marginalize factor over other variables */
				/* This is simplified - real BP is more complex */
			}
		}

		/* Update variable to factor messages */
		/* Product of incoming messages from other factors */
	}
}

/* Get beliefs for a variable after BP */
Tensor*
tensorfgbeliefs(TensorFactorGraph *fg, int var)
{
	/* After BP, beliefs are the product of all incoming messages */
	/* For now, fall back to exact marginal */
	return tensorfgmarginal(fg, var);
}
