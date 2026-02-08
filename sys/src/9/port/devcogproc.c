/*
 * Cognitive Process Extensions - devcogproc
 * 
 * Per-process cognitive namespace extensions for /proc
 * Exposes cognitive state as files under /proc/pid/cog/
 * 
 * Files exposed:
 *   /proc/<pid>/cog/atoms     - Process's relevant atoms
 *   /proc/<pid>/cog/focus     - Attentional focus
 *   /proc/<pid>/cog/goals     - OpenPsi goals
 *   /proc/<pid>/cog/beliefs   - Belief state
 *   /proc/<pid>/cog/ctl       - Control allocation
 *   /proc/<pid>/cog/stats     - Statistics
 * 
 * This leverages Plan 9's per-process namespace model for cognitive state.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

enum
{
	/* Qid types for /proc/pid/cog/ */
	Qcogdir = 0,
	Qatoms,
	Qfocus,
	Qgoals,
	Qbeliefs,
	Qctl,
	Qstats,
};

static Dirtab cogprocdir[] =
{
	"atoms",	{Qatoms},	0,	0444,
	"focus",	{Qfocus},	0,	0644,
	"goals",	{Qgoals},	0,	0644,
	"beliefs",	{Qbeliefs},	0,	0644,
	"ctl",		{Qctl},		0,	0220,
	"stats",	{Qstats},	0,	0444,
};

/*
 * Per-process cognitive state
 * Attached to Proc->cogext
 */
typedef struct CogProcState CogProcState;
struct CogProcState
{
	int	pid;		/* Process ID */
	
	/* Attention focus */
	ulong	focusatom;	/* Current focus atom ID */
	short	focussti;	/* Focus STI threshold */
	
	/* Goals (OpenPsi) */
	ulong	*goals;		/* Goal atom IDs */
	int	ngoals;		/* Number of goals */
	int	maxgoals;	/* Max goals */
	
	/* Beliefs */
	ulong	*beliefs;	/* Belief atom IDs */
	int	nbeliefs;	/* Number of beliefs */
	int	maxbeliefs;	/* Max beliefs */
	
	/* Statistics */
	ulong	inferences;	/* Inferences performed */
	ulong	focuses;	/* Focus shifts */
	ulong	goalupdates;	/* Goal updates */
	
	Lock;
};

/* Global state */
static QLock	cogproclk;

/*
 * Get or create cognitive state for process
 */
static CogProcState*
cogprocstate(Proc *p)
{
	CogProcState *cs;
	
	if(p == nil)
		return nil;
	
	qlock(&cogproclk);
	
	/* Check if already allocated */
	if(p->cogext != nil){
		cs = p->cogext;
		qunlock(&cogproclk);
		return cs;
	}
	
	/* Allocate new state */
	cs = smalloc(sizeof(CogProcState));
	cs->pid = p->pid;
	cs->focusatom = 0;
	cs->focussti = 0;
	cs->maxgoals = 16;
	cs->goals = smalloc(sizeof(ulong) * cs->maxgoals);
	cs->ngoals = 0;
	cs->maxbeliefs = 32;
	cs->beliefs = smalloc(sizeof(ulong) * cs->maxbeliefs);
	cs->nbeliefs = 0;
	cs->inferences = 0;
	cs->focuses = 0;
	cs->goalupdates = 0;
	
	p->cogext = cs;
	qunlock(&cogproclk);
	
	return cs;
}

/*
 * Free cognitive state for process
 * Called from pexit() via cogprocextfree()
 */
void
cogprocstatefree(Proc *p)
{
	CogProcState *cs;
	
	if(p == nil || p->cogext == nil)
		return;
	
	qlock(&cogproclk);
	cs = p->cogext;
	p->cogext = nil;
	
	if(cs != nil){
		if(cs->goals != nil)
			free(cs->goals);
		if(cs->beliefs != nil)
			free(cs->beliefs);
		free(cs);
	}
	
	qunlock(&cogproclk);
}

/*
 * Generate directory listing for /proc/pid/cog/
 */
static int
cogprocgen(Chan *c, char *name, Dirtab *tab, int ntab, int i, Dir *dp)
{
	Qid qid;
	
	if(i == DEVDOTDOT){
		devdir(c, c->qid, "#Ψ", 0, eve, 0555, dp);
		return 1;
	}
	
	if(i >= nelem(cogprocdir))
		return -1;
	
	tab = &cogprocdir[i];
	qid = tab->qid;
	qid.vers = c->qid.vers;
	qid.path = c->qid.path | (i << 8);
	
	devdir(c, qid, tab->name, tab->length, eve, tab->perm, dp);
	return 1;
}

