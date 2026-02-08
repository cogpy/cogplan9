/*
 * devcoginf - Inference Debugging Interface
 * 
 * Provides file-based interface for debugging PLN inference.
 * Exposes inference state, traces, and breakpoints.
 * 
 * Files exposed:
 *   /mnt/cog/inference/
 *     active       - List of running inferences
 *     trace        - Inference execution trace
 *     breakpoints  - Breakpoint management
 *     ctl          - Control interface
 *     stats        - Inference statistics
 *     <id>/        - Per-inference state
 *       state      - Current state
 *       steps      - Inference steps
 *       result     - Inference result
 *       ctl        - Control this inference
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

/* Inference states */
enum
{
	InfInit = 0,
	InfRunning,
	InfBlocked,
	InfComplete,
	InfFailed,
};

static char *infstates[] = {
	"init",
	"running",
	"blocked",
	"complete",
	"failed",
};

/* Inference step */
typedef struct InfStep InfStep;
struct InfStep
{
	int	stepnum;	/* Step number */
	char	rule[256];	/* Rule applied */
	ulong	atomid;		/* Atom produced */
	vlong	timestamp;	/* Timestamp (µs) */
	char	*detail;	/* Step details */
};

/* Inference session */
typedef struct InfSession InfSession;
struct InfSession
{
	int	id;		/* Inference ID */
	int	state;		/* Current state */
	ulong	goalatom;	/* Goal atom ID */
	
	/* Steps */
	InfStep	*steps;		/* Inference steps */
	int	nsteps;		/* Number of steps */
	int	maxsteps;	/* Max steps */
	
	/* Result */
	ulong	resultatom;	/* Result atom ID */
	float	confidence;	/* Result confidence */
	
	/* Control */
	int	paused;		/* Paused flag */
	int	step;		/* Single-step mode */
	
	/* Statistics */
	vlong	starttime;	/* Start time */
	vlong	endtime;	/* End time */
	ulong	rulesapplied;	/* Rules applied */
	
	Lock;
};

/* Breakpoint */
typedef struct Breakpoint Breakpoint;
struct Breakpoint
{
	int	enabled;	/* Enable flag */
	char	rule[256];	/* Rule name (nil = any) */
	ulong	atomid;		/* Atom ID (0 = any) */
	int	hitcount;	/* Hit count */
};

/* Global inference state */
typedef struct InfDebug InfDebug;
struct InfDebug
{
	InfSession **sessions;	/* Active sessions */
	int	nsessions;	/* Number of sessions */
	int	maxsessions;	/* Max sessions */
	
	Breakpoint *breakpoints; /* Breakpoints */
	int	nbreakpoints;	/* Number of breakpoints */
	int	maxbreakpoints;	/* Max breakpoints */
	
	/* Global trace */
	char	*trace;		/* Trace buffer */
	int	tracelen;	/* Trace length */
	int	tracemax;	/* Trace max size */
	int	traceenabled;	/* Trace enabled */
	
	/* Statistics */
	ulong	totalinferences;
	ulong	activeinferences;
	
	QLock;
};

static InfDebug *infdbg;

/*
 * Initialize inference debugging
 */
void
coginfdebuginit(void)
{
	if(infdbg != nil)
		return;
	
	infdbg = smalloc(sizeof(InfDebug));
	infdbg->maxsessions = 64;
	infdbg->sessions = smalloc(sizeof(InfSession*) * infdbg->maxsessions);
	infdbg->nsessions = 0;
	
	infdbg->maxbreakpoints = 32;
	infdbg->breakpoints = smalloc(sizeof(Breakpoint) * infdbg->maxbreakpoints);
	infdbg->nbreakpoints = 0;
	
	infdbg->tracemax = 65536;  /* 64KB trace buffer */
	infdbg->trace = smalloc(infdbg->tracemax);
	infdbg->tracelen = 0;
	infdbg->traceenabled = 1;
	
	infdbg->totalinferences = 0;
	infdbg->activeinferences = 0;
}

/*
 * Start new inference session
 */
