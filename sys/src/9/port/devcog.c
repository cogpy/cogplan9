/*
 * Cognitive Device Driver - devcog
 * 
 * Revolutionary kernel-level cognitive processing device.
 * Makes thinking, reasoning, and intelligence fundamental kernel services.
 * 
 * Inspired by Inferno's Dis VM, this implements a Cognitive Virtual Machine
 * at the kernel level where cognitive operations are first-class OS primitives.
 * 
 * Files exposed:
 *   #Σ/clone     - Allocate new cognitive context
 *   #Σ/atomspace - Kernel AtomSpace operations
 *   #Σ/pln       - Kernel PLN inference
 *   #Σ/ecan      - Kernel attention allocation
 *   #Σ/cogvm     - Cognitive VM state
 *   #Σ/stats     - Cognitive statistics
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

/* Cognitive Atom - Fundamental unit of knowledge in kernel */
typedef struct CogAtom CogAtom;
struct CogAtom
{
	ulong	id;		/* Unique atom ID */
	int	type;		/* Atom type (node/link) */
	char	name[256];	/* Atom name */
	CogAtom	**outgoing;	/* Outgoing atoms (for links) */
	int	noutgoing;	/* Number of outgoing atoms */
	float	tvstrength;	/* Truth value strength */
	float	tvconf;		/* Truth value confidence */
	short	sti;		/* Short-term importance */
	short	lti;		/* Long-term importance */
	Lock;
};

/* Kernel AtomSpace - Global hypergraph knowledge base */
typedef struct KernelAtomSpace KernelAtomSpace;
struct KernelAtomSpace
{
	CogAtom	**atoms;	/* Array of atoms */
	int	natoms;		/* Number of atoms */
	int	maxatoms;	/* Maximum capacity */
	ulong	nextid;		/* Next atom ID */
	QLock;			/* AtomSpace lock */
};

/* Cognitive Context - Per-process cognitive state */
typedef struct CogContext CogContext;
struct CogContext
{
	int	ref;		/* Reference count */
	int	ctxid;		/* Context ID */
	KernelAtomSpace *as;	/* AtomSpace reference */
	short	stitotal;	/* STI budget */
	short	ltitotal;	/* LTI budget */
	Lock;
};

/* Cognitive VM State - Kernel-level cognitive processor */
typedef struct CogVMState CogVMState;
struct CogVMState
{
	int	running;	/* VM running flag */
	ulong	cycles;		/* Cognitive cycles executed */
	ulong	inferences;	/* Inferences performed */
	ulong	allocations;	/* Attention allocations */
	Lock;
};

enum
{
	/* Atom types */
	AtomNode = 1,
	ConceptNode,
	PredicateNode,
	InheritanceLink,
	SimilarityLink,
	EvaluationLink,

	/* File QIDs */
	Qdir = 0,
	Qclone,
	Qatomspace,
	Qpln,
	Qecan,
	Qcogvm,
	Qstats,
	Qctl,

	/* Max atoms */
	MaxAtoms = 1000000,	/* 1 million atoms in kernel */
};

static Dirtab cogdir[] =
{
	".",		{Qdir, 0, QTDIR},	0,	DMDIR|0555,
	"clone",	{Qclone},		0,	0666,
	"atomspace",	{Qatomspace},		0,	0666,
	"pln",		{Qpln},			0,	0666,
	"ecan",		{Qecan},		0,	0666,
	"cogvm",	{Qcogvm},		0,	0444,
	"stats",	{Qstats},		0,	0444,
	"ctl",		{Qctl},			0,	0666,
};

/* Global kernel cognitive state */
static struct {
	KernelAtomSpace	atomspace;
	CogVMState	vmstate;
	CogContext	**contexts;
	int		ncontexts;
	int		maxcontexts;
	QLock;
} cogkernel;

static char Enomem[] = "out of cognitive memory";
static char Ebadcmd[] = "bad cognitive command";

/* Initialize kernel AtomSpace */
static void
cogatomspaceinit(void)
{
	KernelAtomSpace *as;

	as = &cogkernel.atomspace;
	as->maxatoms = MaxAtoms;
	as->atoms = malloc(MaxAtoms * sizeof(CogAtom*));
	if(as->atoms == nil)
		panic("cogatomspaceinit: no memory");
	as->natoms = 0;
	as->nextid = 1;
}

/* Create atom in kernel AtomSpace */
static CogAtom*
cogatomcreate(int type, char *name)
{
	KernelAtomSpace *as;
	CogAtom *atom;

	as = &cogkernel.atomspace;
	qlock(as);
	if(as->natoms >= as->maxatoms) {
		qunlock(as);
		error(Enomem);
	}

	atom = malloc(sizeof(CogAtom));
	if(atom == nil) {
		qunlock(as);
		error(Enomem);
	}

	atom->id = as->nextid++;
	atom->type = type;
	if(name)
		strncpy(atom->name, name, sizeof(atom->name)-1);
	atom->outgoing = nil;
	atom->noutgoing = 0;
	atom->tvstrength = 0.5;
	atom->tvconf = 0.5;
	atom->sti = 0;
	atom->lti = 0;

	as->atoms[as->natoms++] = atom;
	qunlock(as);

	return atom;
}