/*
 * Walk into /proc/pid/cog/ directory
 */
static Walkqid*
cogprocwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, cogprocdir, nelem(cogprocdir), cogprocgen);
}

/*
 * Stat /proc/pid/cog/ files
 */
static int
cogprocstat(Chan *c, uchar *db, int n)
{
	return devstat(c, db, n, cogprocdir, nelem(cogprocdir), cogprocgen);
}

/*
 * Open /proc/pid/cog/ files
 */
static Chan*
cogprocopen(Chan *c, int omode)
{
	return devopen(c, omode, cogprocdir, nelem(cogprocdir), cogprocgen);
}

/*
 * Close /proc/pid/cog/ files
 */
static void
cogprocclose(Chan *c)
{
	USED(c);
}

/*
 * Read from /proc/pid/cog/ files
 */
static long
cogprocread(Chan *c, void *va, long n, vlong offset)
{
	Proc *p;
	CogProcState *cs;
	char *buf, *s;
	int i, qid;
	
	/* Get process from channel */
	p = up;  /* For now, only allow reading own process */
	if(p == nil)
		error(Enonexist);
	
	cs = cogprocstate(p);
	if(cs == nil)
		error(Enomem);
	
	qid = QID(c->qid);
	
	switch(qid){
	case Qatoms:
		/* Return list of relevant atoms for this process */
		buf = smalloc(8192);
		s = buf;
		s += snprint(s, 8192 - (s - buf), "# Relevant atoms for process %d\n", p->pid);
		
		/* Focus atom */
		if(cs->focusatom != 0)
			s += snprint(s, 8192 - (s - buf), "focus %ld\n", cs->focusatom);
		
		/* Goal atoms */
		lock(cs);
		for(i = 0; i < cs->ngoals; i++)
			s += snprint(s, 8192 - (s - buf), "goal %ld\n", cs->goals[i]);
		
		/* Belief atoms */
		for(i = 0; i < cs->nbeliefs; i++)
			s += snprint(s, 8192 - (s - buf), "belief %ld\n", cs->beliefs[i]);
		unlock(cs);
		
		n = readstr(offset, va, n, buf);
		free(buf);
		return n;
		
	case Qfocus:
		/* Return current attentional focus */
		buf = smalloc(256);
		lock(cs);
		snprint(buf, 256, "atom %ld sti %d\n", cs->focusatom, cs->focussti);
		unlock(cs);
		n = readstr(offset, va, n, buf);
		free(buf);
		return n;
		
	case Qgoals:
		/* Return current goals */
		buf = smalloc(4096);
		s = buf;
		s += snprint(s, 4096 - (s - buf), "# OpenPsi goals for process %d\n", p->pid);
		
		lock(cs);
		for(i = 0; i < cs->ngoals; i++)
			s += snprint(s, 4096 - (s - buf), "%ld\n", cs->goals[i]);
		unlock(cs);
		
		n = readstr(offset, va, n, buf);
		free(buf);
		return n;
		
	case Qbeliefs:
		/* Return current beliefs */
		buf = smalloc(4096);
		s = buf;
		s += snprint(s, 4096 - (s - buf), "# Beliefs for process %d\n", p->pid);
		
		lock(cs);
		for(i = 0; i < cs->nbeliefs; i++)
			s += snprint(s, 4096 - (s - buf), "%ld\n", cs->beliefs[i]);
		unlock(cs);
		
		n = readstr(offset, va, n, buf);
		free(buf);
		return n;
		
	case Qstats:
		/* Return cognitive statistics */
		buf = smalloc(1024);
		lock(cs);
		snprint(buf, 1024, 
			"pid %d\n"
			"inferences %ld\n"
			"focuses %ld\n"
			"goalupdates %ld\n"
			"ngoals %d\n"
			"nbeliefs %d\n"
			"focusatom %ld\n"
			"focussti %d\n",
			p->pid,
			cs->inferences,
			cs->focuses,
			cs->goalupdates,
			cs->ngoals,
			cs->nbeliefs,
			cs->focusatom,
			cs->focussti);
		unlock(cs);
		n = readstr(offset, va, n, buf);
		free(buf);
		return n;
		
	default:
		error(Egreg);
	}
	
	return 0;
}

/*
 * Write to /proc/pid/cog/ files
 */