int
coginfstart(ulong goalatom)
{
	InfSession *s;
	int id;
	
	if(infdbg == nil)
		coginfdebuginit();
	
	qlock(infdbg);
	
	if(infdbg->nsessions >= infdbg->maxsessions){
		qunlock(infdbg);
		return -1;
	}
	
	/* Allocate session */
	s = smalloc(sizeof(InfSession));
	id = infdbg->totalinferences++;
	s->id = id;
	s->state = InfInit;
	s->goalatom = goalatom;
	
	s->maxsteps = 1024;
	s->steps = smalloc(sizeof(InfStep) * s->maxsteps);
	s->nsteps = 0;
	
	s->resultatom = 0;
	s->confidence = 0.0;
	s->paused = 0;
	s->step = 0;
	s->starttime = nsec() / 1000;  /* Convert to µs */
	s->endtime = 0;
	s->rulesapplied = 0;
	
	infdbg->sessions[infdbg->nsessions++] = s;
	infdbg->activeinferences++;
	
	qunlock(infdbg);
	
	coginftrace("Inference %d started for goal atom %ld\n", id, goalatom);
	
	return id;
}

/*
 * Complete inference session
 */
void
coginfcomplete(int infid, ulong resultatom, float confidence)
{
	InfSession *s;
	int i;
	
	if(infdbg == nil)
		return;
	
	qlock(infdbg);
	
	for(i = 0; i < infdbg->nsessions; i++){
		s = infdbg->sessions[i];
		if(s->id == infid){
			lock(s);
			s->state = InfComplete;
			s->resultatom = resultatom;
			s->confidence = confidence;
			s->endtime = nsec() / 1000;
			unlock(s);
			
			infdbg->activeinferences--;
			break;
		}
	}
	
	qunlock(infdbg);
	
	coginftrace("Inference %d completed: atom %ld (%.3f)\n",
		infid, resultatom, confidence);
}

/*
 * Add inference step
 */
void
coginfstep(int infid, char *rule, ulong atomid, char *detail)
{
	InfSession *s;
	InfStep *step;
	int i;
	
	if(infdbg == nil)
		return;
	
	qlock(infdbg);
	
	/* Find session */
	s = nil;
	for(i = 0; i < infdbg->nsessions; i++){
		if(infdbg->sessions[i]->id == infid){
			s = infdbg->sessions[i];
			break;
		}
	}
	
	if(s == nil){
		qunlock(infdbg);
		return;
	}
	
	lock(s);
	
	/* Add step */
	if(s->nsteps < s->maxsteps){
		step = &s->steps[s->nsteps];
		step->stepnum = s->nsteps;
		strncpy(step->rule, rule, sizeof step->rule - 1);
		step->atomid = atomid;
		step->timestamp = nsec() / 1000;
		
		if(detail != nil){
			step->detail = smalloc(strlen(detail) + 1);
			strcpy(step->detail, detail);
		}
		else{
			step->detail = nil;
		}
		
		s->nsteps++;
		s->rulesapplied++;
	}
	
	/* Check breakpoints */
	for(i = 0; i < infdbg->nbreakpoints; i++){
		Breakpoint *bp = &infdbg->breakpoints[i];
		if(!bp->enabled)
			continue;
		
		/* Check rule match */
		if(bp->rule[0] != 0 && strcmp(bp->rule, rule) != 0)
			continue;
		
		/* Check atom match */
		if(bp->atomid != 0 && bp->atomid != atomid)
			continue;
		
		/* Breakpoint hit */
		bp->hitcount++;
		s->paused = 1;
		s->state = InfBlocked;
		coginftrace("Breakpoint %d hit in inference %d\n", i, infid);
	}
	
	unlock(s);
	qunlock(infdbg);
	
	coginftrace("Inference %d: step %d: %s -> %ld\n",
		infid, s->nsteps - 1, rule, atomid);
}

/*
 * Add trace entry
 */
void
coginftrace(char *fmt, ...)
{
	va_list arg;
	char buf[1024];
	int n;
	
	if(infdbg == nil || !infdbg->traceenabled)
		return;
	
	va_start(arg, fmt);
	n = vsnprint(buf, sizeof buf, fmt, arg);
	va_end(arg);
	
	qlock(infdbg);
	
	/* Append to trace buffer (circular) */
	if(infdbg->tracelen + n >= infdbg->tracemax){
		/* Wrap around */
		infdbg->tracelen = 0;
	}
	
	memmove(infdbg->trace + infdbg->tracelen, buf, n);
	infdbg->tracelen += n;
	infdbg->trace[infdbg->tracelen] = 0;
	
	qunlock(infdbg);
}

/*
 * Read active inferences list
 */
