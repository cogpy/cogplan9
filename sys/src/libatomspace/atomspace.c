/*
 * AtomSpace implementation for Plan9Cog
 * Hypergraph knowledge representation system
 */

#include <u.h>
#include <libc.h>
#include <plan9cog/atomspace.h>

static ulong nextatomid = 1;

/* Hash table for O(1) atom lookup */
#define HASH_SIZE 1024

typedef struct HashEntry HashEntry;
struct HashEntry {
	ulong id;
	Atom *atom;
	HashEntry *next;
};

typedef struct AtomHash AtomHash;
struct AtomHash {
	HashEntry *buckets[HASH_SIZE];
};

static AtomHash *globalhash;

/* Hash function for atom IDs */
static int
hashid(ulong id)
{
	return (int)(id % HASH_SIZE);
}

/* Initialize hash table */
static AtomHash*
hashinit(void)
{
	AtomHash *h;
	int i;

	h = mallocz(sizeof(AtomHash), 1);
	if(h == nil)
		return nil;

	for(i = 0; i < HASH_SIZE; i++)
		h->buckets[i] = nil;

	return h;
}

/* Insert atom into hash table */
static void
hashinsert(AtomHash *h, Atom *a)
{
	HashEntry *e;
	int bucket;

	if(h == nil || a == nil)
		return;

	bucket = hashid(a->id);

	e = mallocz(sizeof(HashEntry), 1);
	if(e == nil)
		return;

	e->id = a->id;
	e->atom = a;
	e->next = h->buckets[bucket];
	h->buckets[bucket] = e;
}

/* Find atom in hash table - O(1) average case */
static Atom*
hashfind(AtomHash *h, ulong id)
{
	HashEntry *e;
	int bucket;

	if(h == nil)
		return nil;

	bucket = hashid(id);

	for(e = h->buckets[bucket]; e != nil; e = e->next){
		if(e->id == id)
			return e->atom;
	}

	return nil;
}

/* Remove atom from hash table */
static void
hashremove(AtomHash *h, ulong id)
{
	HashEntry *e, *prev;
	int bucket;

	if(h == nil)
		return;

	bucket = hashid(id);
	prev = nil;

	for(e = h->buckets[bucket]; e != nil; prev = e, e = e->next){
		if(e->id == id){
			if(prev == nil)
				h->buckets[bucket] = e->next;
			else
				prev->next = e->next;
			free(e);
			return;
		}
	}
}

/* Free hash table */
static void
hashfree(AtomHash *h)
{
	HashEntry *e, *next;
	int i;

	if(h == nil)
		return;

	for(i = 0; i < HASH_SIZE; i++){
		for(e = h->buckets[i]; e != nil; e = next){
			next = e->next;
			free(e);
		}
	}

	free(h);
}

AtomSpace*
atomspacecreate(void)
{
	AtomSpace *as;

	as = mallocz(sizeof(AtomSpace), 1);
	if(as == nil)
		return nil;

	as->maxatoms = 1024;
	as->atoms = mallocz(sizeof(Atom*) * as->maxatoms, 1);
	if(as->atoms == nil){
		free(as);
		return nil;
	}

	as->natoms = 0;

	/* Initialize hash table */
	if(globalhash == nil)
		globalhash = hashinit();

	return as;
}

void
atomspacefree(AtomSpace *as)
{
	int i;

	if(as == nil)
		return;

	lock(as);
	for(i = 0; i < as->natoms; i++){
		if(as->atoms[i]){
			/* Remove from hash table */
			if(globalhash != nil)
				hashremove(globalhash, as->atoms[i]->id);

			free(as->atoms[i]->name);
			free(as->atoms[i]->outgoing);
			free(as->atoms[i]);
		}
	}
	unlock(as);

	free(as->atoms);
	free(as);
}

static Atom*
atomalloc(AtomSpace *as)
{
	Atom *a;
	Atom **newatoms;

	a = mallocz(sizeof(Atom), 1);
	if(a == nil)
		return nil;

	lock(as);

	/* Expand atom array if needed */
	if(as->natoms >= as->maxatoms){
		as->maxatoms *= 2;
		newatoms = realloc(as->atoms, sizeof(Atom*) * as->maxatoms);
		if(newatoms == nil){
			unlock(as);
			free(a);
			return nil;
		}
		as->atoms = newatoms;
	}

	a->id = nextatomid++;
	as->atoms[as->natoms++] = a;

	/* Insert into hash table for O(1) lookup */
	if(globalhash != nil)
		hashinsert(globalhash, a);

	unlock(as);
	return a;
}