static long
cogprocwrite(Chan *c, void *va, long n, vlong offset)
{
	Proc *p;
	CogProcState *cs;
	char *buf, *fields[8];
	int nfields, qid;
	ulong atomid;
	
	USED(offset);
	
	/* Get process from channel */
	p = up;  /* For now, only allow writing to own process */
	if(p == nil)
		error(Enonexist);
	
	cs = cogprocstate(p);
	if(cs == nil)
		error(Enomem);
	
	qid = QID(c->qid);
	
	/* Copy input */
	buf = smalloc(n + 1);
	memmove(buf, va, n);
	buf[n] = 0;
	
	/* Parse command */
	nfields = tokenize(buf, fields, nelem(fields));
	if(nfields < 1){
		free(buf);
		error(Ebadarg);
	}
	
	switch(qid){
	case Qfocus:
		/* Set attentional focus: "atom <id>" or "sti <threshold>" */
		if(nfields >= 2){
			if(strcmp(fields[0], "atom") == 0){
				atomid = strtoul(fields[1], nil, 0);
				lock(cs);
				cs->focusatom = atomid;
				cs->focuses++;
				unlock(cs);
			}
			else if(strcmp(fields[0], "sti") == 0){
				lock(cs);
				cs->focussti = atoi(fields[1]);
				unlock(cs);
			}
		}
		break;
		
	case Qgoals:
		/* Add goal: "add <atomid>" or "remove <atomid>" */
		if(nfields >= 2){
			atomid = strtoul(fields[1], nil, 0);
			
			if(strcmp(fields[0], "add") == 0){
				lock(cs);
				if(cs->ngoals < cs->maxgoals){
					cs->goals[cs->ngoals++] = atomid;
					cs->goalupdates++;
				}
				unlock(cs);
			}
			else if(strcmp(fields[0], "remove") == 0 || strcmp(fields[0], "del") == 0){
				int i, j;
				lock(cs);
				for(i = 0; i < cs->ngoals; i++){
					if(cs->goals[i] == atomid){
						/* Shift remaining goals */
						for(j = i; j < cs->ngoals - 1; j++)
							cs->goals[j] = cs->goals[j + 1];
						cs->ngoals--;
						cs->goalupdates++;
						break;
					}
				}
				unlock(cs);
			}
		}
		break;
		
	case Qbeliefs:
		/* Add belief: "add <atomid>" or "remove <atomid>" */
		if(nfields >= 2){
			atomid = strtoul(fields[1], nil, 0);
			
			if(strcmp(fields[0], "add") == 0){
				lock(cs);
				if(cs->nbeliefs < cs->maxbeliefs){
					cs->beliefs[cs->nbeliefs++] = atomid;
				}
				unlock(cs);
			}
			else if(strcmp(fields[0], "remove") == 0 || strcmp(fields[0], "del") == 0){
				int i, j;
				lock(cs);
				for(i = 0; i < cs->nbeliefs; i++){
					if(cs->beliefs[i] == atomid){
						/* Shift remaining beliefs */
						for(j = i; j < cs->nbeliefs - 1; j++)
							cs->beliefs[j] = cs->beliefs[j + 1];
						cs->nbeliefs--;
						break;
					}
				}
				unlock(cs);
			}
		}
		break;
		
	case Qctl:
		/* Control commands: "inference", "reset", etc. */
		if(strcmp(fields[0], "inference") == 0){
			lock(cs);
			cs->inferences++;
			unlock(cs);
		}
		else if(strcmp(fields[0], "reset") == 0){
			lock(cs);
			cs->inferences = 0;
			cs->focuses = 0;
			cs->goalupdates = 0;
			unlock(cs);
		}
		else if(strcmp(fields[0], "cleargoals") == 0){
			lock(cs);
			cs->ngoals = 0;
			cs->goalupdates++;
			unlock(cs);
		}
		else if(strcmp(fields[0], "clearbeliefs") == 0){
			lock(cs);
			cs->nbeliefs = 0;
			unlock(cs);
		}
		break;
		
	default:
		free(buf);
		error(Eperm);
	}
	
	free(buf);
	return n;
}

/*
 * Device structure for cognitive process extensions
 * 
 * Note: This is not a standalone device but an extension
 * that integrates with devproc.c to add /proc/pid/cog/
 */
Dev cogprocdevtab = {
	'Ψ',		/* Psi symbol for cognitive process */
	"cogproc",
	
	devreset,
	devinit,
	devshutdown,
	cogprocwalk,
	cogprocstat,
	cogprocopen,
	devcreate,
	cogprocclose,
	cogprocread,
	devbread,
	cogprocwrite,
	devbwrite,
	devremove,
	devwstat,
};
