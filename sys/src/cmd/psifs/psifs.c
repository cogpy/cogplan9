/*
 * psifs - OpenPsi File Server
 * 
 * Exposes OpenPsi motivational system as a file hierarchy.
 * Provides access to urges, goals, demands, and modulators.
 * 
 * Mount point: /mnt/psi or /n/psi
 * 
 * Hierarchy:
 *   /mnt/psi/
 *     urges/
 *       curiosity/
 *         level       - Current urge level
 *         target      - Target satisfaction level
 *         demands     - Associated demands
 *       competence/
 *       certainty/
 *       ...
 *     goals/
 *       <id>/
 *         state       - Goal state
 *         urge        - Associated urge
 *         priority    - Goal priority
 *         context     - Context atoms
 *         actions     - Possible actions
 *       active        - List active goals
 *       pending       - List pending goals
 *     demands/
 *       <id>/
 *         level       - Demand level
 *         urge        - Associated urge
 *         satisfiers  - Satisfying actions
 *     modulators/
 *       arousal       - Arousal level
 *       valence       - Valence level
 *       resolution    - Resolution level
 *       selection     - Selection threshold
 *       securing      - Securing threshold
 *     ctl             - Control interface
 *     stats           - Statistics
 */

#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include <plan9cog/atomspace.h>
#include <plan9cog/openpsi.h>

enum
{
	Qroot,
	Qurges,
	Qurgedir,
	Qurgelevel,
	Qurgetarget,
	Qurgedemands,
	Qgoals,
	Qgoaldir,
	Qgoalstate,
	Qgoalurge,
	Qgoalpriority,
	Qgoalcontext,
	Qgoalactions,
	Qgoalsactive,
	Qgoalspending,
	Qdemands,
	Qdemanddir,
	Qdemandlevel,
	Qdemandurge,
	Qdemandsatisfiers,
	Qmodulators,
	Qmodulatorfile,
	Qctl,
	Qstats,
};

typedef struct PsiFile PsiFile;
struct PsiFile
{
	int qid;
	char *name;
	int urgetype;	/* For urge files */
	int modtype;	/* For modulator files */
	ulong goalid;	/* For goal files */
	ulong demandid;	/* For demand files */
};

/* Global OpenPsi context */
static OpenPsi *psictx;
static AtomSpace *as;

/*
 * Read urge level
 */
static void
readurgelevel(Req *r, int urgetype)
{
	char buf[256];
	PsiUrge *u;
	int i;
	
	u = nil;
	for(i = 0; i < psictx->nurges; i++){
		if(psictx->urges[i]->type == urgetype){
			u = psictx->urges[i];
			break;
		}
	}
	
	if(u == nil){
		respond(r, "urge not found");
		return;
	}
	
	snprint(buf, sizeof buf, "%.3f\n", u->level);
	readstr(r, buf);
	respond(r, nil);
}

/*
 * Write urge level
 */
static void
writeurgelevel(Req *r, int urgetype)
{
	PsiUrge *u;
	float level;
	int i;
	
	u = nil;
	for(i = 0; i < psictx->nurges; i++){
		if(psictx->urges[i]->type == urgetype){
			u = psictx->urges[i];
			break;
		}
	}
	
	if(u == nil){
		respond(r, "urge not found");
		return;
	}
	
	level = atof(r->ifcall.data);
	psiurgeset(psictx, urgetype, level);
	
	r->ofcall.count = r->ifcall.count;
	respond(r, nil);
}

/*
 * Read modulator value
 */
static void
readmodulator(Req *r, int modtype)
{
	char buf[256];
	float value;
	
	if(modtype < 0 || modtype >= NumModulators){
		respond(r, "invalid modulator");
		return;
	}
	
	value = psictx->modulators[modtype];
	snprint(buf, sizeof buf, "%.3f\n", value);
	readstr(r, buf);
	respond(r, nil);
}

/*
 * Write modulator value
 */
static void
writemodulator(Req *r, int modtype)
{
	float value;
	
	if(modtype < 0 || modtype >= NumModulators){
		respond(r, "invalid modulator");
		return;
	}
	
	value = atof(r->ifcall.data);
	if(value < 0.0)
		value = 0.0;
	if(value > 1.0)
		value = 1.0;
	
	psictx->modulators[modtype] = value;
	
	r->ofcall.count = r->ifcall.count;
	respond(r, nil);
}

/*
 * Read active goals
 */
