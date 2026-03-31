/*
 * temporalfs - Temporal Reasoning File Server
 *
 * Exposes the TemporalSpace as a file hierarchy, providing
 * time-travel access to the AtomSpace using Plan 9's
 * "everything is a file" philosophy.
 *
 * Mount point: /mnt/temporal
 *
 * Hierarchy:
 *   /mnt/temporal/
 *     now         - Current atom list
 *     history     - Full atom change history
 *     changed     - Atoms changed in the last minute
 *     snap/       - Named snapshots directory
 *       <name>    - Snapshot info (time + atom count)
 *     ctl         - Control: snap <name>, restore <name>,
 *                            delsnap <name>, prune <time>
 *     stats       - Statistics
 */

#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include <plan9cog/atomspace.h>
#include <plan9cog/temporal.h>

enum
{
	Qroot,
	Qnow,
	Qhistory,
	Qchanged,
	Qsnaps,
	Qsnapfile,
	Qctl,
	Qstats,
};

typedef struct TFile TFile;
struct TFile
{
	int	qid;
	char	*snapname;	/* For Qsnapfile entries */
};

/* Global temporal space */
static TemporalSpace	*ts;
static AtomSpace	*as;

/*
 * Format a single atom as a line:  id type name strength confidence
 */
static int
fmtatom(char *buf, int size, Atom *a)
{
	TruthValue tv;

	if(a == nil)
		return 0;
	tv = atomgettruth(a);
	return snprint(buf, size, "%ld %d %s %.3f %.3f\n",
		a->id, a->type,
		a->name ? a->name : "(link)",
		tv.strength, tv.confidence);
}

/*
 * Read /now - list current atoms
 */
static void
readnow(Req *r)
{
	char *buf, *s;
	int i, size;
	Atom *a;

	size = 65536;
	buf = emalloc9p(size);
	s = buf;

	s += snprint(s, size - (s - buf), "# Current AtomSpace  time=%s\n",
		temporalstr(temporalnow()));
	s += snprint(s, size - (s - buf), "# id type name strength confidence\n");

	if(as != nil){
		for(i = 0; i < as->natoms; i++){
			a = as->atoms[i];
			if(a == nil)
				continue;
			s += fmtatom(s, size - (s - buf), a);
		}
	}

	readstr(r, buf);
	free(buf);
	respond(r, nil);
}

/*
 * Read /history - show temporal history of all atoms
 */
static void
readhistory(Req *r)
{
	char *buf, *s;
	int i, n, size;
	TemporalAtom **hist;
	TemporalAtom *ta;

	size = 65536;
	buf = emalloc9p(size);
	s = buf;

	s += snprint(s, size - (s - buf),
		"# Temporal history  now=%s\n", temporalstr(temporalnow()));
	s += snprint(s, size - (s - buf),
		"# id tstart tend strength confidence\n");

	if(as != nil && ts != nil){
		for(i = 0; i < as->natoms; i++){
			Atom *a = as->atoms[i];
			if(a == nil)
				continue;

			hist = temporalhistory(ts, a->id, &n);
			if(hist == nil)
				continue;

			for(ta = hist[0]; ta != nil; ta = ta->next){
				s += snprint(s, size - (s - buf),
					"%ld %s %s %.3f %.3f\n",
					ta->atom ? ta->atom->id : 0,
					temporalstr(ta->tstart),
					ta->tend ? temporalstr(ta->tend) : "now",
					ta->tvat.strength,
					ta->tvat.confidence);
			}
			free(hist);
		}
	}

	readstr(r, buf);
	free(buf);
	respond(r, nil);
}

/*
 * Read /changed - atoms changed in last 60 seconds
 */
static void
readchanged(Req *r)
{
	char *buf, *s;
	Atom **atoms;
	int i, n, size;
	vlong since;

	size = 32768;
	buf = emalloc9p(size);
	s = buf;

	since = temporalnow() - 60 * TimeSec;
	s += snprint(s, size - (s - buf),
		"# Atoms changed since %s\n", temporalstr(since));
	s += snprint(s, size - (s - buf),
		"# id type name strength confidence\n");

	if(ts != nil){
		atoms = temporalchanged(ts, since, &n);
		if(atoms != nil){
			for(i = 0; i < n; i++)
				s += fmtatom(s, size - (s - buf), atoms[i]);
			free(atoms);
		}
	}

	readstr(r, buf);
	free(buf);
	respond(r, nil);
}

/*
 * Read a snapshot file - show snapshot info and atom list
 */
