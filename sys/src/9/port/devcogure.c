/*
 * devcogure - URE (Unified Rule Engine) File Interface
 * 
 * Exposes inference rules as files for inspection and control.
 * Integrates with cogfs to provide /mnt/cog/rules/ hierarchy.
 * 
 * Files exposed:
 *   /mnt/cog/rules/<rule-name>/
 *     ctl          - Enable/disable, set weight
 *     definition   - Rule pattern
 *     stats        - Usage statistics
 *     trace        - Execution trace
 * 
 * Rule types:
 *   - deduction/    - Deductive inference rules
 *   - induction/    - Inductive inference rules
 *   - abduction/    - Abductive inference rules
 *   - custom/       - User-defined rules
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

/* Rule types */
enum
{
	RuleDeduction = 1,
	RuleInduction,
	RuleAbduction,
	RuleCustom,
};

/* Rule structure */
typedef struct CogRule CogRule;
struct CogRule
{
	char	name[256];	/* Rule name */
	int	type;		/* Rule type */
	int	enabled;	/* Enable flag */
	float	weight;		/* Rule weight/priority */
	
	/* Statistics */
	ulong	applied;	/* Times applied */
	ulong	succeeded;	/* Successful applications */
	ulong	failed;		/* Failed applications */
	vlong	totaltime;	/* Total execution time (µs) */
	
	/* Pattern (simplified for now) */
	char	pattern[1024];	/* Rule pattern string */
	
	/* Trace */
	char	*trace;		/* Last execution trace */
	int	tracelen;	/* Trace length */
	
	Lock;
};

/* Rule registry */
typedef struct RuleRegistry RuleRegistry;
struct RuleRegistry
{
	CogRule	**rules;	/* Array of rules */
	int	nrules;		/* Number of rules */
	int	maxrules;	/* Max rules */
	QLock;
};

static RuleRegistry *rulereg;

/* Qid types */
enum
{
	Qdir = 0,
	Qrulesdir,
	Qruledir,
	Qctl,
	Qdef,
	Qstats,
	Qtrace,
};

/*
 * Initialize rule registry
 */
void
cogureinit(void)
{
	if(rulereg != nil)
		return;
	
	rulereg = smalloc(sizeof(RuleRegistry));
	rulereg->maxrules = 128;
	rulereg->rules = smalloc(sizeof(CogRule*) * rulereg->maxrules);
	rulereg->nrules = 0;
	
	/* Register default PLN rules */
	cogureadddefaultrules();
}

/*
 * Add a rule to the registry
 */
CogRule*
cogureaddrule(char *name, int type, char *pattern)
{
	CogRule *r;
	
	if(rulereg == nil)
		cogureinit();
	
	qlock(rulereg);
	
	/* Check if rule already exists */
	for(int i = 0; i < rulereg->nrules; i++){
		if(strcmp(rulereg->rules[i]->name, name) == 0){
			qunlock(rulereg);
			return rulereg->rules[i];
		}
	}
	
	/* Allocate new rule */
	if(rulereg->nrules >= rulereg->maxrules){
		qunlock(rulereg);
		return nil;
	}
	
	r = smalloc(sizeof(CogRule));
	strncpy(r->name, name, sizeof r->name - 1);
	r->type = type;
	r->enabled = 1;
	r->weight = 1.0;
	r->applied = 0;
	r->succeeded = 0;
	r->failed = 0;
	r->totaltime = 0;
	
	if(pattern != nil)
		strncpy(r->pattern, pattern, sizeof r->pattern - 1);
	
	r->trace = nil;
	r->tracelen = 0;
	
	rulereg->rules[rulereg->nrules++] = r;
	
	qunlock(rulereg);
	return r;
}

/*
 * Find rule by name
 */
static CogRule*
findrule(char *name)
{
	CogRule *r;
	int i;
	
	if(rulereg == nil)
		return nil;
	
	qlock(rulereg);
	for(i = 0; i < rulereg->nrules; i++){
		r = rulereg->rules[i];
		if(strcmp(r->name, name) == 0){
			qunlock(rulereg);
			return r;
		}
	}
	qunlock(rulereg);
	return nil;
}

/*
 * Add default PLN rules
 */
void
cogureadddefaultrules(void)
{
	/* Deduction rules */
	cogureaddrule("deduction.ModusPonens", RuleDeduction,
		"Implication(A,B) & A => B");
	cogureaddrule("deduction.Syllogism", RuleDeduction,
		"Inheritance(A,B) & Inheritance(B,C) => Inheritance(A,C)");
	cogureaddrule("deduction.Contraposition", RuleDeduction,
		"Implication(A,B) => Implication(Not(B),Not(A))");
	
	/* Induction rules */
	cogureaddrule("induction.Direct", RuleInduction,
		"Evaluation(P,A) & Evaluation(P,B) => Similarity(A,B)");
	cogureaddrule("induction.Abductive", RuleInduction,
		"Implication(A,B) & B => A [weak]");
	
	/* Abduction rules */
	cogureaddrule("abduction.Simple", RuleAbduction,
		"Implication(A,B) & B => A [hypothesis]");
	
	/* Revision rules */
	cogureaddrule("revision.TruthValue", RuleCustom,
		"Atom(A,TV1) & Atom(A,TV2) => Atom(A,Revised(TV1,TV2))");
}

