/*
 * Logic-to-Tensor Mapping
 * Convert Datalog rules to tensor equations
 *
 * Relations become Boolean tensors
 * Rules become einsum operations with step function
 */

#include <u.h>
#include <libc.h>
#include <plan9cog/tensorlogic.h>

/* ============================================================
 * Tensor Relations
 * ============================================================ */

/* Create tensor relation */
TensorRelation*
tensorrelcreate(char *name, int arity, int *sizes)
{
	TensorRelation *rel;
	int i;

	rel = mallocz(sizeof(TensorRelation), 1);
	if(rel == nil)
		return nil;

	rel->name = strdup(name);
	rel->arity = arity;

	rel->domainsizes = mallocz(sizeof(int) * arity, 1);
	rel->argnames = mallocz(sizeof(char*) * arity, 1);
	if(rel->domainsizes == nil || rel->argnames == nil){
		free(rel->name);
		free(rel->domainsizes);
		free(rel->argnames);
		free(rel);
		return nil;
	}

	for(i = 0; i < arity; i++){
		rel->domainsizes[i] = sizes[i];
		rel->argnames[i] = nil;
	}

	/* Create underlying Boolean tensor */
	rel->tensor = tensorcreate(TensorFloat, arity, sizes);
	if(rel->tensor == nil){
		free(rel->name);
		free(rel->domainsizes);
		free(rel->argnames);
		free(rel);
		return nil;
	}

	return rel;
}

/* Free tensor relation */
void
tensorrelfree(TensorRelation *rel)
{
	int i;

	if(rel == nil)
		return;

	free(rel->name);
	for(i = 0; i < rel->arity; i++)
		free(rel->argnames[i]);
	free(rel->argnames);
	free(rel->domainsizes);
	tensorfree(rel->tensor);
	free(rel);
}

/* Assert tuple in relation (variadic) */
void
tensorrelassert(TensorRelation *rel, ...)
{
	va_list args;
	int indices[TENSOR_MAX_RANK];
	int i;
	ulong idx;
	int stride;
	float *data;

	if(rel == nil || rel->tensor == nil)
		return;

	va_start(args, rel);
	for(i = 0; i < rel->arity; i++)
		indices[i] = va_arg(args, int);
	va_end(args);

	/* Calculate linear index */
	idx = 0;
	stride = 1;
	for(i = rel->arity - 1; i >= 0; i--){
		idx += indices[i] * stride;
		stride *= rel->domainsizes[i];
	}

	if(idx < rel->tensor->nelems){
		data = (float*)rel->tensor->data;
		data[idx] = 1.0;
	}
}

/* Query tuple in relation (variadic) */
int
tensorrelquery(TensorRelation *rel, ...)
{
	va_list args;
	int indices[TENSOR_MAX_RANK];
	int i;
	ulong idx;
	int stride;
	float *data;

	if(rel == nil || rel->tensor == nil)
		return 0;

	va_start(args, rel);
	for(i = 0; i < rel->arity; i++)
		indices[i] = va_arg(args, int);
	va_end(args);

	/* Calculate linear index */
	idx = 0;
	stride = 1;
	for(i = rel->arity - 1; i >= 0; i--){
		idx += indices[i] * stride;
		stride *= rel->domainsizes[i];
	}

	if(idx < rel->tensor->nelems){
		data = (float*)rel->tensor->data;
		return data[idx] > 0.5 ? 1 : 0;
	}

	return 0;
}

/* Get underlying tensor */
Tensor*
tensorrelget(TensorRelation *rel)
{
	if(rel == nil)
		return nil;
	return rel->tensor;
}

/* ============================================================
 * Tensor Rules
 * ============================================================ */

/* Create tensor rule */
TensorRule*
tensorrulecreate(TensorRelation *head, TensorRelation **body, int n)
{
	TensorRule *rule;
	int i;

	rule = mallocz(sizeof(TensorRule), 1);
	if(rule == nil)
		return nil;

	rule->head = head;
	rule->nbody = n;

	if(n > 0){
		rule->body = mallocz(sizeof(TensorRelation*) * n, 1);
		if(rule->body == nil){
			free(rule);
			return nil;
		}
		for(i = 0; i < n; i++)
			rule->body[i] = body[i];
	} else {
		rule->body = nil;
	}

	rule->equation = nil;
	return rule;
}

/* Free tensor rule */
void
tensorrulefree(TensorRule *rule)
{
	if(rule == nil)
		return;

	free(rule->body);
	tenseqfree(rule->equation);
	free(rule);
}

/* Generate einsum equation from rule */
void
tensorrulecompile(TensorRule *rule)
{
	char lhs[64], rhs[256];
	char indices[] = "xyzwvutsrqponmlkjihgfedcba";
	int nextidx = 0;
	int i, j;

	if(rule == nil || rule->head == nil)
		return;

	/* Build LHS indices from head */
	memset(lhs, 0, sizeof(lhs));
	for(i = 0; i < rule->head->arity && nextidx < 26; i++){
		lhs[i] = indices[nextidx++];
	}

	/* Build RHS from body */
	memset(rhs, 0, sizeof(rhs));

	if(rule->nbody == 0){
		/* Fact: just copy */
		strcpy(rhs, lhs);
	} else {
		int pos = 0;

		for(i = 0; i < rule->nbody; i++){
			if(i > 0)
				rhs[pos++] = '*';

			/* Add indices for this body relation */
			for(j = 0; j < rule->body[i]->arity; j++){
				/* Try to reuse indices for shared variables */
				/* Simplified: just use sequential indices */
				if(nextidx < 26)
					rhs[pos++] = indices[nextidx++];
			}
		}
	}

	/* Create equation with step function for Boolean semantics */
	rule->equation = tenseqcreate(lhs, rhs, TensorStep);
}

