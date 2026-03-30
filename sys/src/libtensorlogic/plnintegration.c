/*
 * Tensor Logic Integration with PLN
 * Bridges tensor-based and probabilistic reasoning
 */

#include <u.h>
#include <libc.h>
#include <plan9cog.h>
#include <plan9cog/tensorlogic.h>

/* ============================================================
 * AtomSpace to Tensor Relation Conversion
 * ============================================================ */

/* Convert AtomSpace predicate to tensor relation */
TensorRelation*
atomtotensorrel(AtomSpace *as, Atom *predicate)
{
	TensorRelation *rel;
	int i, j;
	int sizes[TENSOR_MAX_RANK];
	int arity;

	if(as == nil || predicate == nil)
		return nil;

	/* Determine arity from predicate links */
	arity = 2;  /* Default binary relation */

	/* Get domain sizes from atomspace */
	for(i = 0; i < arity; i++)
		sizes[i] = as->natoms > 0 ? as->natoms : 100;

	rel = tensorrelcreate(predicate->name, arity, sizes);
	if(rel == nil)
		return nil;

	/* Populate tensor from EvaluationLinks */
	for(i = 0; i < as->natoms; i++){
		Atom *a = as->atoms[i];
		if(a->type == EvaluationLink && a->outgoing != nil){
			/* Check if this link uses our predicate */
			if(a->outgoing[0] == predicate && a->noutgoing >= 3){
				/* Get argument indices */
				int idx1 = a->outgoing[1]->id % sizes[0];
				int idx2 = a->outgoing[2]->id % sizes[1];
				tensorrelassert(rel, idx1, idx2);
			}
		}
	}

	return rel;
}

/* Convert tensor relation back to AtomSpace */
void
tensorreltoadom(TensorRelation *rel, AtomSpace *as)
{
	int i, j;
	float *data;
	Atom *pred;
	char name[64];

	if(rel == nil || as == nil)
		return;

	/* Create or find predicate node */
	pred = atomcreate(as, PredicateNode, rel->name);
	if(pred == nil)
		return;

	/* Get tensor data */
	data = (float*)rel->tensor->data;

	/* Create EvaluationLinks for true tuples */
	for(i = 0; i < rel->domainsizes[0]; i++){
		for(j = 0; j < rel->domainsizes[1]; j++){
			int idx = i * rel->domainsizes[1] + j;
			if(data[idx] > 0.5){
				/* Create argument atoms */
				snprint(name, sizeof(name), "entity_%d", i);
				Atom *arg1 = atomcreate(as, ConceptNode, name);
				snprint(name, sizeof(name), "entity_%d", j);
				Atom *arg2 = atomcreate(as, ConceptNode, name);

				/* Create EvaluationLink */
				Atom *outgoing[3] = {pred, arg1, arg2};
				linkcreate(as, EvaluationLink, outgoing, 3);
			}
		}
	}
}

/* ============================================================
 * PLN Rule to Tensor Rule Conversion
 * ============================================================ */

/* Convert PLN rule to tensor rule */
TensorRule*
plntotensorrule(PlnInference *pln, PlnRule *rule)
{
	TensorRelation *head;
	TensorRelation **body;
	TensorRule *trule;
	int sizes[2] = {100, 100};
	int i;

	if(pln == nil || rule == nil)
		return nil;

	/* Create head relation */
	head = tensorrelcreate("Head", 2, sizes);
	if(head == nil)
		return nil;

	/* Create body relations */
	body = mallocz(sizeof(TensorRelation*) * 2, 1);
	if(body == nil){
		tensorrelfree(head);
		return nil;
	}

	body[0] = tensorrelcreate("Body1", 2, sizes);
	body[1] = tensorrelcreate("Body2", 2, sizes);

	if(body[0] == nil || body[1] == nil){
		tensorrelfree(head);
		tensorrelfree(body[0]);
		tensorrelfree(body[1]);
		free(body);
		return nil;
	}

	/* Create tensor rule */
	trule = tensorrulecreate(head, body, 2);
	if(trule != nil)
		tensorrulecompile(trule);

	return trule;
}

/* Convert tensor rule back to PLN rule */
void
tensorruletoopln(TensorRule *rule, PlnInference *pln)
{
	/* Create PLN rule from tensor rule */
	if(rule == nil || pln == nil)
		return;

	/* Create rule atoms in PLN's atomspace */
	/* This would create BindLink or similar */
}

/* ============================================================
 * Tensor-Enhanced PLN Inference
 * ============================================================ */