/*
 * Read from rule control file
 */
static long
readctl(CogRule *r, void *va, long n, vlong offset)
{
	char buf[256];
	
	lock(r);
	snprint(buf, sizeof buf, "enabled %d\nweight %.3f\n",
		r->enabled, r->weight);
	unlock(r);
	
	return readstr(offset, va, n, buf);
}

/*
 * Write to rule control file
 */
static long
writectl(CogRule *r, void *va, long n)
{
	char *buf, *fields[8];
	int nfields;
	
	buf = smalloc(n + 1);
	memmove(buf, va, n);
	buf[n] = 0;
	
	nfields = tokenize(buf, fields, nelem(fields));
	if(nfields < 2){
		free(buf);
		error(Ebadarg);
	}
	
	lock(r);
	
	if(strcmp(fields[0], "enable") == 0){
		r->enabled = 1;
	}
	else if(strcmp(fields[0], "disable") == 0){
		r->enabled = 0;
	}
	else if(strcmp(fields[0], "weight") == 0){
		r->weight = atof(fields[1]);
	}
	else if(strcmp(fields[0], "reset") == 0){
		r->applied = 0;
		r->succeeded = 0;
		r->failed = 0;
		r->totaltime = 0;
	}
	
	unlock(r);
	free(buf);
	
	return n;
}

/*
 * Read rule definition
 */
static long
readdef(CogRule *r, void *va, long n, vlong offset)
{
	char buf[2048];
	
	lock(r);
	snprint(buf, sizeof buf,
		"# Rule: %s\n"
		"# Type: %d\n"
		"# Pattern:\n%s\n",
		r->name, r->type, r->pattern);
	unlock(r);
	
	return readstr(offset, va, n, buf);
}

/*
 * Read rule statistics
 */
static long
readstats(CogRule *r, void *va, long n, vlong offset)
{
	char buf[1024];
	float avgtime;
	
	lock(r);
	
	avgtime = r->applied > 0 ? (float)r->totaltime / r->applied : 0.0;
	
	snprint(buf, sizeof buf,
		"name %s\n"
		"type %d\n"
		"enabled %d\n"
		"weight %.3f\n"
		"applied %ld\n"
		"succeeded %ld\n"
		"failed %ld\n"
		"successrate %.2f%%\n"
		"totaltime %lld µs\n"
		"avgtime %.2f µs\n",
		r->name,
		r->type,
		r->enabled,
		r->weight,
		r->applied,
		r->succeeded,
		r->failed,
		r->applied > 0 ? (100.0 * r->succeeded / r->applied) : 0.0,
		r->totaltime,
		avgtime);
	
	unlock(r);
	
	return readstr(offset, va, n, buf);
}

/*
 * Read rule trace
 */
static long
readtrace(CogRule *r, void *va, long n, vlong offset)
{
	char *buf;
	long ret;
	
	lock(r);
	
	if(r->trace == nil || r->tracelen == 0){
		unlock(r);
		return readstr(offset, va, n, "# No trace available\n");
	}
	
	buf = r->trace;
	ret = readstr(offset, va, n, buf);
	
	unlock(r);
	return ret;
}

/*
 * Update rule statistics
 * Called by PLN inference engine
 */
void
cogurestats(char *rulename, int succeeded, vlong exectime)
{
	CogRule *r;
	
	r = findrule(rulename);
	if(r == nil)
		return;
	
	lock(r);
	r->applied++;
	if(succeeded)
		r->succeeded++;
	else
		r->failed++;
	r->totaltime += exectime;
	unlock(r);
}

/*
 * Set rule trace
 * Called by PLN inference engine during execution
 */
void
coguretrace(char *rulename, char *trace, int tracelen)
{
	CogRule *r;
	
	r = findrule(rulename);
	if(r == nil)
		return;
	
	lock(r);
	
	/* Free old trace */
	if(r->trace != nil)
		free(r->trace);
	
	/* Allocate new trace */
	r->trace = smalloc(tracelen + 1);
	memmove(r->trace, trace, tracelen);
	r->trace[tracelen] = 0;
	r->tracelen = tracelen;
	
	unlock(r);
}

/*
 * List all rules
 */
void
cogurelistrules(char *buf, int bufsize)
{
	CogRule *r;
	char *s;
	int i;
	
	if(rulereg == nil){
		snprint(buf, bufsize, "# No rules loaded\n");
		return;
	}
	
	s = buf;
	s += snprint(s, bufsize - (s - buf), "# Rules (%d)\n", rulereg->nrules);
	
	qlock(rulereg);
	for(i = 0; i < rulereg->nrules; i++){
		r = rulereg->rules[i];
		lock(r);
		s += snprint(s, bufsize - (s - buf), "%s %c %.2f %ld/%ld\n",
			r->name,
			r->enabled ? 'E' : 'D',
			r->weight,
			r->succeeded,
			r->applied);
		unlock(r);
	}
	qunlock(rulereg);
}

/*
 * These functions integrate with cogfs.c to provide
 * the /mnt/cog/rules/ hierarchy via 9P
 */