static void
readsnapfile(Req *r, char *name)
{
	char *buf, *s;
	AtomSpace *snapas;
	int i, size;
	Atom *a;
	Snapshot **snaps;
	int nsnaps;
	vlong snaptime;

	if(ts == nil){
		respond(r, "no temporal space");
		return;
	}

	size = 32768;
	buf = emalloc9p(size);
	s = buf;

	/* Find snapshot time */
	snaptime = 0;
	snaps = temporalsnapslist(ts, &nsnaps);
	if(snaps != nil){
		for(i = 0; i < nsnaps; i++){
			if(strcmp(snaps[i]->name, name) == 0){
				snaptime = snaps[i]->time;
				break;
			}
		}
		free(snaps);
	}

	s += snprint(s, size - (s - buf), "# Snapshot: %s\n", name);
	if(snaptime)
		s += snprint(s, size - (s - buf), "# time: %s\n",
			temporalstr(snaptime));
	s += snprint(s, size - (s - buf),
		"# id type name strength confidence\n");

	snapas = temporalrestore(ts, name);
	if(snapas == nil){
		s += snprint(s, size - (s - buf), "# (empty snapshot)\n");
	} else {
		for(i = 0; i < snapas->natoms; i++){
			a = snapas->atoms[i];
			if(a == nil)
				continue;
			s += fmtatom(s, size - (s - buf), a);
		}
	}

	readstr(r, buf);
	free(buf);
	respond(r, nil);
}

/*
 * Read /stats
 */
static void
readstats(Req *r)
{
	char buf[2048];
	int nsnaps, nhistory;
	Snapshot **snaps;

	nhistory = ts ? ts->nhistory : 0;
	nsnaps = 0;
	if(ts != nil){
		snaps = temporalsnapslist(ts, &nsnaps);
		if(snaps != nil)
			free(snaps);
	}

	snprint(buf, sizeof buf,
		"atoms_current %d\n"
		"history_records %d\n"
		"snapshots %d\n"
		"granularity_ns %lld\n"
		"retention_ns %lld\n"
		"now %s\n",
		as ? as->natoms : 0,
		nhistory,
		nsnaps,
		ts ? ts->granularity : 0LL,
		ts ? ts->retention : 0LL,
		temporalstr(temporalnow()));

	readstr(r, buf);
	respond(r, nil);
}

/*
 * Write /ctl
 * Commands:
 *   snap <name>          - create named snapshot
 *   restore <name>       - restore from snapshot (replaces current)
 *   delsnap <name>       - delete named snapshot
 *   prune <seconds>      - prune history older than N seconds ago
 *   addatom <type> <name> - create atom in current space
 */
