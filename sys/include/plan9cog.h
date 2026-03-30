/*
 * Plan9Cog - AGI-enabled Plan 9 Operating System
 * Main header for cognitive computing integration
 * 
 * Integrates OpenCog Collection (OCC), HurdCog, and Cognumach
 * concepts into the Plan 9 distributed system architecture.
 * 
 * Architecture:
 *   Layer 1: Plan9 Kernel with Cognitive Extensions
 *   Layer 2: Cognitive Services (AtomSpace, PLN, ECAN)
 *   Layer 3: AGI Research Platform (URE, Pattern Mining)
 */

#ifndef _PLAN9COG_H_
#define _PLAN9COG_H_ 1

#pragma src "/sys/src/libplan9cog"
#pragma lib "libplan9cog.a"

/* Core cognitive components */
#include <plan9cog/atomspace.h>		/* Hypergraph knowledge representation */
#include <plan9cog/pln.h>		/* Probabilistic Logic Networks */
#include <plan9cog/cogvm.h>		/* Cognitive VM extensions */
#include <plan9cog/tensorlogic.h>	/* Tensor Logic reasoning */
#include <plan9cog/openpsi.h>		/* OpenPsi goal-directed behavior */
#include <plan9cog/temporal.h>		/* Temporal reasoning */

/* Plan9Cog System */
typedef struct Plan9Cog Plan9Cog;
struct Plan9Cog {
	AtomSpace *atomspace;	/* Global AtomSpace */
	PlnInference *pln;	/* PLN inference engine */
	CogMemory *cogmem;	/* Cognitive memory manager */
	int initialized;	/* Initialization flag */
	Lock;			/* System lock */
};

/* System initialization */
Plan9Cog*	plan9coginit(void);
void		plan9cogfree(Plan9Cog *p9c);
Plan9Cog*	plan9coginstance(void);	/* Get global instance */

/* Cognitive file server protocol */
enum {
	/* File server cognitive operations */
	Tcogatom = 100,		/* Create/query atom */
	Rcogatom,
	Tcogpln,		/* PLN inference */
	Rcogpln,
	Tcogecan,		/* ECAN attention allocation */
	Rcogecan,
	Tcogpattern,		/* Pattern matching */
	Rcogpattern,
	Tcogmine,		/* Pattern mining */
	Rcogmine,
};

/* Cognitive Fusion Reactor - distributed cognitive processing */
typedef struct CogFusionReactor CogFusionReactor;
struct CogFusionReactor {
	Plan9Cog *p9c;		/* Plan9Cog system */
	int nworkers;		/* Number of worker processes */
	Chan **workers;		/* Worker channels */
	Lock;			/* Reactor lock */
};

CogFusionReactor*	cogreactorinit(Plan9Cog *p9c, int nworkers);
void			cogreactorfree(CogFusionReactor *cfr);
void			cogreactorsubmit(CogFusionReactor *cfr, void *task);
void*			cogreactorresult(CogFusionReactor *cfr);

/* MachSpace - distributed hypergraph memory system */
typedef struct MachSpace MachSpace;
struct MachSpace {
	AtomSpace *local;	/* Local AtomSpace */
	AtomSpace **remote;	/* Remote AtomSpaces */
	int nremote;		/* Number of remote spaces */
	char **hosts;		/* Remote host names */
	Lock;			/* MachSpace lock */
};

MachSpace*	machspaceinit(AtomSpace *local);
void		machspacefree(MachSpace *ms);
int		machspaceconnect(MachSpace *ms, char *host);
int		machspace9pconnect(MachSpace *ms, char *addr);
int		machspace9pdisconnect(MachSpace *ms, char *host);
Atom*		machspacefind(MachSpace *ms, ulong id);
Atom*		machspacecreate(MachSpace *ms, int type, char *name);
Atom*		machspacelink(MachSpace *ms, int type, Atom **outgoing, int n);
Atom**		machspacequery(MachSpace *ms, AtomPredicate pred, void *arg, int *n);
int		machspacesync(MachSpace *ms);
char**		machspacenodes(MachSpace *ms, int *n);

