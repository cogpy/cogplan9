/*
 * Pattern Mining implementation for Plan9Cog
 * Extracts frequent patterns from AtomSpace
 */

#include <u.h>
#include <libc.h>
#include <plan9cog.h>

/* Pattern frequency tracking */
typedef struct PatternFreq PatternFreq;
struct PatternFreq {
	Pattern *pat;
	int count;
	float support;
	float confidence;
};

/* Internal: create a pattern from an atom */
static Pattern*
atomtopattern(Atom *a)
{
	Pattern *pat;
	int i;

	if(a == nil)
		return nil;

	pat = mallocz(sizeof(Pattern), 1);
	if(pat == nil)
		return nil;

	pat->type = a->type;
	pat->name = a->name ? strdup(a->name) : nil;
	pat->wildcard = 0;

	if(a->noutgoing > 0 && a->outgoing != nil){
		pat->noutgoing = a->noutgoing;
		pat->outgoing = mallocz(sizeof(Pattern*) * a->noutgoing, 1);
		if(pat->outgoing == nil){
			free(pat->name);
			free(pat);
			return nil;
		}

		for(i = 0; i < a->noutgoing; i++){
			pat->outgoing[i] = atomtopattern(a->outgoing[i]);
		}
	} else {
		pat->noutgoing = 0;
		pat->outgoing = nil;
	}

	return pat;
}

/* Internal: free a pattern */
static void
freepattern(Pattern *pat)
{
	int i;

	if(pat == nil)
		return;

	for(i = 0; i < pat->noutgoing; i++){
		freepattern(pat->outgoing[i]);
	}
	free(pat->outgoing);
	free(pat->name);
	free(pat);
}

/* Internal: compare two patterns for equality */
static int
patterneq(Pattern *a, Pattern *b)
{
	int i;

	if(a == nil && b == nil)
		return 1;
	if(a == nil || b == nil)
		return 0;

	if(a->type != b->type)
		return 0;

	if(a->wildcard != b->wildcard)
		return 0;

	if(a->name != nil && b->name != nil){
		if(strcmp(a->name, b->name) != 0)
			return 0;
	} else if(a->name != nil || b->name != nil){
		return 0;
	}

	if(a->noutgoing != b->noutgoing)
		return 0;

	for(i = 0; i < a->noutgoing; i++){
		if(!patterneq(a->outgoing[i], b->outgoing[i]))
			return 0;
	}

	return 1;
}

/* Internal: count pattern matches */
static int
countmatches(AtomSpace *as, Pattern *pat)
{
	int n;
	Atom **matches;

	matches = atommatch(as, pat, &n);
	if(matches != nil)
		free(matches);

	return n;
}

/* Initialize pattern miner */
PatternMiner*
patterninit(AtomSpace *as)
{
	PatternMiner *pm;

	pm = mallocz(sizeof(PatternMiner), 1);
	if(pm == nil)
		return nil;

	pm->as = as;
	pm->minsupport = 2;
	pm->minconf = 0.5;
	pm->patterns = nil;
	pm->npatterns = 0;

	return pm;
}

/* Free pattern miner */
void
patternfree(PatternMiner *pm)
{
	int i;

	if(pm == nil)
		return;

	lock(pm);
	for(i = 0; i < pm->npatterns; i++){
		freepattern(pm->patterns[i]);
	}
	free(pm->patterns);
	unlock(pm);

	free(pm);
}

