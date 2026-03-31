/*
 * Plan9Cog main library implementation
 */

#include <u.h>
#include <libc.h>
#include <plan9cog.h>

static Plan9Cog *globalp9c;

Plan9Cog*
plan9coginit(void)
{
	Plan9Cog *p9c;
	
	p9c = mallocz(sizeof(Plan9Cog), 1);
	if(p9c == nil)
		return nil;
	
	/* Initialize AtomSpace */
	p9c->atomspace = atomspacecreate();
	if(p9c->atomspace == nil){
		free(p9c);
		return nil;
	}
	
	/* Initialize PLN */
	p9c->pln = plninit(p9c->atomspace);
	if(p9c->pln == nil){
		atomspacefree(p9c->atomspace);
		free(p9c);
		return nil;
	}
	
	/* Initialize cognitive memory (stub) */
	p9c->cogmem = nil;

	p9c->initialized = 1;
	p9c->starttime = nsec();	/* user-space library start time */
	globalp9c = p9c;
	
	return p9c;
}

void
plan9cogfree(Plan9Cog *p9c)
{
	if(p9c == nil)
		return;
	
	lock(p9c);
	
	if(p9c->pln)
		plnfree(p9c->pln);
	
	if(p9c->atomspace)
		atomspacefree(p9c->atomspace);
	
	if(p9c->cogmem)
		cogmemfree(p9c->cogmem);
	
	unlock(p9c);
	
	if(globalp9c == p9c)
		globalp9c = nil;
	
	free(p9c);
}

Plan9Cog*
plan9coginstance(void)
{
	if(globalp9c == nil)
		globalp9c = plan9coginit();
	return globalp9c;
}

void
coginfo(Plan9Cog *p9c, CogInfo *info)
{
	PlnStats ps;

	if(p9c == nil || info == nil)
		return;

	memset(info, 0, sizeof(CogInfo));
	strcpy(info->version, "Plan9Cog 0.2");

	/* Uptime in seconds since plan9coginit() */
	if(p9c->starttime > 0)
		info->uptime = (ulong)((nsec() - p9c->starttime) / 1000000000LL);

	info->natoms = p9c->atomspace ? p9c->atomspace->natoms : 0;
	info->nrules = p9c->pln ? p9c->pln->nrules : 0;

	/* Collect live inference statistics from PLN */
	if(p9c->pln){
		plnstats(p9c->pln, &ps);
		info->ninferences = ps.inferences;
	}

	/* Rough cognitive memory estimate: atoms * per-atom overhead */
	info->cogmem = info->natoms * sizeof(ulong) * 8;
}

void
cogprint(char *fmt, ...)
{
	va_list arg;
	
	va_start(arg, fmt);
	vfprint(1, fmt, arg);
	va_end(arg);
}

void
cogdebug(int level, char *fmt, ...)
{
	va_list arg;
	char buf[1024];
	
	if(level > 0){
		va_start(arg, fmt);
		vsnprint(buf, sizeof(buf), fmt, arg);
		va_end(arg);
		fprint(2, "cogdebug[%d]: %s\n", level, buf);
	}
}

char*
cogatomstr(Atom *a)
{
	static char buf[256];
	
	if(a == nil)
		return "nil";
	
	if(a->name)
		snprint(buf, sizeof(buf), "Atom(%ld, %d, %s)", a->id, a->type, a->name);
	else
		snprint(buf, sizeof(buf), "Link(%ld, %d, %d)", a->id, a->type, a->noutgoing);
	
	return buf;
}

char*
cogtvstr(TruthValue tv)
{
	static char buf[128];
	
	snprint(buf, sizeof(buf), "TV(%.3f, %.3f, %ld)",
		tv.strength, tv.confidence, tv.count);
	
	return buf;
}

/* Cognitive Grip implementation */
CogGrip*
coggrip(int type, void *object)
{
	CogGrip *grip;
	
	grip = mallocz(sizeof(CogGrip), 1);
	if(grip == nil)
		return nil;
	
	grip->type = type;
	grip->object = object;
	grip->refcount = 1;
	
	return grip;
}

void
cogrelease(CogGrip *grip)
{
	if(grip == nil)
		return;
	
	lock(grip);
	grip->refcount--;
	if(grip->refcount <= 0){
		unlock(grip);
		free(grip);
		return;
	}
	unlock(grip);
}

void*
cogobject(CogGrip *grip)
{
	return grip ? grip->object : nil;
}

CogGrip*
cogretain(CogGrip *grip)
{
	if(grip == nil)
		return nil;
	
	lock(grip);
	grip->refcount++;
	unlock(grip);
	
	return grip;
}