Atom*
atomcreate(AtomSpace *as, int type, char *name)
{
	Atom *a;
	
	a = atomalloc(as);
	if(a == nil)
		return nil;
	
	a->type = type;
	a->name = name ? strdup(name) : nil;
	a->outgoing = nil;
	a->noutgoing = 0;
	
	/* Default truth value */
	a->tv.strength = 0.5;
	a->tv.confidence = 0.0;
	a->tv.count = 0;
	
	/* Default attention value */
	a->av.sti = 0;
	a->av.lti = 0;
	a->av.vlti = 0;
	
	return a;
}

Atom*
linkcreate(AtomSpace *as, int type, Atom **outgoing, int n)
{
	Atom *a;
	int i;
	
	a = atomalloc(as);
	if(a == nil)
		return nil;
	
	a->type = type;
	a->name = nil;
	a->noutgoing = n;
	
	if(n > 0){
		a->outgoing = mallocz(sizeof(Atom*) * n, 1);
		if(a->outgoing == nil){
			free(a);
			return nil;
		}
		for(i = 0; i < n; i++)
			a->outgoing[i] = outgoing[i];
	} else {
		a->outgoing = nil;
	}
	
	/* Default truth value */
	a->tv.strength = 0.5;
	a->tv.confidence = 0.0;
	a->tv.count = 0;
	
	/* Default attention value */
	a->av.sti = 0;
	a->av.lti = 0;
	a->av.vlti = 0;
	
	return a;
}

Atom*
atomfind(AtomSpace *as, ulong id)
{
	Atom *a;

	/* Use hash table for O(1) lookup */
	if(globalhash != nil){
		a = hashfind(globalhash, id);
		if(a != nil)
			return a;
	}

	/* Fallback to linear search */
	lock(as);
	for(int i = 0; i < as->natoms; i++){
		a = as->atoms[i];
		if(a && a->id == id){
			unlock(as);
			return a;
		}
	}
	unlock(as);
	return nil;
}

int
atomdelete(AtomSpace *as, ulong id)
{
	int i;
	Atom *a;

	/* Remove from hash table */
	if(globalhash != nil)
		hashremove(globalhash, id);

	lock(as);
	for(i = 0; i < as->natoms; i++){
		a = as->atoms[i];
		if(a && a->id == id){
			free(a->name);
			free(a->outgoing);
			free(a);
			as->atoms[i] = nil;
			unlock(as);
			return 0;
		}
	}
	unlock(as);
	return -1;
}

TruthValue
atomgettruth(Atom *a)
{
	return a->tv;
}

void
atomsettruth(Atom *a, TruthValue tv)
{
	a->tv = tv;
}

AttentionValue
atomgetattention(Atom *a)
{
	return a->av;
}

void
atomsetattention(Atom *a, AttentionValue av)
{
	a->av = av;
}

Atom**
atomquery(AtomSpace *as, AtomPredicate pred, void *arg, int *n)
{
	Atom **results;
	int i, count, maxresults;
	
	maxresults = 128;
	results = mallocz(sizeof(Atom*) * maxresults, 1);
	if(results == nil){
		*n = 0;
		return nil;
	}
	
	count = 0;
	lock(as);
	for(i = 0; i < as->natoms; i++){
		if(as->atoms[i] && pred(as->atoms[i], arg)){
			if(count >= maxresults){
				maxresults *= 2;
				results = realloc(results, sizeof(Atom*) * maxresults);
			}
			results[count++] = as->atoms[i];
		}
	}
	unlock(as);
	
	*n = count;
	return results;
}

Atom**
atomgetincoming(AtomSpace *as, Atom *a, int *n)
{
	Atom **results;
	Atom *link;
	int i, j, count, maxresults;
	
	maxresults = 128;
	results = mallocz(sizeof(Atom*) * maxresults, 1);
	if(results == nil){
		*n = 0;
		return nil;
	}
	
	count = 0;
	lock(as);
	for(i = 0; i < as->natoms; i++){
		link = as->atoms[i];
		if(link == nil || link->outgoing == nil)
			continue;
		
		for(j = 0; j < link->noutgoing; j++){
			if(link->outgoing[j] == a){
				if(count >= maxresults){
					maxresults *= 2;
					results = realloc(results, sizeof(Atom*) * maxresults);
				}
				results[count++] = link;
				break;
			}
		}
	}
	unlock(as);
	
	*n = count;
	return results;
}

/* IPC operations */
int
cogsend(int fd, CogMsg *msg)
{
	return write(fd, msg, sizeof(CogMsg));
}

int
cogrecv(int fd, CogMsg *msg)
{
	return read(fd, msg, sizeof(CogMsg));
}