static void
writectl(Req *r)
{
	char *cmd;
	char *f[4];
	int nf;

	cmd = emalloc9p(r->ifcall.count + 1);
	memmove(cmd, r->ifcall.data, r->ifcall.count);
	cmd[r->ifcall.count] = '\0';

	/* Strip trailing newline */
	if(r->ifcall.count > 0 && cmd[r->ifcall.count - 1] == '\n')
		cmd[r->ifcall.count - 1] = '\0';

	nf = tokenize(cmd, f, nelem(f));
	if(nf < 1){
		free(cmd);
		respond(r, "invalid command");
		return;
	}

	if(strcmp(f[0], "snap") == 0 && nf >= 2){
		if(ts == nil){
			free(cmd);
			respond(r, "no temporal space");
			return;
		}
		if(temporalsnap(ts, f[1]) == nil){
			free(cmd);
			respond(r, "snap failed");
			return;
		}
		/* Create the file in the snap directory dynamically */
	}
	else if(strcmp(f[0], "restore") == 0 && nf >= 2){
		if(ts == nil){
			free(cmd);
			respond(r, "no temporal space");
			return;
		}
		if(temporalrestore(ts, f[1]) == nil){
			free(cmd);
			respond(r, "snapshot not found");
			return;
		}
	}
	else if(strcmp(f[0], "delsnap") == 0 && nf >= 2){
		if(ts == nil){
			free(cmd);
			respond(r, "no temporal space");
			return;
		}
		if(temporaldeletesnap(ts, f[1]) < 0){
			free(cmd);
			respond(r, "snapshot not found");
			return;
		}
	}
	else if(strcmp(f[0], "prune") == 0 && nf >= 2){
		vlong secs, cutoff;
		if(ts == nil){
			free(cmd);
			respond(r, "no temporal space");
			return;
		}
		secs = atoi(f[1]);
		cutoff = temporalnow() - secs * TimeSec;
		temporalprune(ts, cutoff);
	}
	else if(strcmp(f[0], "addatom") == 0 && nf >= 3){
		int type;
		Atom *a;
		if(ts == nil){
			free(cmd);
			respond(r, "no temporal space");
			return;
		}
		type = atoi(f[1]);
		a = temporalcreate(ts, type, f[2]);
		if(a == nil){
			free(cmd);
			respond(r, "addatom failed");
			return;
		}
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
 * snapdirgen - generate snap/ directory entries on the fly
 */
static void
snapdirgen(int s, Dir *d, void *aux)
{
	Snapshot **snaps;
	int nsnaps;
	TFile *tf;

	USED(aux);

	if(ts == nil){
		d->qid.path = 0;
		return;
	}

	snaps = temporalsnapslist(ts, &nsnaps);
	if(snaps == nil || s >= nsnaps){
		if(snaps)
			free(snaps);
		d->qid.path = 0;
		return;
	}

	tf = emalloc9p(sizeof(TFile));
	tf->qid = Qsnapfile;
	tf->snapname = strdup(snaps[s]->name);

	d->qid.path = Qsnapfile * 1000 + s;
	d->qid.vers = 0;
	d->qid.type = 0;
	d->mode = 0444;
	d->name = estrdup9p(snaps[s]->name);
	d->uid = estrdup9p("temporal");
	d->gid = estrdup9p("temporal");
	d->muid = estrdup9p("temporal");
	d->atime = d->mtime = (ulong)(snaps[s]->time / TimeSec);
	d->length = 0;
	d->aux = tf;

	free(snaps);
}

/*
 * File server read dispatch
 */
static void
fsread(Req *r)
{
	TFile *tf;

	tf = r->fid->file->aux;
	if(tf == nil){
		respond(r, "no file context");
		return;
	}

	switch(tf->qid){
	case Qnow:
		readnow(r);
		break;
	case Qhistory:
		readhistory(r);
		break;
	case Qchanged:
		readchanged(r);
		break;
	case Qsnapfile:
		readsnapfile(r, tf->snapname);
		break;
	case Qstats:
		readstats(r);
		break;
	default:
		respond(r, "not readable");
		break;
	}
}

/*
 * File server write dispatch
 */
static void
fswrite(Req *r)
{
	TFile *tf;

	tf = r->fid->file->aux;
	if(tf == nil){
		respond(r, "no file context");
		return;
	}

	switch(tf->qid){
	case Qctl:
		writectl(r);
		break;
	default:
		respond(r, "not writable");
		break;
	}
}

Srv fs = {
	.read	= fsread,
	.write	= fswrite,
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
	File *root, *snapdir;
	TFile *tf;

	mtpt = "/mnt/temporal";

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

	/* Initialize TemporalSpace and seed with a few demo atoms */
	ts = temporalinit(as);
	if(ts == nil)
		sysfatal("cannot create temporalspace");
	temporalcreate(ts, AtomNode, "concept:time");
	temporalcreate(ts, AtomNode, "concept:history");
	temporalcreate(ts, AtomNode, "concept:snapshot");

	/* Create file tree */
	fs.tree = alloctree(nil, nil, DMDIR|0555, nil);
	root = fs.tree->root;

	/* /now */
	tf = emalloc9p(sizeof(TFile));
	tf->qid = Qnow;
	tf->snapname = nil;
	createfile(root, "now", nil, 0444, tf);

	/* /history */
	tf = emalloc9p(sizeof(TFile));
	tf->qid = Qhistory;
	tf->snapname = nil;
	createfile(root, "history", nil, 0444, tf);

	/* /changed */
	tf = emalloc9p(sizeof(TFile));
	tf->qid = Qchanged;
	tf->snapname = nil;
	createfile(root, "changed", nil, 0444, tf);

	/* /snap/ (directory for named snapshots) */
	tf = emalloc9p(sizeof(TFile));
	tf->qid = Qsnaps;
	tf->snapname = nil;
	snapdir = createfile(root, "snap", nil, DMDIR|0555, tf);
	USED(snapdir);

	/* /ctl */
	tf = emalloc9p(sizeof(TFile));
	tf->qid = Qctl;
	tf->snapname = nil;
	createfile(root, "ctl", nil, 0220, tf);

	/* /stats */
	tf = emalloc9p(sizeof(TFile));
	tf->qid = Qstats;
	tf->snapname = nil;
	createfile(root, "stats", nil, 0444, tf);

	/* Start serving */
	threadpostmountsrv(&fs, nil, mtpt, MREPL);
	threadexits(nil);
}