/* Find atom by ID */
static CogAtom*
cogatomfind(ulong id)
{
	KernelAtomSpace *as;
	int i;

	as = &cogkernel.atomspace;
	qlock(as);
	for(i = 0; i < as->natoms; i++) {
		if(as->atoms[i]->id == id) {
			qunlock(as);
			return as->atoms[i];
		}
	}
	qunlock(as);
	return nil;
}

/* PLN deduction at kernel level */
static void
cogplndeduction(CogAtom *ab, CogAtom *bc, float *strength, float *conf)
{
	*strength = ab->tvstrength * bc->tvstrength;
	*conf = ab->tvconf * bc->tvconf;
	cogkernel.vmstate.inferences++;
}

/* ECAN attention update */
static void
cogecanupdate(void)
{
	KernelAtomSpace *as;
	int i;

	as = &cogkernel.atomspace;
	qlock(as);
	
	/* Decay all attention values */
	for(i = 0; i < as->natoms; i++) {
		CogAtom *a = as->atoms[i];
		if(a->sti > 0)
			a->sti--;
	}
	
	cogkernel.vmstate.allocations++;
	qunlock(as);
}

/* Cognitive VM cycle */
static void
cogvmcycle(void)
{
	lock(&cogkernel.vmstate);
	cogkernel.vmstate.cycles++;
	unlock(&cogkernel.vmstate);
}

static void
coginit(void)
{
	cogatomspaceinit();
	cogkernel.vmstate.running = 1;
	cogkernel.vmstate.cycles = 0;
	cogkernel.vmstate.inferences = 0;
	cogkernel.vmstate.allocations = 0;
	cogkernel.maxcontexts = 1024;
	cogkernel.contexts = malloc(cogkernel.maxcontexts * sizeof(CogContext*));
	cogkernel.ncontexts = 0;
}

static Chan*
cogattach(char *spec)
{
	return devattach('Σ', spec);
}

static Walkqid*
cogwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, cogdir, nelem(cogdir), devgen);
}

static int
cogstat(Chan *c, uchar *db, int n)
{
	return devstat(c, db, n, cogdir, nelem(cogdir), devgen);
}

static Chan*
cogopen(Chan *c, int omode)
{
	return devopen(c, omode, cogdir, nelem(cogdir), devgen);
}

static void
cogclose(Chan*)
{
}

static long
cogread(Chan *c, void *va, long n, vlong off)
{
	char *buf, *p, *e;
	KernelAtomSpace *as;
	int i;

	switch((ulong)c->qid.path) {
	case Qdir:
		return devdirread(c, va, n, cogdir, nelem(cogdir), devgen);

	case Qatomspace:
		buf = malloc(8192);
		if(buf == nil)
			error(Enomem);
		p = buf;
		e = buf + 8192;
		
		as = &cogkernel.atomspace;
		qlock(as);
		p = seprint(p, e, "atoms: %d/%d\n", as->natoms, as->maxatoms);
		for(i = 0; i < as->natoms && i < 100; i++) {
			CogAtom *a = as->atoms[i];
			p = seprint(p, e, "atom %ld: type=%d name=%s tv=(%.2f,%.2f) av=(%d,%d)\n",
				a->id, a->type, a->name, a->tvstrength, a->tvconf, a->sti, a->lti);
		}
		qunlock(as);
		
		n = readstr(off, va, n, buf);
		free(buf);
		return n;

	case Qcogvm:
		buf = malloc(1024);
		if(buf == nil)
			error(Enomem);
		lock(&cogkernel.vmstate);
		seprint(buf, buf+1024, "running: %d\ncycles: %ld\ninferences: %ld\nallocations: %ld\n",
			cogkernel.vmstate.running, cogkernel.vmstate.cycles,
			cogkernel.vmstate.inferences, cogkernel.vmstate.allocations);
		unlock(&cogkernel.vmstate);
		n = readstr(off, va, n, buf);
		free(buf);
		return n;

	case Qstats:
		buf = malloc(2048);
		if(buf == nil)
			error(Enomem);
		as = &cogkernel.atomspace;
		qlock(as);
		seprint(buf, buf+2048, 
			"Kernel Cognitive Statistics\n"
			"===========================\n"
			"AtomSpace:\n"
			"  Total atoms: %d\n"
			"  Max atoms: %d\n"
			"  Memory: %ld KB\n"
			"Cognitive VM:\n"
			"  Cycles: %ld\n"
			"  Inferences: %ld\n"
			"  Attention updates: %ld\n"
			"Contexts: %d\n",
			as->natoms, as->maxatoms, (as->natoms * sizeof(CogAtom)) / 1024,
			cogkernel.vmstate.cycles, cogkernel.vmstate.inferences,
			cogkernel.vmstate.allocations, cogkernel.ncontexts);
		qunlock(as);
		n = readstr(off, va, n, buf);
		free(buf);
		return n;
	}

	error(Egreg);
	return 0;
}

