/*
 * Cognitive Process Management
 * 
 * Extends Plan 9 process model with cognitive capabilities.
 * Every process can think, reason, and learn at the kernel level.
 * 
 * This implements the revolutionary idea that processes are
 * cognitive agents by default, not traditional passive executors.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

/* Cognitive Process Extensions */
typedef struct CogProcExt CogProcExt;
struct CogProcExt {
	ulong	atomid;		/* Process's primary atom ID */
	short	sti;		/* Process short-term importance */
	short	lti;		/* Process long-term importance */
	ulong	inferences;	/* Inferences performed */
	ulong	cogcycles;	/* Cognitive cycles executed */
	int	cogstate;	/* Cognitive state */
	Lock;
};

enum {
	CogIdle = 0,
	CogThinking,
	CogReasoning,
	CogLearning,
	CogWaiting,
};

/* Associate cognitive extension with process */
static CogProcExt**	cogprocs;
static int		ncogprocs;
static int		maxcogprocs;
static Lock		cogproclock;

/* Initialize cognitive process subsystem */
void
cogprocinit(void)
{
	maxcogprocs = conf.nproc;
	cogprocs = malloc(maxcogprocs * sizeof(CogProcExt*));
	if(cogprocs == nil)
		panic("cogprocinit: no memory");
	ncogprocs = 0;
}

/* Allocate cognitive extension for process */
CogProcExt*
cogprocalloc(void)
{
	CogProcExt *ce;

	lock(&cogproclock);
	if(ncogprocs >= maxcogprocs) {
		unlock(&cogproclock);
		return nil;
	}

	ce = malloc(sizeof(CogProcExt));
	if(ce == nil) {
		unlock(&cogproclock);
		return nil;
	}

	ce->atomid = 0;
	ce->sti = 0;
	ce->lti = 0;
	ce->inferences = 0;
	ce->cogcycles = 0;
	ce->cogstate = CogIdle;

	cogprocs[ncogprocs++] = ce;
	unlock(&cogproclock);

	return ce;
}

/* Free cognitive extension */
void
cogprocfree(CogProcExt *ce)
{
	int i;

	if(ce == nil)
		return;

	lock(&cogproclock);
	/* Remove from array */
	for(i = 0; i < ncogprocs; i++) {
		if(cogprocs[i] == ce) {
			cogprocs[i] = nil;
			break;
		}
	}
	free(ce);
	unlock(&cogproclock);
}

/* Cognitive priority calculation */
int
cogpriority(CogProcExt *ce)
{
	int pri;

	if(ce == nil)
		return 0;
	
	/* Priority based on STI and cognitive state */
	lock(ce);
	pri = ce->sti;
	if(ce->cogstate == CogThinking)
		pri += 10;
	else if(ce->cogstate == CogReasoning)
		pri += 20;
	unlock(ce);
	
	return pri;
}

/* Cognitive time slice allocation */
int
cogtimeslice(CogProcExt *ce)
{
	int slice;

	if(ce == nil)
		return HZ/100;	/* Default 10ms */

	lock(ce);
	/* Longer time slices for higher importance processes */
	if(ce->sti > 100)
		slice = HZ/50;		/* 20ms */
	else if(ce->sti > 50)
		slice = HZ/100;		/* 10ms */
	else
		slice = HZ/200;		/* 5ms */
	unlock(ce);

	return slice;
}

/* Enter cognitive thinking state */
void
cogthink(CogProcExt *ce)
{
	if(ce == nil)
		return;

	lock(ce);
	ce->cogstate = CogThinking;
	ce->cogcycles++;
	unlock(ce);
}

/* Perform inference operation */
void
coginfer(CogProcExt *ce)
{
	if(ce == nil)
		return;

	lock(ce);
	ce->cogstate = CogReasoning;
	ce->inferences++;
	ce->cogcycles++;
	unlock(ce);
}

/* Enter learning state */
void
coglearn(CogProcExt *ce)
{
	if(ce == nil)
		return;

	lock(ce);
	ce->cogstate = CogLearning;
	ce->cogcycles++;
	unlock(ce);
}

/* Update process attention values */
void
cogupdate(CogProcExt *ce, short dsti, short dlti)
{
	if(ce == nil)
		return;

	lock(ce);
	ce->sti += dsti;
	ce->lti += dlti;
	
	/* Clamp values */
	if(ce->sti < 0)
		ce->sti = 0;
	if(ce->lti < 0)
		ce->lti = 0;
	if(ce->sti > 32767)
		ce->sti = 32767;
	if(ce->lti > 32767)
		ce->lti = 32767;
	
	unlock(ce);
}

/* Cognitive attention decay */
void
cogdecayprocs(void)
{
	int i;
	CogProcExt *ce;

	lock(&cogproclock);
	for(i = 0; i < ncogprocs; i++) {
		ce = cogprocs[i];
		if(ce == nil)
			continue;
		
		lock(ce);
		/* Decay STI over time */
		if(ce->sti > 0)
			ce->sti--;
		unlock(ce);
	}
	unlock(&cogproclock);
}

/* Get cognitive statistics */
void
cogprocstats(ulong *totalinf, ulong *totalcyc, int *nprocs)
{
	int i;
	CogProcExt *ce;
	ulong inf, cyc;

	inf = 0;
	cyc = 0;

	lock(&cogproclock);
	for(i = 0; i < ncogprocs; i++) {
		ce = cogprocs[i];
		if(ce == nil)
			continue;
		
		lock(ce);
		inf += ce->inferences;
		cyc += ce->cogcycles;
		unlock(ce);
	}
	*nprocs = ncogprocs;
	unlock(&cogproclock);

	*totalinf = inf;
	*totalcyc = cyc;
}

/* Find most important cognitive process */
CogProcExt*
cogfindmax(void)
{
	int i, maxsti;
	CogProcExt *ce, *best;

	maxsti = -1;
	best = nil;

	lock(&cogproclock);
	for(i = 0; i < ncogprocs; i++) {
		ce = cogprocs[i];
		if(ce == nil)
			continue;
		
		lock(ce);
		if(ce->sti > maxsti) {
			maxsti = ce->sti;
			best = ce;
		}
		unlock(ce);
	}
	unlock(&cogproclock);

	return best;
}

/* Spread attention between processes */
void
cogspreadattention(CogProcExt *source, CogProcExt *target, short amount)
{
	if(source == nil || target == nil)
		return;

	lock(source);
	lock(target);
	
	if(source->sti >= amount) {
		source->sti -= amount;
		target->sti += amount;
	}
	
	unlock(target);
	unlock(source);
}

/* Make process more important */
void
cogboost(CogProcExt *ce, short boost)
{
	if(ce == nil)
		return;

	lock(ce);
	ce->sti += boost;
	ce->lti += boost / 10;
	unlock(ce);
}

/* Cognitive sleep - Wait with attention decay */
void
cogsleep(CogProcExt *ce, int ms)
{
	if(ce == nil)
		return;

	lock(ce);
	ce->cogstate = CogWaiting;
	/* STI decays faster while waiting */
	ce->sti = (ce->sti * 9) / 10;
	unlock(ce);
}

/* Resume cognitive processing */
void
cogresume(CogProcExt *ce)
{
	if(ce == nil)
		return;

	lock(ce);
	ce->cogstate = CogIdle;
	unlock(ce);
}
