/*
 * Temporal Reasoning for Plan9Cog
 *
 * Provides time-aware AtomSpace operations leveraging
 * Plan 9's versioned namespace capabilities.
 *
 * Architecture:
 *   /mnt/cog/temporal/now/    - Current state
 *   /mnt/cog/temporal/t-1h/   - One hour ago
 *   /mnt/cog/temporal/snap/   - Named snapshots
 */

#pragma once

#include <plan9cog/atomspace.h>

/* Time granularity */
enum {
	TimeNanosec = 1,
	TimeMicrosec = 1000,
	TimeMillisec = 1000000,
	TimeSec = 1000000000LL,
	TimeMin = 60 * TimeSec,
	TimeHour = 60 * TimeMin,
	TimeDay = 24 * TimeHour,
};

/* Temporal atom - atom with time validity */
typedef struct TemporalAtom TemporalAtom;
struct TemporalAtom {
	Atom		*atom;		/* Base atom */
	vlong		tstart;		/* Valid from (nsec) */
	vlong		tend;		/* Valid until (0 = current) */
	TruthValue	tvat;		/* Truth value at creation */
	TemporalAtom	*prev;		/* Previous version */
	TemporalAtom	*next;		/* Next version */
};

/* Temporal interval */
typedef struct TimeInterval TimeInterval;
struct TimeInterval {
	vlong	start;
	vlong	end;
};

/* Temporal relation types */
enum {
	RelBefore = 0,		/* A entirely before B */
	RelAfter,		/* A entirely after B */
	RelMeets,		/* A ends where B starts */
	RelMetBy,		/* A starts where B ends */
	RelOverlaps,		/* A starts before B, ends during B */
	RelOverlappedBy,	/* B starts before A, ends during A */
	RelDuring,		/* A entirely within B */
	RelContains,		/* B entirely within A */
	RelStarts,		/* A and B start together, A ends first */
	RelStartedBy,		/* A and B start together, B ends first */
	RelFinishes,		/* A and B end together, A starts later */
	RelFinishedBy,		/* A and B end together, B starts later */
	RelEquals,		/* A and B same interval */
};

/* Snapshot - named point-in-time state */
typedef struct Snapshot Snapshot;
struct Snapshot {
	char		*name;		/* Snapshot name */
	vlong		time;		/* Snapshot time */
	AtomSpace	*state;		/* Captured state */
	Snapshot	*next;		/* Next snapshot */
};

/* Temporal AtomSpace */
typedef struct TemporalSpace TemporalSpace;
struct TemporalSpace {
	AtomSpace	*current;	/* Current state */
	TemporalAtom	**history;	/* Atom history */
	int		nhistory;
	int		maxhistory;
	Snapshot	*snapshots;	/* Named snapshots */
	vlong		granularity;	/* Time resolution */
	vlong		retention;	/* How long to keep history */
	char		*basepath;	/* Mount point base */
	Lock;
};

/* Initialization */
TemporalSpace*	temporalinit(AtomSpace *current);
void		temporalfree(TemporalSpace *ts);

/* Time-aware atom operations */
Atom*		temporalcreate(TemporalSpace *ts, int type, char *name);
Atom*		temporalfindat(TemporalSpace *ts, ulong id, vlong time);
TruthValue	temporaltvat(TemporalSpace *ts, Atom *a, vlong time);
int		temporaldelete(TemporalSpace *ts, ulong id);

/* History management */
int		temporalrecord(TemporalSpace *ts, Atom *a);
TemporalAtom**	temporalhistory(TemporalSpace *ts, ulong id, int *n);
void		temporalprune(TemporalSpace *ts, vlong before);

/* Snapshot management */
Snapshot*	temporalsnap(TemporalSpace *ts, char *name);
AtomSpace*	temporalrestore(TemporalSpace *ts, char *name);
int		temporaldeletesnap(TemporalSpace *ts, char *name);
Snapshot**	temporalsnapslist(TemporalSpace *ts, int *n);

/* Temporal queries */
Atom**		temporalactive(TemporalSpace *ts, vlong time, int *n);
Atom**		temporalchanged(TemporalSpace *ts, vlong since, int *n);
int		temporalrelation(TimeInterval *a, TimeInterval *b);

/* Time utilities */
vlong		temporalnow(void);
char*		temporalstr(vlong t);
vlong		temporalparse(char *s);

/* Persistence */
int		temporalexport(TemporalSpace *ts, char *path);
TemporalSpace*	temporalimport(char *path);
