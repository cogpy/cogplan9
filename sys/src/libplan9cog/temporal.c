/*
 * Temporal Reasoning for Plan9Cog
 *
 * Provides time-aware AtomSpace operations.
 * Leverages Plan 9's file-based approach for temporal queries.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <plan9cog.h>
#include <plan9cog/temporal.h>

TemporalSpace*
temporalinit(AtomSpace *current)
{
	TemporalSpace *ts;

	ts = mallocz(sizeof(TemporalSpace), 1);
	if(ts == nil)
		return nil;

	ts->current = current;
	ts->maxhistory = 1024;
	ts->history = mallocz(sizeof(TemporalAtom*) * ts->maxhistory, 1);
	if(ts->history == nil){
		free(ts);
		return nil;
	}
	ts->nhistory = 0;
	ts->snapshots = nil;
	ts->granularity = TimeMillisec;	/* Millisecond resolution */
	ts->retention = 7 * TimeDay;	/* Keep 7 days of history */
	ts->basepath = strdup("/mnt/cog/temporal");

	return ts;
}

void
temporalfree(TemporalSpace *ts)
{
	int i;
	Snapshot *snap, *nextsnap;
	TemporalAtom *ta, *nextta;

	if(ts == nil)
		return;

	lock(ts);

	/* Free history */
	for(i = 0; i < ts->nhistory; i++){
		ta = ts->history[i];
		while(ta != nil){
			nextta = ta->next;
			free(ta);
			ta = nextta;
		}
	}
	free(ts->history);

	/* Free snapshots */
	snap = ts->snapshots;
	while(snap != nil){
		nextsnap = snap->next;
		free(snap->name);
		if(snap->state)
			atomspacefree(snap->state);
		free(snap);
		snap = nextsnap;
	}

	free(ts->basepath);
	unlock(ts);
	free(ts);
}

/* Time-aware atom creation */
Atom*
temporalcreate(TemporalSpace *ts, int type, char *name)
{
	Atom *a;

	if(ts == nil || ts->current == nil)
		return nil;

	a = atomcreate(ts->current, type, name);
	if(a != nil)
		temporalrecord(ts, a);

	return a;
}

/* Find atom as it existed at a specific time */
Atom*
temporalfindat(TemporalSpace *ts, ulong id, vlong time)
{
	int i;
	TemporalAtom *ta;

	if(ts == nil)
		return nil;

	/* Current time - return current atom */
	if(time == 0 || time >= temporalnow())
		return atomfind(ts->current, id);

	lock(ts);

	/* Search history for the version valid at that time */
	for(i = 0; i < ts->nhistory; i++){
		for(ta = ts->history[i]; ta != nil; ta = ta->next){
			if(ta->atom == nil)
				continue;
			if(ta->atom->id != id)
				continue;

			/* Check if this version was valid at the requested time */
			if(ta->tstart <= time && (ta->tend == 0 || ta->tend > time)){
				unlock(ts);
				return ta->atom;
			}
		}
	}

	unlock(ts);
	return nil;
}

/* Get truth value of atom at specific time */
TruthValue
temporaltvat(TemporalSpace *ts, Atom *a, vlong time)
{
	TruthValue tv;
	int i;
	TemporalAtom *ta;

	memset(&tv, 0, sizeof(tv));
	tv.strength = 0.5;

	if(ts == nil || a == nil)
		return tv;

	/* Current time */
	if(time == 0 || time >= temporalnow())
		return atomgettruth(a);

	lock(ts);

	/* Find historical truth value */
	for(i = 0; i < ts->nhistory; i++){
		for(ta = ts->history[i]; ta != nil; ta = ta->next){
			if(ta->atom == nil || ta->atom->id != a->id)
				continue;

			if(ta->tstart <= time && (ta->tend == 0 || ta->tend > time)){
				tv = ta->tvat;
				unlock(ts);
				return tv;
			}
		}
	}

	unlock(ts);
	return tv;
}

/* Delete atom and record end time in history */
int
temporaldelete(TemporalSpace *ts, ulong id)
{
	int i;
	TemporalAtom *ta;
	vlong now;

	if(ts == nil)
		return -1;

	now = temporalnow();

	lock(ts);

	/* Mark historical records as ended */
	for(i = 0; i < ts->nhistory; i++){
		for(ta = ts->history[i]; ta != nil; ta = ta->next){
			if(ta->atom && ta->atom->id == id && ta->tend == 0){
				ta->tend = now;
			}
		}
	}

	unlock(ts);

	/* Delete from current atomspace */
	return atomdelete(ts->current, id);
}