/* Cognitive Grip - universal object handling mechanism */
enum {
	GripAtom = 1,		/* Atom object */
	GripPattern,		/* Pattern object */
	GripRule,		/* Rule object */
	GripTask,		/* Task object */
	GripResult,		/* Result object */
};

typedef struct CogGrip CogGrip;
struct CogGrip {
	int type;		/* Grip type */
	void *object;		/* Gripped object */
	int refcount;		/* Reference count */
	Lock;			/* Grip lock */
};

CogGrip*	coggrip(int type, void *object);
void		cogrelease(CogGrip *grip);
void*		cogobject(CogGrip *grip);
CogGrip*	cogretain(CogGrip *grip);

/* Master Control Dashboard - monitoring and control */
typedef struct CogDashboard CogDashboard;
struct CogDashboard {
	Plan9Cog *p9c;		/* Plan9Cog system */
	int httpport;		/* HTTP port for dashboard */
	void *httpd;		/* HTTP server */
	Lock;			/* Dashboard lock */
};

CogDashboard*	cogdashboardinit(Plan9Cog *p9c, int port);
void		cogdashboardfree(CogDashboard *cd);
void		cogdashboardstart(CogDashboard *cd);
void		cogdashboardstop(CogDashboard *cd);

/* Economic Attention Network (ECAN) */
typedef struct EcanNetwork EcanNetwork;
struct EcanNetwork {
	AtomSpace *as;		/* AtomSpace */
	short totalsti;		/* Total STI funds */
	short totallti;		/* Total LTI funds */
	short attentionalfocus;	/* Attentional focus size */
	Atom **focusatoms;	/* Atoms in attentional focus */
	int nfocus;		/* Number of focus atoms */
	Lock;			/* ECAN lock */
};

EcanNetwork*	ecaninit(AtomSpace *as, short totalsti, short totallti);
void		ecanfree(EcanNetwork *ecan);
void		ecanupdate(EcanNetwork *ecan);
void		ecanallocate(EcanNetwork *ecan, Atom *a, short sti);
void		ecanspread(EcanNetwork *ecan, Atom *source);
Atom**		ecanfocus(EcanNetwork *ecan, int *n);
void		ecandecay(EcanNetwork *ecan, float rate);

/* Pattern Mining */
typedef struct PatternMiner PatternMiner;
struct PatternMiner {
	AtomSpace *as;		/* AtomSpace */
	int minsupport;		/* Minimum support */
	float minconf;		/* Minimum confidence */
	Pattern **patterns;	/* Mined patterns */
	int npatterns;		/* Number of patterns */
	Lock;			/* Miner lock */
};

PatternMiner*	patterninit(AtomSpace *as);
void		patternfree(PatternMiner *pm);
void		patternmine(PatternMiner *pm, int minsupport);
Pattern**	patternget(PatternMiner *pm, int *n);
float		patternsupport(PatternMiner *pm, Pattern *pat);
float		patternconfidence(PatternMiner *pm, Pattern *pat);

/* Cognitive utilities */
void		cogprint(char *fmt, ...);
void		cogdebug(int level, char *fmt, ...);
char*		cogatomstr(Atom *a);
char*		cogpatternstr(Pattern *pat);
char*		cogtvstr(TruthValue tv);

/* System information */
typedef struct CogInfo CogInfo;
struct CogInfo {
	char version[32];	/* Plan9Cog version */
	ulong uptime;		/* System uptime */
	ulong natoms;		/* Total atoms */
	ulong nrules;		/* Total rules */
	ulong ninferences;	/* Total inferences */
	ulong cogmem;		/* Cognitive memory usage */
};

void		coginfo(Plan9Cog *p9c, CogInfo *info);

#endif /* _PLAN9COG_H_ */