void
coginfreaddctive(char *buf, int bufsize)
{
	InfSession *s;
	char *p;
	int i;
	
	if(infdbg == nil){
		snprint(buf, bufsize, "# No inferences\n");
		return;
	}
	
	p = buf;
	p += snprint(p, bufsize - (p - buf),
		"# Active inferences: %ld\n", infdbg->activeinferences);
	p += snprint(p, bufsize - (p - buf),
		"# id state goal steps rules time\n");
	
	qlock(infdbg);
	for(i = 0; i < infdbg->nsessions; i++){
		s = infdbg->sessions[i];
		lock(s);
		
		vlong elapsed = (s->endtime ? s->endtime : nsec() / 1000) - s->starttime;
		
		p += snprint(p, bufsize - (p - buf), "%d %s %ld %d %ld %lld\n",
			s->id,
			infstates[s->state],
			s->goalatom,
			s->nsteps,
			s->rulesapplied,
			elapsed);
		
		unlock(s);
	}
	qunlock(infdbg);
}

/*
 * Read trace buffer
 */
long
coginfreadtrace(void *va, long n, vlong offset)
{
	if(infdbg == nil || infdbg->trace == nil)
		return readstr(offset, va, n, "# Tracing disabled\n");
	
	qlock(infdbg);
	n = readstr(offset, va, n, infdbg->trace);
	qunlock(infdbg);
	
	return n;
}

/*
 * Read inference statistics
 */
void
coginfreadstats(char *buf, int bufsize)
{
	if(infdbg == nil){
		snprint(buf, bufsize, "# Debugging not initialized\n");
		return;
	}
	
	qlock(infdbg);
	snprint(buf, bufsize,
		"total %ld\n"
		"active %ld\n"
		"sessions %d\n"
		"breakpoints %d\n"
		"tracing %s\n"
		"tracelen %d\n",
		infdbg->totalinferences,
		infdbg->activeinferences,
		infdbg->nsessions,
		infdbg->nbreakpoints,
		infdbg->traceenabled ? "on" : "off",
		infdbg->tracelen);
	qunlock(infdbg);
}

/*
 * Add breakpoint
 */
int
coginfaddbreakpoint(char *rule, ulong atomid)
{
	Breakpoint *bp;
	int id;
	
	if(infdbg == nil)
		coginfdebuginit();
	
	qlock(infdbg);
	
	if(infdbg->nbreakpoints >= infdbg->maxbreakpoints){
		qunlock(infdbg);
		return -1;
	}
	
	id = infdbg->nbreakpoints;
	bp = &infdbg->breakpoints[id];
	bp->enabled = 1;
	
	if(rule != nil)
		strncpy(bp->rule, rule, sizeof bp->rule - 1);
	else
		bp->rule[0] = 0;
	
	bp->atomid = atomid;
	bp->hitcount = 0;
	
	infdbg->nbreakpoints++;
	qunlock(infdbg);
	
	return id;
}

/*
 * List breakpoints
 */
void
coginflistbreakpoints(char *buf, int bufsize)
{
	Breakpoint *bp;
	char *p;
	int i;
	
	if(infdbg == nil){
		snprint(buf, bufsize, "# No breakpoints\n");
		return;
	}
	
	p = buf;
	p += snprint(p, bufsize - (p - buf), "# Breakpoints (%d)\n",
		infdbg->nbreakpoints);
	p += snprint(p, bufsize - (p - buf), "# id enabled rule atom hits\n");
	
	qlock(infdbg);
	for(i = 0; i < infdbg->nbreakpoints; i++){
		bp = &infdbg->breakpoints[i];
		p += snprint(p, bufsize - (p - buf), "%d %c %s %ld %d\n",
			i,
			bp->enabled ? 'E' : 'D',
			bp->rule[0] ? bp->rule : "*",
			bp->atomid,
			bp->hitcount);
	}
	qunlock(infdbg);
}

/*
 * Control interface commands
 */
void
coginfctl(char *cmd)
{
	char *fields[8];
	int nfields;
	
	if(infdbg == nil)
		coginfdebuginit();
	
	nfields = tokenize(cmd, fields, nelem(fields));
	if(nfields < 1)
		return;
	
	qlock(infdbg);
	
	if(strcmp(fields[0], "trace") == 0){
		if(nfields >= 2){
			if(strcmp(fields[1], "on") == 0)
				infdbg->traceenabled = 1;
			else if(strcmp(fields[1], "off") == 0)
				infdbg->traceenabled = 0;
		}
	}
	else if(strcmp(fields[0], "cleartrace") == 0){
		infdbg->tracelen = 0;
		if(infdbg->trace != nil)
			infdbg->trace[0] = 0;
	}
	else if(strcmp(fields[0], "reset") == 0){
		/* Clear all sessions */
		infdbg->nsessions = 0;
		infdbg->activeinferences = 0;
	}
	
	qunlock(infdbg);
}