/* Record atom state in history */
int
temporalrecord(TemporalSpace *ts, Atom *a)
{
	TemporalAtom *ta, *existing;
	TemporalAtom **newhistory;
	vlong now;
	int i, slot;

	if(ts == nil || a == nil)
		return -1;

	now = temporalnow();

	/* Create temporal record */
	ta = mallocz(sizeof(TemporalAtom), 1);
	if(ta == nil)
		return -1;

	ta->atom = a;
	ta->tstart = now;
	ta->tend = 0;	/* Still valid */
	ta->tvat = atomgettruth(a);
	ta->prev = nil;
	ta->next = nil;

	lock(ts);

	/* Find existing history chain for this atom */
	slot = -1;
	for(i = 0; i < ts->nhistory; i++){
		if(ts->history[i] == nil){
			if(slot < 0)
				slot = i;
			continue;
		}

		existing = ts->history[i];
		if(existing->atom && existing->atom->id == a->id){
			/* End previous version */
			while(existing->next != nil)
				existing = existing->next;

			if(existing->tend == 0)
				existing->tend = now;

			/* Link new version */
			existing->next = ta;
			ta->prev = existing;
			unlock(ts);
			return 0;
		}
	}

	/* New atom - add to history */
	if(slot < 0){
		if(ts->nhistory >= ts->maxhistory){
			ts->maxhistory *= 2;
			newhistory = realloc(ts->history, sizeof(TemporalAtom*) * ts->maxhistory);
			if(newhistory == nil){
				unlock(ts);
				free(ta);
				return -1;
			}
			ts->history = newhistory;
		}
		slot = ts->nhistory++;
	}

	ts->history[slot] = ta;
	unlock(ts);

	return 0;
}

/* Get history of an atom */
TemporalAtom**
temporalhistory(TemporalSpace *ts, ulong id, int *n)
{
	TemporalAtom **results;
	TemporalAtom *ta;
	int i, count, maxresults;

	*n = 0;
	if(ts == nil)
		return nil;

	maxresults = 32;
	results = mallocz(sizeof(TemporalAtom*) * maxresults, 1);
	if(results == nil)
		return nil;

	count = 0;

	lock(ts);

	for(i = 0; i < ts->nhistory; i++){
		for(ta = ts->history[i]; ta != nil; ta = ta->next){
			if(ta->atom == nil || ta->atom->id != id)
				continue;

			if(count >= maxresults){
				maxresults *= 2;
				results = realloc(results, sizeof(TemporalAtom*) * maxresults);
				if(results == nil){
					unlock(ts);
					*n = 0;
					return nil;
				}
			}
			results[count++] = ta;
		}
	}

	unlock(ts);

	*n = count;
	return results;
}

/* Remove history older than specified time */
void
temporalprune(TemporalSpace *ts, vlong before)
{
	int i;
	TemporalAtom *ta, *prev, *next;

	if(ts == nil)
		return;

	lock(ts);

	for(i = 0; i < ts->nhistory; i++){
		prev = nil;
		for(ta = ts->history[i]; ta != nil; ta = next){
			next = ta->next;

			/* Remove if ended before cutoff */
			if(ta->tend != 0 && ta->tend < before){
				if(prev == nil)
					ts->history[i] = next;
				else
					prev->next = next;

				if(next != nil)
					next->prev = prev;

				free(ta);
			} else {
				prev = ta;
			}
		}
	}

	unlock(ts);
}

/* Create named snapshot */
Snapshot*
temporalsnap(TemporalSpace *ts, char *name)
{
	Snapshot *snap;
	int i;
	Atom *a, *copy;

	if(ts == nil || name == nil)
		return nil;

	snap = mallocz(sizeof(Snapshot), 1);
	if(snap == nil)
		return nil;

	snap->name = strdup(name);
	snap->time = temporalnow();
	snap->state = atomspacecreate();

	if(snap->state == nil){
		free(snap->name);
		free(snap);
		return nil;
	}

	/* Copy current atomspace to snapshot */
	lock(ts);

	if(ts->current != nil){
		for(i = 0; i < ts->current->natoms; i++){
			a = ts->current->atoms[i];
			if(a == nil)
				continue;

			if(a->noutgoing == 0){
				copy = atomcreate(snap->state, a->type, a->name);
			} else {
				/* Links need special handling */
				copy = atomcreate(snap->state, a->type, nil);
			}

			if(copy != nil){
				TruthValue tv = atomgettruth(a);
				AttentionValue av = atomgetattention(a);
				atomsettruth(copy, tv);
				atomsetattention(copy, av);
			}
		}
	}

	/* Add to snapshot list */
	snap->next = ts->snapshots;
	ts->snapshots = snap;

	unlock(ts);

	return snap;
}