/* ============================================================
 * Tensor Programs (Datalog)
 * ============================================================ */

/* Initialize tensor program */
TensorProgram*
tensorproginit(void)
{
	TensorProgram *prog;

	prog = mallocz(sizeof(TensorProgram), 1);
	if(prog == nil)
		return nil;

	prog->relations = nil;
	prog->nrelations = 0;
	prog->rules = nil;
	prog->nrules = 0;
	prog->maxiterations = 100;

	return prog;
}

/* Free tensor program */
void
tensorprogfree(TensorProgram *prog)
{
	int i;

	if(prog == nil)
		return;

	for(i = 0; i < prog->nrelations; i++)
		tensorrelfree(prog->relations[i]);
	free(prog->relations);

	for(i = 0; i < prog->nrules; i++)
		tensorrulefree(prog->rules[i]);
	free(prog->rules);

	free(prog);
}

/* Add rule to program */
void
tensorprogaddrule(TensorProgram *prog, TensorRule *rule)
{
	TensorRule **newrules;

	if(prog == nil || rule == nil)
		return;

	newrules = realloc(prog->rules, sizeof(TensorRule*) * (prog->nrules + 1));
	if(newrules == nil)
		return;

	prog->rules = newrules;
	prog->rules[prog->nrules++] = rule;

	/* Compile rule to equation */
	tensorrulecompile(rule);
}

/* Forward chaining: apply rules until fixpoint */
void
tensorprogforward(TensorProgram *prog)
{
	int iter, i;
	int changed;
	Tensor *newval, *stepped;
	TensorRule *rule;
	Tensor *inputs[8];

	if(prog == nil)
		return;

	for(iter = 0; iter < prog->maxiterations; iter++){
		changed = 0;

		for(i = 0; i < prog->nrules; i++){
			rule = prog->rules[i];
			if(rule == nil || rule->equation == nil)
				continue;

			/* Gather input tensors from body relations */
			int ninputs = 0;
			int j;
			for(j = 0; j < rule->nbody && ninputs < 8; j++){
				if(rule->body[j] != nil && rule->body[j]->tensor != nil)
					inputs[ninputs++] = rule->body[j]->tensor;
			}

			/* Evaluate equation */
			if(ninputs >= 2){
				newval = tensoreinsum("ij,jk->ik", inputs[0], inputs[1]);
				if(newval != nil){
					/* Apply step function for Boolean semantics */
					stepped = tensorstep(newval);
					tensorfree(newval);

					if(stepped != nil){
						/* Merge with head relation (OR = max) */
						float *head_data = (float*)rule->head->tensor->data;
						float *new_data = (float*)stepped->data;
						ulong k;

						for(k = 0; k < rule->head->tensor->nelems && k < stepped->nelems; k++){
							if(new_data[k] > head_data[k]){
								head_data[k] = new_data[k];
								changed = 1;
							}
						}

						tensorfree(stepped);
					}
				}
			}
		}

		if(!changed)
			break;  /* Fixpoint reached */
	}
}

/* Query relation in program */
Tensor*
tensorprogquery(TensorProgram *prog, TensorRelation *rel)
{
	if(prog == nil || rel == nil)
		return nil;

	/* Run forward chaining to completion */
	tensorprogforward(prog);

	/* Return copy of relation tensor */
	return tensorcopy(rel->tensor);
}

/* ============================================================
 * Tensor Equations
 * ============================================================ */

/* Create tensor equation */
TensorEquation*
tenseqcreate(char *lhs, char *rhs, int nonlin)
{
	TensorEquation *eq;

	eq = mallocz(sizeof(TensorEquation), 1);
	if(eq == nil)
		return nil;

	eq->lhs = strdup(lhs);
	eq->rhs = strdup(rhs);
	eq->nonlinearity = nonlin;
	eq->temperature = 1.0;
	eq->terms = nil;
	eq->termidx = nil;
	eq->nterms = 0;
	eq->result = nil;

	return eq;
}

/* Free tensor equation */
void
tenseqfree(TensorEquation *eq)
{
	int i;

	if(eq == nil)
		return;

	free(eq->lhs);
	free(eq->rhs);

	for(i = 0; i < eq->nterms; i++){
		free(eq->terms[i]);
		free(eq->termidx[i]);
	}
	free(eq->terms);
	free(eq->termidx);

	tensorfree(eq->result);
	free(eq);
}

/* Evaluate tensor equation */
Tensor*
tenseqeval(TensorEquation *eq, Tensor **inputs, int ninputs)
{
	Tensor *result, *applied;
	char eqstr[256];

	if(eq == nil || inputs == nil || ninputs < 1)
		return nil;

	/* Build einsum string */
	snprint(eqstr, sizeof(eqstr), "%s->%s", eq->rhs, eq->lhs);

	/* Evaluate based on number of inputs */
	if(ninputs == 1){
		result = tensorcopy(inputs[0]);
	} else if(ninputs == 2){
		result = tensoreinsum(eqstr, inputs[0], inputs[1]);
	} else {
		result = tensoreinsumv(eqstr, inputs, ninputs);
	}

	if(result == nil)
		return nil;

	/* Apply nonlinearity */
	applied = tensorapply(result, eq->nonlinearity);
	tensorfree(result);

	return applied;
}