/* Forward chaining using tensor operations */
Tensor*
tensorplnforward(PlnInference *pln, int steps)
{
	TensorProgram *prog;
	Tensor *result;
	int i;

	if(pln == nil || pln->as == nil)
		return nil;

	/* Create tensor program from PLN rules */
	prog = tensorproginit();
	if(prog == nil)
		return nil;

	prog->maxiterations = steps;

	/* Convert PLN rules to tensor rules */
	for(i = 0; i < pln->nrules; i++){
		TensorRule *trule = plntotensorrule(pln, pln->rules[i]);
		if(trule != nil)
			tensorprogaddrule(prog, trule);
	}

	/* Run tensor-based forward chaining */
	tensorprogforward(prog);

	/* Extract results */
	if(prog->nrelations > 0){
		result = tensorcopy(prog->relations[0]->tensor);
	} else {
		int shape[1] = {1};
		result = tensorzeros(1, shape);
	}

	tensorprogfree(prog);
	return result;
}

/* Backward chaining using tensor operations */
Tensor*
tensorplnbackward(PlnInference *pln, Atom *goal)
{
	Tensor *result;
	int shape[1];

	if(pln == nil || goal == nil)
		return nil;

	/* Create goal tensor */
	shape[0] = pln->as->natoms > 0 ? pln->as->natoms : 100;
	result = tensorzeros(1, shape);

	/* Mark goal in tensor */
	if(goal->id < shape[0]){
		float *data = (float*)result->data;
		data[goal->id] = 1.0;
	}

	/* Backward propagation through rules */
	/* Apply transposed tensor equations */

	return result;
}

/* ============================================================
 * Embedding Integration with AtomSpace
 * ============================================================ */

/* Create embeddings for AtomSpace entities */
TensorEmbedding*
atomspaceembed(AtomSpace *as, int dims)
{
	TensorEmbedding *emb;
	int i;
	int shape[1];
	Tensor *vec;

	if(as == nil)
		return nil;

	emb = tensorembcreate(as->natoms, dims);
	if(emb == nil)
		return nil;

	/* Initialize embeddings for each atom */
	shape[0] = dims;
	for(i = 0; i < as->natoms; i++){
		Atom *a = as->atoms[i];
		if(a != nil && a->name != nil){
			tensorembsetname(emb, i, a->name);

			/* Initialize with random embedding */
			vec = tensorrand(1, shape);
			if(vec != nil){
				tensorembset(emb, i, vec);
				tensorfree(vec);
			}
		}
	}

	return emb;
}

/* Convert embeddings back to AtomSpace similarities */
void
embedtoatomspace(TensorEmbedding *emb, AtomSpace *as)
{
	int i, j;
	float dist;
	Atom *sim;
	char name[64];

	if(emb == nil || as == nil)
		return;

	/* Create SimilarityLinks for close embeddings */
	for(i = 0; i < emb->nobjects; i++){
		for(j = i + 1; j < emb->nobjects; j++){
			dist = tensorembdist(emb, i, j);

			/* If close enough, create similarity link */
			if(dist < 1.0){
				snprint(name, sizeof(name), "entity_%d", i);
				Atom *a1 = atomcreate(as, ConceptNode, name);
				snprint(name, sizeof(name), "entity_%d", j);
				Atom *a2 = atomcreate(as, ConceptNode, name);

				if(a1 != nil && a2 != nil){
					Atom *outgoing[2] = {a1, a2};
					Atom *link = linkcreate(as, SimilarityLink, outgoing, 2);

					/* Set truth value based on distance */
					if(link != nil){
						TruthValue tv;
						tv.strength = 1.0 / (1.0 + dist);
						tv.confidence = 0.9;
						tv.count = 10;
						atomsettruth(link, tv);
					}
				}
			}
		}
	}
}

/* ============================================================
 * Tensor Truth Value Computation
 * ============================================================ */

/* Compute truth value using tensor-based inference */
TruthValue
tensortruth(TensorRelation *rel, int *indices)
{
	TruthValue tv;
	float val;

	tv.strength = 0.0;
	tv.confidence = 0.0;
	tv.count = 0;

	if(rel == nil || indices == nil)
		return tv;

	/* Query relation tensor */
	val = (float)tensorrelquery(rel, indices[0], indices[1]);

	tv.strength = val;
	tv.confidence = 0.9;  /* High confidence for tensor-based */
	tv.count = 100;

	return tv;
}

/* Combine tensor and PLN truth values */
TruthValue
tensorplntruth(TruthValue tensor_tv, TruthValue pln_tv, float weight)
{
	TruthValue combined;

	/* Weighted combination */
	combined.strength = weight * tensor_tv.strength +
			    (1.0 - weight) * pln_tv.strength;

	/* Combine confidence */
	combined.confidence = tensor_tv.confidence * pln_tv.confidence;

	/* Sum counts */
	combined.count = tensor_tv.count + pln_tv.count;

	return combined;
}