/* Restore from named snapshot */
AtomSpace*
temporalrestore(TemporalSpace *ts, char *name)
{
	Snapshot *snap;

	if(ts == nil || name == nil)
		return nil;

	lock(ts);

	for(snap = ts->snapshots; snap != nil; snap = snap->next){
		if(strcmp(snap->name, name) == 0){
			unlock(ts);
			return snap->state;
		}
	}

	unlock(ts);
	return nil;
}

/* Delete named snapshot */
int
temporaldeletesnap(TemporalSpace *ts, char *name)
{
	Snapshot *snap, *prev;

	if(ts == nil || name == nil)
		return -1;

	lock(ts);

	prev = nil;
	for(snap = ts->snapshots; snap != nil; prev = snap, snap = snap->next){
		if(strcmp(snap->name, name) == 0){
			if(prev == nil)
				ts->snapshots = snap->next;
			else
				prev->next = snap->next;

			free(snap->name);
			if(snap->state)
				atomspacefree(snap->state);
			free(snap);

			unlock(ts);
			return 0;
		}
	}

	unlock(ts);
	return -1;
}

/* List all snapshots */
Snapshot**
temporalsnapslist(TemporalSpace *ts, int *n)
{
	Snapshot **list;
	Snapshot *snap;
	int count;

	*n = 0;
	if(ts == nil)
		return nil;

	lock(ts);

	/* Count snapshots */
	count = 0;
	for(snap = ts->snapshots; snap != nil; snap = snap->next)
		count++;

	if(count == 0){
		unlock(ts);
		return nil;
	}

	list = mallocz(sizeof(Snapshot*) * count, 1);
	if(list == nil){
		unlock(ts);
		return nil;
	}

	count = 0;
	for(snap = ts->snapshots; snap != nil; snap = snap->next)
		list[count++] = snap;

	unlock(ts);

	*n = count;
	return list;
}

/* Get atoms that were active at a specific time */
Atom**
temporalactive(TemporalSpace *ts, vlong time, int *n)
{
	Atom **results;
	int i, count, maxresults;
	TemporalAtom *ta;

	*n = 0;
	if(ts == nil)
		return nil;

	/* Current time */
	if(time == 0 || time >= temporalnow()){
		*n = ts->current->natoms;
		return ts->current->atoms;
	}

	maxresults = 128;
	results = mallocz(sizeof(Atom*) * maxresults, 1);
	if(results == nil)
		return nil;

	count = 0;

	lock(ts);

	for(i = 0; i < ts->nhistory; i++){
		for(ta = ts->history[i]; ta != nil; ta = ta->next){
			if(ta->atom == nil)
				continue;

			/* Check if valid at requested time */
			if(ta->tstart <= time && (ta->tend == 0 || ta->tend > time)){
				if(count >= maxresults){
					maxresults *= 2;
					results = realloc(results, sizeof(Atom*) * maxresults);
					if(results == nil){
						unlock(ts);
						*n = 0;
						return nil;
					}
				}
				results[count++] = ta->atom;
				break;	/* Only include once per atom */
			}
		}
	}

	unlock(ts);

	*n = count;
	return results;
}

/* Get atoms changed since a specific time */
Atom**
temporalchanged(TemporalSpace *ts, vlong since, int *n)
{
	Atom **results;
	int i, count, maxresults;
	TemporalAtom *ta;

	*n = 0;
	if(ts == nil)
		return nil;

	maxresults = 128;
	results = mallocz(sizeof(Atom*) * maxresults, 1);
	if(results == nil)
		return nil;

	count = 0;

	lock(ts);

	for(i = 0; i < ts->nhistory; i++){
		for(ta = ts->history[i]; ta != nil; ta = ta->next){
			if(ta->atom == nil)
				continue;

			/* Check if modified since requested time */
			if(ta->tstart >= since){
				if(count >= maxresults){
					maxresults *= 2;
					results = realloc(results, sizeof(Atom*) * maxresults);
					if(results == nil){
						unlock(ts);
						*n = 0;
						return nil;
					}
				}

				/* Avoid duplicates */
				int j, dup = 0;
				for(j = 0; j < count; j++){
					if(results[j]->id == ta->atom->id){
						dup = 1;
						break;
					}
				}
				if(!dup)
					results[count++] = ta->atom;
			}
		}
	}

	unlock(ts);

	*n = count;
	return results;
}