static long
cogwrite(Chan *c, void *va, long n, vlong)
{
	char *cmd, *p;
	Cmdbuf *cb;
	CogAtom *atom, *ab, *bc;
	float strength, conf;
	int type;

	switch((ulong)c->qid.path) {
	case Qatomspace:
		cb = parsecmd(va, n);
		if(waserror()) {
			free(cb);
			nexterror();
		}
		cmd = cb->f[0];
		
		if(strcmp(cmd, "create") == 0) {
			if(cb->nf < 3)
				error(Ebadcmd);
			type = atoi(cb->f[1]);
			atom = cogatomcreate(type, cb->f[2]);
			cogvmcycle();
		}
		else if(strcmp(cmd, "settruth") == 0) {
			if(cb->nf < 4)
				error(Ebadcmd);
			atom = cogatomfind(atoi(cb->f[1]));
			if(atom == nil)
				error("atom not found");
			atom->tvstrength = atof(cb->f[2]);
			atom->tvconf = atof(cb->f[3]);
		}
		else
			error(Ebadcmd);
		
		poperror();
		free(cb);
		break;

	case Qpln:
		cb = parsecmd(va, n);
		if(waserror()) {
			free(cb);
			nexterror();
		}
		cmd = cb->f[0];
		
		if(strcmp(cmd, "deduction") == 0) {
			if(cb->nf < 3)
				error(Ebadcmd);
			ab = cogatomfind(atoi(cb->f[1]));
			bc = cogatomfind(atoi(cb->f[2]));
			if(ab == nil || bc == nil)
				error("atoms not found");
			cogplndeduction(ab, bc, &strength, &conf);
			cogvmcycle();
		}
		else
			error(Ebadcmd);
		
		poperror();
		free(cb);
		break;

	case Qecan:
		cb = parsecmd(va, n);
		if(waserror()) {
			free(cb);
			nexterror();
		}
		cmd = cb->f[0];
		
		if(strcmp(cmd, "update") == 0) {
			cogecanupdate();
			cogvmcycle();
		}
		else if(strcmp(cmd, "allocate") == 0) {
			if(cb->nf < 3)
				error(Ebadcmd);
			atom = cogatomfind(atoi(cb->f[1]));
			if(atom == nil)
				error("atom not found");
			atom->sti += atoi(cb->f[2]);
			cogkernel.vmstate.allocations++;
			cogvmcycle();
		}
		else
			error(Ebadcmd);
		
		poperror();
		free(cb);
		break;

	case Qctl:
		cb = parsecmd(va, n);
		if(waserror()) {
			free(cb);
			nexterror();
		}
		cmd = cb->f[0];
		
		if(strcmp(cmd, "start") == 0) {
			lock(&cogkernel.vmstate);
			cogkernel.vmstate.running = 1;
			unlock(&cogkernel.vmstate);
		}
		else if(strcmp(cmd, "stop") == 0) {
			lock(&cogkernel.vmstate);
			cogkernel.vmstate.running = 0;
			unlock(&cogkernel.vmstate);
		}
		else if(strcmp(cmd, "reset") == 0) {
			int i;
			qlock(&cogkernel.atomspace);
			/* Free all atoms */
			for(i = 0; i < cogkernel.atomspace.natoms; i++) {
				if(cogkernel.atomspace.atoms[i]) {
					if(cogkernel.atomspace.atoms[i]->outgoing)
						free(cogkernel.atomspace.atoms[i]->outgoing);
					free(cogkernel.atomspace.atoms[i]);
					cogkernel.atomspace.atoms[i] = nil;
				}
			}
			cogkernel.atomspace.natoms = 0;
			cogkernel.atomspace.nextid = 1;
			qunlock(&cogkernel.atomspace);
			lock(&cogkernel.vmstate);
			cogkernel.vmstate.cycles = 0;
			cogkernel.vmstate.inferences = 0;
			cogkernel.vmstate.allocations = 0;
			unlock(&cogkernel.vmstate);
		}
		else
			error(Ebadcmd);
		
		poperror();
		free(cb);
		break;

	default:
		error(Egreg);
	}

	return n;
}

Dev cogdevtab = {
	'Σ',
	"cog",

	devreset,
	coginit,
	devshutdown,
	cogattach,
	cogwalk,
	cogstat,
	cogopen,
	devcreate,
	cogclose,
	cogread,
	devbread,
	cogwrite,
	devbwrite,
	devremove,
	devwstat,
};