/* Mine patterns from atomspace */
void
patternmine(PatternMiner *pm, int minsupport)
{
	int i, j, npatterns, maxpatterns;
	Pattern **patterns;
	PatternFreq *freq;
	int nfreq, maxfreq;
	Atom *a;
	Pattern *pat;
	int found;

	if(pm == nil || pm->as == nil)
		return;

	pm->minsupport = minsupport;

	lock(pm);

	/* Free existing patterns */
	for(i = 0; i < pm->npatterns; i++){
		freepattern(pm->patterns[i]);
	}
	free(pm->patterns);

	/* Collect candidate patterns from atoms */
	maxfreq = pm->as->natoms;
	freq = mallocz(sizeof(PatternFreq) * maxfreq, 1);
	if(freq == nil){
		pm->patterns = nil;
		pm->npatterns = 0;
		unlock(pm);
		return;
	}

	nfreq = 0;

	/* Generate patterns from each atom */
	for(i = 0; i < pm->as->natoms; i++){
		a = pm->as->atoms[i];
		if(a == nil)
			continue;

		pat = atomtopattern(a);
		if(pat == nil)
			continue;

		/* Check if pattern already exists */
		found = 0;
		for(j = 0; j < nfreq; j++){
			if(patterneq(freq[j].pat, pat)){
				freq[j].count++;
				found = 1;
				freepattern(pat);
				break;
			}
		}

		if(!found && nfreq < maxfreq){
			freq[nfreq].pat = pat;
			freq[nfreq].count = 1;
			nfreq++;
		} else if(!found){
			freepattern(pat);
		}
	}

	/* Calculate support and filter by minsupport */
	maxpatterns = 128;
	patterns = mallocz(sizeof(Pattern*) * maxpatterns, 1);
	npatterns = 0;

	for(i = 0; i < nfreq; i++){
		freq[i].support = (float)freq[i].count / pm->as->natoms;

		if(freq[i].count >= minsupport){
			if(npatterns >= maxpatterns){
				maxpatterns *= 2;
				patterns = realloc(patterns, sizeof(Pattern*) * maxpatterns);
				if(patterns == nil)
					break;
			}
			patterns[npatterns++] = freq[i].pat;
			freq[i].pat = nil; /* Transfer ownership */
		} else {
			freepattern(freq[i].pat);
		}
	}

	free(freq);

	pm->patterns = patterns;
	pm->npatterns = npatterns;

	unlock(pm);
}

/* Get mined patterns */
Pattern**
patternget(PatternMiner *pm, int *n)
{
	if(pm == nil){
		*n = 0;
		return nil;
	}

	*n = pm->npatterns;
	return pm->patterns;
}

/* Calculate pattern support */
float
patternsupport(PatternMiner *pm, Pattern *pat)
{
	int matches;

	if(pm == nil || pm->as == nil || pat == nil)
		return 0.0;

	matches = countmatches(pm->as, pat);
	if(pm->as->natoms == 0)
		return 0.0;

	return (float)matches / pm->as->natoms;
}

/* Calculate pattern confidence */
float
patternconfidence(PatternMiner *pm, Pattern *pat)
{
	int matches;
	Pattern *generalized;
	int gmatches;
	float conf;

	if(pm == nil || pm->as == nil || pat == nil)
		return 0.0;

	matches = countmatches(pm->as, pat);
	if(matches == 0)
		return 0.0;

	/* Generalize pattern by making first outgoing a wildcard */
	if(pat->noutgoing > 0 && pat->outgoing != nil){
		generalized = mallocz(sizeof(Pattern), 1);
		if(generalized == nil)
			return 0.0;

		generalized->type = pat->type;
		generalized->noutgoing = pat->noutgoing;
		generalized->outgoing = mallocz(sizeof(Pattern*) * pat->noutgoing, 1);
		if(generalized->outgoing == nil){
			free(generalized);
			return 0.0;
		}

		/* First outgoing is wildcard */
		generalized->outgoing[0] = mallocz(sizeof(Pattern), 1);
		if(generalized->outgoing[0] != nil)
			generalized->outgoing[0]->wildcard = 1;

		/* Copy rest */
		for(int i = 1; i < pat->noutgoing; i++){
			generalized->outgoing[i] = atomtopattern(nil);
			if(generalized->outgoing[i] != nil)
				generalized->outgoing[i]->wildcard = 1;
		}

		gmatches = countmatches(pm->as, generalized);
		freepattern(generalized);

		if(gmatches == 0)
			return 1.0;

		conf = (float)matches / gmatches;
	} else {
		/* Simple atom - confidence is 1 if it exists */
		conf = matches > 0 ? 1.0 : 0.0;
	}

	return conf;
}

/* Convert pattern to string for display */
char*
cogpatternstr(Pattern *pat)
{
	static char buf[256];

	if(pat == nil)
		return "nil";

	if(pat->wildcard)
		return "*";

	if(pat->name)
		snprint(buf, sizeof(buf), "Pattern(%d, %s, %d)", pat->type, pat->name, pat->noutgoing);
	else
		snprint(buf, sizeof(buf), "Pattern(%d, %d)", pat->type, pat->noutgoing);

	return buf;
}