/* Allen's interval relations */
int
temporalrelation(TimeInterval *a, TimeInterval *b)
{
	if(a == nil || b == nil)
		return -1;

	/* Before: a ends before b starts */
	if(a->end < b->start)
		return RelBefore;

	/* After: a starts after b ends */
	if(a->start > b->end)
		return RelAfter;

	/* Meets: a ends exactly when b starts */
	if(a->end == b->start)
		return RelMeets;

	/* Met by: b ends exactly when a starts */
	if(b->end == a->start)
		return RelMetBy;

	/* Equals: same interval */
	if(a->start == b->start && a->end == b->end)
		return RelEquals;

	/* Starts: same start, a ends first */
	if(a->start == b->start && a->end < b->end)
		return RelStarts;

	/* Started by: same start, b ends first */
	if(a->start == b->start && a->end > b->end)
		return RelStartedBy;

	/* Finishes: same end, a starts later */
	if(a->end == b->end && a->start > b->start)
		return RelFinishes;

	/* Finished by: same end, b starts later */
	if(a->end == b->end && a->start < b->start)
		return RelFinishedBy;

	/* During: a entirely within b */
	if(a->start > b->start && a->end < b->end)
		return RelDuring;

	/* Contains: b entirely within a */
	if(b->start > a->start && b->end < a->end)
		return RelContains;

	/* Overlaps: a starts before b, overlaps */
	if(a->start < b->start && a->end > b->start && a->end < b->end)
		return RelOverlaps;

	/* Overlapped by: b starts before a, overlaps */
	if(b->start < a->start && b->end > a->start && b->end < a->end)
		return RelOverlappedBy;

	return -1;
}

/* Time utilities */
vlong
temporalnow(void)
{
	return nsec();
}

char*
temporalstr(vlong t)
{
	static char buf[64];
	Tm *tm;
	vlong sec;

	sec = t / TimeSec;
	tm = localtime(sec);

	snprint(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
		tm->year + 1900, tm->mon + 1, tm->mday,
		tm->hour, tm->min, tm->sec);

	return buf;
}

vlong
temporalparse(char *s)
{
	Tm tm;
	int n;

	if(s == nil)
		return 0;

	/* Parse ISO-like format: YYYY-MM-DD HH:MM:SS */
	memset(&tm, 0, sizeof(tm));
	n = sscanf(s, "%d-%d-%d %d:%d:%d",
		&tm.year, &tm.mon, &tm.mday,
		&tm.hour, &tm.min, &tm.sec);

	if(n < 3)
		return 0;

	tm.year -= 1900;
	tm.mon -= 1;

	return (vlong)tm2sec(&tm) * TimeSec;
}

/* Export temporal space to directory */
int
temporalexport(TemporalSpace *ts, char *path)
{
	char filepath[512];
	int fd;

	if(ts == nil || path == nil)
		return -1;

	/* Export current state */
	snprint(filepath, sizeof(filepath), "%s/current", path);
	if(atomspaceexportfile(ts->current, filepath) < 0)
		return -1;

	/* Export snapshots */
	Snapshot *snap;
	for(snap = ts->snapshots; snap != nil; snap = snap->next){
		snprint(filepath, sizeof(filepath), "%s/snap_%s", path, snap->name);
		atomspaceexportfile(snap->state, filepath);
	}

	return 0;
}

/* Import temporal space from directory */
TemporalSpace*
temporalimport(char *path)
{
	TemporalSpace *ts;
	AtomSpace *current;
	char filepath[512];

	if(path == nil)
		return nil;

	/* Import current state */
	snprint(filepath, sizeof(filepath), "%s/current", path);
	current = atomspaceimportfile(filepath);

	if(current == nil)
		return nil;

	ts = temporalinit(current);
	return ts;
}
