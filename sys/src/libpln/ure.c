/*
 * Unified Rule Engine (URE) implementation
 */

#include <u.h>
#include <libc.h>
#include <plan9cog/atomspace.h>
#include <plan9cog/pln.h>

UreChainer*
ureinit(AtomSpace *as, Atom *rulebase)
{
	UreChainer *ure;
	
	ure = mallocz(sizeof(UreChainer), 1);
	if(ure == nil)
		return nil;
	
	ure->pln = plninit(as);
	if(ure->pln == nil){
		free(ure);
		return nil;
	}
	
	ure->rulebase = rulebase;
	ure->maxiter = 100;
	ure->complexity = 1.0;
	
	return ure;
}

void
urefree(UreChainer *ure)
{
	if(ure == nil)
		return;
	
	plnfree(ure->pln);
	free(ure);
}

/* Internal: get rules from rulebase atom */
static PlnRule**
uregetrules(UreChainer *ure, int *nrules)
{
	if(ure == nil || ure->pln == nil){
		*nrules = 0;
		return nil;
	}

	*nrules = ure->pln->nrules;
	return ure->pln->rules;
}

/* Internal: calculate rule priority based on complexity */
static float
urerulepriority(PlnRule *rule, float complexity)
{
	if(rule == nil)
		return 0.0;

	return rule->weight / (1.0 + complexity);
}

/* Internal: check if rule premises match atoms in atomspace */
static int
urematchpremises(AtomSpace *as, PlnRule *rule, Atom ***matches, int *nmatches)
{
	int i, count;
	Atom **found;
	int nfound;

	if(rule == nil || rule->premises == nil || rule->npremises == 0){
		*nmatches = 0;
		*matches = nil;
		return 0;
	}

	*matches = mallocz(sizeof(Atom*) * rule->npremises, 1);
	if(*matches == nil){
		*nmatches = 0;
		return 0;
	}

	count = 0;
	for(i = 0; i < rule->npremises; i++){
		found = atommatch(as, &rule->premises[i], &nfound);
		if(found != nil && nfound > 0){
			(*matches)[count++] = found[0];
			free(found);
		}
	}

	*nmatches = count;
	return count == rule->npremises;
}

/* Apply URE rule to generate conclusion */
static Atom*
ureapplyrule(UreChainer *ure, PlnRule *rule, Atom **premises, int npremises)
{
	Atom *result;
	TruthValue tv;
	int i;

	if(rule == nil || rule->conclusion == nil)
		return nil;

	/* Create conclusion atom from pattern */
	if(rule->conclusion->noutgoing > 0 && npremises >= 2){
		/* Link conclusion */
		result = linkcreate(ure->pln->as, rule->conclusion->type, premises, npremises);
	} else {
		/* Node conclusion */
		result = atomcreate(ure->pln->as, rule->conclusion->type, rule->conclusion->name);
	}

	if(result == nil)
		return nil;

	/* Compute truth value based on premises */
	if(rule->formula != nil && rule->formula->compute != nil && npremises > 0){
		TruthValue *tvs = mallocz(sizeof(TruthValue) * npremises, 1);
		if(tvs != nil){
			for(i = 0; i < npremises; i++){
				tvs[i] = atomgettruth(premises[i]);
			}
			tv = rule->formula->compute(tvs, npremises);
			atomsettruth(result, tv);
			free(tvs);
		}
	} else if(npremises >= 2){
		/* Default: use deduction formula */
		tv = plndeduction(atomgettruth(premises[0]), atomgettruth(premises[1]));
		atomsettruth(result, tv);
	}

	return result;
}

Atom**
urechain(UreChainer *ure, Atom *target, int *n)
{
	Atom **results;
	PlnRule **rules;
	int nrules, i, iter, count;
	Atom **matches;
	int nmatches;
	Atom *newatom;
	int maxresults;
	float priority, bestpriority;
	int bestrule;

	maxresults = ure->maxiter * 2;
	results = mallocz(sizeof(Atom*) * maxresults, 1);
	if(results == nil){
		*n = 0;
		return nil;
	}

	count = 0;
	rules = uregetrules(ure, &nrules);

	/* If no rules defined, use PLN forward chaining */
	if(rules == nil || nrules == 0){
		int nforward;
		Atom **forward = plnforward(ure->pln, target, ure->maxiter, &nforward);
		if(forward != nil){
			for(i = 0; i < nforward && count < maxresults; i++){
				results[count++] = forward[i];
			}
			free(forward);
		}
		*n = count;
		return results;
	}

	/* URE iteration: select and apply rules based on priority */
	for(iter = 0; iter < ure->maxiter && count < maxresults; iter++){
		bestpriority = 0.0;
		bestrule = -1;

		/* Find highest priority rule that matches */
		for(i = 0; i < nrules; i++){
			if(rules[i] == nil)
				continue;

			priority = urerulepriority(rules[i], ure->complexity);
			if(priority > bestpriority){
				if(urematchpremises(ure->pln->as, rules[i], &matches, &nmatches)){
					bestpriority = priority;
					bestrule = i;
					free(matches);
				}
			}
		}

		/* Apply best matching rule */
		if(bestrule >= 0){
			if(urematchpremises(ure->pln->as, rules[bestrule], &matches, &nmatches)){
				newatom = ureapplyrule(ure, rules[bestrule], matches, nmatches);
				if(newatom != nil){
					results[count++] = newatom;

					/* Check if we reached target */
					if(target != nil && newatom == target)
						break;
				}
				free(matches);
			}
		} else {
			/* No matching rules - try forward chaining */
			int nforward;
			Atom **forward = plnforward(ure->pln, target, 1, &nforward);
			if(forward != nil){
				for(i = 0; i < nforward && count < maxresults; i++){
					results[count++] = forward[i];
				}
				free(forward);
			}
			break;
		}

		/* Increase complexity penalty each iteration */
		ure->complexity += 0.1;
	}

	*n = count;
	return results;
}

/* Attention allocation for PLN */
void
plnallocattention(PlnInference *pln, Atom *a, short amount)
{
	AttentionValue av;
	
	av = atomgetattention(a);
	av.sti += amount;
	atomsetattention(a, av);
}

short
plngetattention(PlnInference *pln, Atom *a)
{
	AttentionValue av;
	
	av = atomgetattention(a);
	return av.sti;
}

void
plnspreadattention(PlnInference *pln, Atom *source)
{
	int i, nincoming;
	Atom **incoming;
	AttentionValue av;
	short spread;
	
	av = atomgetattention(source);
	if(av.sti <= 0)
		return;
	
	incoming = atomgetincoming(pln->as, source, &nincoming);
	if(incoming == nil || nincoming == 0)
		return;
	
	spread = av.sti / (nincoming + 1);
	
	for(i = 0; i < nincoming; i++){
		plnallocattention(pln, incoming[i], spread);
	}
	
	free(incoming);
}
