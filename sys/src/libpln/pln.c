/*
 * PLN (Probabilistic Logic Networks) implementation
 */

#include <u.h>
#include <libc.h>
#include <plan9cog/atomspace.h>
#include <plan9cog/pln.h>

PlnInference*
plninit(AtomSpace *as)
{
	PlnInference *pln;
	
	pln = mallocz(sizeof(PlnInference), 1);
	if(pln == nil)
		return nil;
	
	pln->as = as;
	pln->maxsteps = 1000;
	pln->minconf = 0.1;
	pln->nrules = 0;
	pln->rules = nil;
	
	return pln;
}

void
plnfree(PlnInference *pln)
{
	int i;
	
	if(pln == nil)
		return;
	
	lock(pln);
	for(i = 0; i < pln->nrules; i++){
		if(pln->rules[i]){
			free(pln->rules[i]->name);
			free(pln->rules[i]);
		}
	}
	unlock(pln);
	
	free(pln->rules);
	free(pln);
}

void
plnaddrule(PlnInference *pln, PlnRule *rule)
{
	PlnRule **newrules;
	
	lock(pln);
	newrules = realloc(pln->rules, sizeof(PlnRule*) * (pln->nrules + 1));
	if(newrules != nil){
		pln->rules = newrules;
		pln->rules[pln->nrules++] = rule;
	}
	unlock(pln);
}

/* Deduction formula: P(A->C) given P(A->B) and P(B->C) */
TruthValue
plndeduction(TruthValue ab, TruthValue bc)
{
	TruthValue result;
	
	result.strength = ab.strength * bc.strength;
	result.confidence = ab.confidence * bc.confidence;
	result.count = ab.count + bc.count;
	
	return result;
}

/* Induction formula: P(B->A) given P(A->B) */
TruthValue
plninduction(TruthValue ab, TruthValue ba)
{
	TruthValue result;
	float k = 1.0; /* Indifference prior */
	
	result.strength = (ab.strength * ba.strength) / 
		((ab.strength * ba.strength) + k * (1 - ab.strength) * (1 - ba.strength));
	result.confidence = ab.confidence * ba.confidence * 0.9;
	result.count = ab.count;
	
	return result;
}

/* Abduction formula: P(C->A) given P(A->B) and P(B->C) */
TruthValue
plnabduction(TruthValue ab, TruthValue bc)
{
	TruthValue result;
	
	/* Simplified abduction */
	result.strength = ab.strength * bc.strength * 0.8;
	result.confidence = ab.confidence * bc.confidence * 0.7;
	result.count = ab.count + bc.count;
	
	return result;
}

/* Revision formula: combine two truth values */
TruthValue
plnrevision(TruthValue a, TruthValue b)
{
	TruthValue result;
	float wa, wb, wtotal;
	
	wa = a.confidence;
	wb = b.confidence;
	wtotal = wa + wb;
	
	if(wtotal > 0){
		result.strength = (a.strength * wa + b.strength * wb) / wtotal;
		result.confidence = wtotal / (wtotal + 1);
	} else {
		result.strength = 0.5;
		result.confidence = 0.0;
	}
	result.count = a.count + b.count;
	
	return result;
}

/* AND formula */
TruthValue
plnand(TruthValue a, TruthValue b)
{
	TruthValue result;
	
	result.strength = a.strength * b.strength;
	result.confidence = a.confidence * b.confidence;
	result.count = a.count + b.count;
	
	return result;
}

/* OR formula */
TruthValue
plnor(TruthValue a, TruthValue b)
{
	TruthValue result;
	
	result.strength = a.strength + b.strength - (a.strength * b.strength);
	result.confidence = (a.confidence + b.confidence) / 2;
	result.count = a.count + b.count;
	
	return result;
}

/* NOT formula */
TruthValue
plnnot(TruthValue a)
{
	TruthValue result;
	
	result.strength = 1.0 - a.strength;
	result.confidence = a.confidence;
	result.count = a.count;
	
	return result;
}

/* Direct evaluation */
TruthValue
plneval(PlnInference *pln, Atom *query)
{
	TruthValue tv;
	
	if(query == nil){
		tv.strength = 0.0;
		tv.confidence = 0.0;
		tv.count = 0;
		return tv;
	}
	
	return atomgettruth(query);
}

/* PLN inference statistics - internal tracking */
static PlnStats plnglobalstats;

/* Find matching links by type and pattern */
static Atom**
plnfindlinks(AtomSpace *as, int linktype, int *nresults)
{
	Atom **results;
	int i, count, maxresults;
	Atom *a;

	maxresults = 128;
	results = mallocz(sizeof(Atom*) * maxresults, 1);
	if(results == nil){
		*nresults = 0;
		return nil;
	}

	count = 0;
	lock(as);
	for(i = 0; i < as->natoms; i++){
		a = as->atoms[i];
		if(a && a->type == linktype && a->outgoing != nil){
			if(count >= maxresults){
				maxresults *= 2;
				results = realloc(results, sizeof(Atom*) * maxresults);
				if(results == nil){
					unlock(as);
					*nresults = 0;
					return nil;
				}
			}
			results[count++] = a;
		}
	}
	unlock(as);

	*nresults = count;
	return results;
}