static void
readgoalsactive(Req *r)
{
	char *buf, *s;
	PsiGoal *g;
	int i, size;
	
	size = 8192;
	buf = emalloc9p(size);
	s = buf;
	
	s += snprint(s, size - (s - buf), "# Active goals\n");
	s += snprint(s, size - (s - buf), "# id urge priority status\n");
	
	for(i = 0; i < psictx->ngoals; i++){
		g = psictx->goals[i];
		if(g == nil || g->status != GoalPursuing)
			continue;
		
		s += snprint(s, size - (s - buf), "%ld %d %.3f %d\n",
			g->id, g->urge, g->priority, g->status);
	}
	
	readstr(r, buf);
	free(buf);
	respond(r, nil);
}

/*
 * Read pending goals
 */
static void
readgoalspending(Req *r)
{
	char *buf, *s;
	PsiGoal *g;
	int i, size;
	
	size = 8192;
	buf = emalloc9p(size);
	s = buf;
	
	s += snprint(s, size - (s - buf), "# Pending goals\n");
	s += snprint(s, size - (s - buf), "# id urge priority status\n");
	
	for(i = 0; i < psictx->ngoals; i++){
		g = psictx->goals[i];
		if(g == nil || g->status != GoalPending)
			continue;
		
		s += snprint(s, size - (s - buf), "%ld %d %.3f %d\n",
			g->id, g->urge, g->priority, g->status);
	}
	
	readstr(r, buf);
	free(buf);
	respond(r, nil);
}

/*
 * Read goal state
 */
static void
readgoalstate(Req *r, ulong goalid)
{
	char buf[1024];
	PsiGoal *g;
	int i;
	
	g = nil;
	for(i = 0; i < psictx->ngoals; i++){
		if(psictx->goals[i]->id == goalid){
			g = psictx->goals[i];
			break;
		}
	}
	
	if(g == nil){
		respond(r, "goal not found");
		return;
	}
	
	snprint(buf, sizeof buf,
		"id %ld\n"
		"urge %d\n"
		"priority %.3f\n"
		"status %d\n"
		"atom %ld\n",
		g->id, g->urge, g->priority, g->status,
		g->atom ? g->atom->id : 0);
	
	readstr(r, buf);
	respond(r, nil);
}

/*
 * Read statistics
 */
static void
readstats(Req *r)
{
	char buf[2048];
	int i, active, pending;
	
	active = 0;
	pending = 0;
	for(i = 0; i < psictx->ngoals; i++){
		if(psictx->goals[i] == nil)
			continue;
		if(psictx->goals[i]->status == GoalPursuing)
			active++;
		else if(psictx->goals[i]->status == GoalPending)
			pending++;
	}
	
	snprint(buf, sizeof buf,
		"cycles %ld\n"
		"goals_total %d\n"
		"goals_active %d\n"
		"goals_pending %d\n"
		"goals_set %ld\n"
		"goals_met %ld\n"
		"goals_failed %ld\n"
		"urges %d\n"
		"demands %d\n"
		"actions %d\n"
		"arousal %.3f\n"
		"valence %.3f\n"
		"resolution %.3f\n",
		psictx->cycles,
		psictx->ngoals,
		active,
		pending,
		psictx->goalsset,
		psictx->goalsmet,
		psictx->goalsfailed,
		psictx->nurges,
		psictx->ndemands,
		psictx->nactions,
		psictx->modulators[ModArousal],
		psictx->modulators[ModValence],
		psictx->modulators[ModResolution]);
	
	readstr(r, buf);
	respond(r, nil);
}

/*
 * Control interface
 */
static void
writectl(Req *r)
{
	char *cmd;
	char *f[8];
	int nf;
	
	cmd = emalloc9p(r->ifcall.count + 1);
	memmove(cmd, r->ifcall.data, r->ifcall.count);
	cmd[r->ifcall.count] = 0;
	
	nf = tokenize(cmd, f, nelem(f));
	if(nf < 1){
		free(cmd);
		respond(r, "invalid command");
		return;
	}
	
	if(strcmp(f[0], "cycle") == 0){
		/* Run one OpenPsi cycle */
		psicycle(psictx);
	}
	else if(strcmp(f[0], "reset") == 0){
		/* Reset statistics */
		psictx->cycles = 0;
		psictx->goalsset = 0;
		psictx->goalsmet = 0;
		psictx->goalsfailed = 0;
	}
	else if(strcmp(f[0], "addgoal") == 0 && nf >= 3){
		/* Add goal: addgoal <urge> <priority> */
		int urge = atoi(f[1]);
		float priority = atof(f[2]);
		psigoalcreate(psictx, urge, priority);
	}
	else{
		free(cmd);
		respond(r, "unknown command");
		return;
	}
	
	free(cmd);
	r->ofcall.count = r->ifcall.count;
	respond(r, nil);
}

/*
 * File server read
 */