/* Check if atom matches target or is connected to target */
static int
plnrelatesto(Atom *a, Atom *target)
{
	int i;

	if(a == target)
		return 1;

	if(a->outgoing == nil)
		return 0;

	for(i = 0; i < a->noutgoing; i++){
		if(a->outgoing[i] == target)
			return 1;
	}

	return 0;
}

/* Apply deduction rule: A->B, B->C => A->C */
static Atom*
plnapplydeduction(PlnInference *pln, Atom *ab, Atom *bc)
{
	Atom *result;
	Atom *outgoing[2];
	TruthValue tv;

	if(ab == nil || bc == nil)
		return nil;

	if(ab->noutgoing < 2 || bc->noutgoing < 2)
		return nil;

	/* Check if B matches: ab->outgoing[1] == bc->outgoing[0] */
	if(ab->outgoing[1] != bc->outgoing[0])
		return nil;

	/* Create A->C link */
	outgoing[0] = ab->outgoing[0];  /* A */
	outgoing[1] = bc->outgoing[1];  /* C */

	result = linkcreate(pln->as, ab->type, outgoing, 2);
	if(result == nil)
		return nil;

	/* Compute truth value using deduction formula */
	tv = plndeduction(atomgettruth(ab), atomgettruth(bc));
	atomsettruth(result, tv);

	plnglobalstats.tvcompute++;

	return result;
}

/* Forward chaining - apply rules to generate new atoms */
Atom**
plnforward(PlnInference *pln, Atom *target, int maxsteps, int *n)
{
	Atom **results;
	Atom **links;
	int nlinks, i, j, step, count;
	Atom *newatom;
	int maxresults;

	maxresults = maxsteps * 2;
	results = mallocz(sizeof(Atom*) * maxresults, 1);
	if(results == nil){
		*n = 0;
		return nil;
	}

	count = 0;

	/* Get all inheritance/implication links */
	links = plnfindlinks(pln->as, InheritanceLink, &nlinks);
	if(links == nil){
		*n = 0;
		return results;
	}

	/* Forward chaining: apply deduction rules */
	for(step = 0; step < maxsteps && count < maxresults; step++){
		plnglobalstats.forward++;

		/* Try all pairs of links */
		for(i = 0; i < nlinks && count < maxresults; i++){
			for(j = 0; j < nlinks && count < maxresults; j++){
				if(i == j)
					continue;

				/* Only process links related to target */
				if(target != nil && !plnrelatesto(links[i], target))
					continue;

				newatom = plnapplydeduction(pln, links[i], links[j]);
				if(newatom != nil){
					results[count++] = newatom;
					plnglobalstats.inferences++;
					plnglobalstats.rulematch++;
				}
			}
		}

		/* If no progress, break */
		if(count == 0)
			break;
	}

	free(links);
	*n = count;
	return results;
}

/* Backward chaining - prove goal by finding premises */
Atom**
plnbackward(PlnInference *pln, Atom *goal, int maxsteps, int *n)
{
	Atom **results;
	Atom **links;
	int nlinks, i, step, count;
	int maxresults;

	maxresults = maxsteps * 2;
	results = mallocz(sizeof(Atom*) * maxresults, 1);
	if(results == nil){
		*n = 0;
		return nil;
	}

	count = 0;

	if(goal == nil){
		*n = 0;
		return results;
	}

	/* Get all links */
	links = plnfindlinks(pln->as, InheritanceLink, &nlinks);
	if(links == nil){
		links = plnfindlinks(pln->as, ImplicationLink, &nlinks);
	}

	if(links == nil){
		*n = 0;
		return results;
	}

	/* Backward chaining: find links that conclude to goal */
	for(step = 0; step < maxsteps && count < maxresults; step++){
		plnglobalstats.backward++;

		for(i = 0; i < nlinks && count < maxresults; i++){
			/* Check if this link concludes to goal */
			if(links[i]->noutgoing >= 2 && links[i]->outgoing[1] == goal){
				/* This link concludes to goal - add premise */
				results[count++] = links[i]->outgoing[0];
				plnglobalstats.inferences++;

				/* Recursively find premises for this node */
				if(step + 1 < maxsteps){
					int j;
					for(j = 0; j < nlinks && count < maxresults; j++){
						if(links[j]->noutgoing >= 2 &&
						   links[j]->outgoing[1] == links[i]->outgoing[0]){
							results[count++] = links[j]->outgoing[0];
							plnglobalstats.inferences++;
						}
					}
				}
			}
		}

		/* If no progress, break */
		if(count == 0)
			break;
	}

	free(links);
	*n = count;
	return results;
}

void
plnstats(PlnInference *pln, PlnStats *stats)
{
	if(stats == nil)
		return;

	lock(pln);
	*stats = plnglobalstats;
	unlock(pln);
}

void
plnresetstats(PlnInference *pln)
{
	lock(pln);
	memset(&plnglobalstats, 0, sizeof(PlnStats));
	unlock(pln);
}