static void
fsread(Req *r)
{
	PsiFile *pf;
	
	pf = r->fid->file->aux;
	if(pf == nil){
		respond(r, "no file context");
		return;
	}
	
	switch(pf->qid){
	case Qurgelevel:
		readurgelevel(r, pf->urgetype);
		break;
	case Qurgetarget:
		/* Similar to urgelevel but for target */
		readurgelevel(r, pf->urgetype);
		break;
	case Qgoalsactive:
		readgoalsactive(r);
		break;
	case Qgoalspending:
		readgoalspending(r);
		break;
	case Qgoalstate:
		readgoalstate(r, pf->goalid);
		break;
	case Qmodulatorfile:
		readmodulator(r, pf->modtype);
		break;
	case Qstats:
		readstats(r);
		break;
	default:
		respond(r, "not implemented");
		break;
	}
}

/*
 * File server write
 */
static void
fswrite(Req *r)
{
	PsiFile *pf;
	
	pf = r->fid->file->aux;
	if(pf == nil){
		respond(r, "no file context");
		return;
	}
	
	switch(pf->qid){
	case Qurgelevel:
		writeurgelevel(r, pf->urgetype);
		break;
	case Qmodulatorfile:
		writemodulator(r, pf->modtype);
		break;
	case Qctl:
		writectl(r);
		break;
	default:
		respond(r, "not writable");
		break;
	}
}

Srv fs = {
	.read = fsread,
	.write = fswrite,
};

void
usage(void)
{
	fprint(2, "usage: %s [-m mtpt]\n", argv0);
	threadexitsall("usage");
}

void
threadmain(int argc, char *argv[])
{
	char *mtpt;
	File *root, *urges, *goals, *demands, *mods;
	File *f;
	PsiFile *pf;
	int i;
	char *modnames[] = {"arousal", "valence", "resolution", "selection", "securing"};
	
	mtpt = "/mnt/psi";
	
	ARGBEGIN{
	case 'm':
		mtpt = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND
	
	/* Initialize AtomSpace */
	as = atomspacecreate();
	if(as == nil)
		sysfatal("cannot create atomspace");
	
	/* Initialize OpenPsi */
	psictx = psiinit(as);
	if(psictx == nil)
		sysfatal("cannot initialize openpsi");
	
	/* Create file tree */
	fs.tree = alloctree(nil, nil, DMDIR|0555, nil);
	root = fs.tree->root;
	
	/* /urges/ */
	pf = emalloc9p(sizeof(PsiFile));
	pf->qid = Qurges;
	urges = createfile(root, "urges", nil, DMDIR|0555, pf);
	
	/* Create urge directories */
	for(i = 0; i < psictx->nurges; i++){
		PsiUrge *u = psictx->urges[i];
		File *udir;
		
		pf = emalloc9p(sizeof(PsiFile));
		pf->qid = Qurgedir;
		pf->urgetype = u->type;
		udir = createfile(urges, u->name, nil, DMDIR|0555, pf);
		
		/* level file */
		pf = emalloc9p(sizeof(PsiFile));
		pf->qid = Qurgelevel;
		pf->urgetype = u->type;
		createfile(udir, "level", nil, 0644, pf);
		
		/* target file */
		pf = emalloc9p(sizeof(PsiFile));
		pf->qid = Qurgetarget;
		pf->urgetype = u->type;
		createfile(udir, "target", nil, 0644, pf);
	}
	
	/* /goals/ */
	pf = emalloc9p(sizeof(PsiFile));
	pf->qid = Qgoals;
	goals = createfile(root, "goals", nil, DMDIR|0555, pf);
	
	pf = emalloc9p(sizeof(PsiFile));
	pf->qid = Qgoalsactive;
	createfile(goals, "active", nil, 0444, pf);
	
	pf = emalloc9p(sizeof(PsiFile));
	pf->qid = Qgoalspending;
	createfile(goals, "pending", nil, 0444, pf);
	
	/* /modulators/ */
	pf = emalloc9p(sizeof(PsiFile));
	pf->qid = Qmodulators;
	mods = createfile(root, "modulators", nil, DMDIR|0555, pf);
	
	/* Create modulator files */
	for(i = 0; i < NumModulators; i++){
		pf = emalloc9p(sizeof(PsiFile));
		pf->qid = Qmodulatorfile;
		pf->modtype = i;
		createfile(mods, modnames[i], nil, 0644, pf);
	}
	
	/* /ctl */
	pf = emalloc9p(sizeof(PsiFile));
	pf->qid = Qctl;
	createfile(root, "ctl", nil, 0220, pf);
	
	/* /stats */
	pf = emalloc9p(sizeof(PsiFile));
	pf->qid = Qstats;
	createfile(root, "stats", nil, 0444, pf);
	
	/* Start serving */
	threadpostmountsrv(&fs, nil, mtpt, MREPL);
	threadexits(nil);
}
