/*
 * OpenPsi - Goal-directed behavior system for Plan9Cog
 *
 * Implements OpenCog's OpenPsi framework for motivational
 * behavior using Plan 9's file-based interfaces.
 *
 * Architecture:
 *   /mnt/psi/urges/     - Basic drives (curiosity, competence, etc.)
 *   /mnt/psi/goals/     - Active goals
 *   /mnt/psi/demands/   - Unsatisfied demands
 *   /mnt/psi/modulators/- Global modulation parameters
 */

#pragma once

#include <plan9cog/atomspace.h>

/* Urge types - basic drives */
enum {
	UrgeNone = 0,
	UrgeCuriosity,		/* Drive to explore/learn */
	UrgeCompetence,		/* Drive to achieve/succeed */
	UrgeAffiliation,	/* Drive for social connection */
	UrgeAvoidance,		/* Drive to avoid harm */
	UrgeEnergy,		/* Drive to maintain resources */
	UrgeCertainty,		/* Drive to reduce uncertainty */
	UrgeMax,
};

/* Goal status */
enum {
	GoalPending = 0,
	GoalPursuing,
	GoalAchieved,
	GoalAbandoned,
	GoalFailed,
};

/* Modulator types */
enum {
	ModArousal = 0,		/* Activation level */
	ModValence,		/* Positive/negative affect */
	ModResolution,		/* Decision threshold */
	ModSelection,		/* Action selection threshold */
	ModSecuring,		/* Goal securing threshold */
	ModMax,
};

/* Urge structure */
typedef struct PsiUrge PsiUrge;
struct PsiUrge {
	int	type;		/* Urge type */
	char	*name;		/* Urge name */
	float	level;		/* Current satisfaction (0-1) */
	float	target;		/* Target satisfaction level */
	float	weight;		/* Importance weight */
	float	decay;		/* Decay rate per update */
	Atom	*demand;	/* Associated demand atom */
	Lock;
};

/* Goal structure */
typedef struct PsiGoal PsiGoal;
struct PsiGoal {
	ulong	id;		/* Goal ID */
	Atom	*atom;		/* Goal atom in AtomSpace */
	float	priority;	/* Priority (0-1) */
	float	urgency;	/* Urgency (0-1) */
	int	status;		/* Goal status */
	Atom	**plan;		/* Action sequence */
	int	nplan;		/* Plan length */
	int	planidx;	/* Current plan step */
	PsiUrge	*urge;		/* Associated urge */
	vlong	created;	/* Creation time */
	vlong	deadline;	/* Deadline (0 = none) */
	Lock;
};

/* Demand structure - link between urge and goal */
typedef struct PsiDemand PsiDemand;
struct PsiDemand {
	PsiUrge	*urge;		/* Source urge */
	PsiGoal	*goal;		/* Target goal */
	float	strength;	/* Demand strength */
	Atom	*atom;		/* Demand atom */
};

/* Action structure */
typedef struct PsiAction PsiAction;
struct PsiAction {
	char	*name;		/* Action name */
	Atom	*atom;		/* Action atom */
	float	cost;		/* Estimated cost */
	float	utility;	/* Expected utility */
	int	(*execute)(PsiAction*, void*);	/* Execution function */
	void	*arg;		/* Execution argument */
};

/* OpenPsi system context */
typedef struct OpenPsi OpenPsi;
struct OpenPsi {
	AtomSpace	*as;		/* Knowledge base */

	/* Urges */
	PsiUrge		**urges;
	int		nurges;

	/* Goals */
	PsiGoal		**goals;
	int		ngoals;
	int		maxgoals;

	/* Demands */
	PsiDemand	**demands;
	int		ndemands;

	/* Actions */
	PsiAction	**actions;
	int		nactions;

	/* Modulators */
	float		modulators[ModMax];

	/* Configuration */
	float		goalthreshold;	/* Min priority for goal selection */
	float		urgethreshold;	/* Min urge level for demand */
	int		maxactive;	/* Max concurrent active goals */

	/* Statistics */
	ulong		cycles;		/* Update cycles */
	ulong		goalsset;	/* Goals created */
	ulong		goalsmet;	/* Goals achieved */
	ulong		goalsfailed;	/* Goals failed */

	Lock;
};

/* Initialization */
OpenPsi*	psiinit(AtomSpace *as);
void		psifree(OpenPsi *psi);

/* Urge management */
PsiUrge*	psiurgecreate(OpenPsi *psi, int type, char *name);
void		psiurgefree(PsiUrge *urge);
void		psiurgeupdate(OpenPsi *psi, PsiUrge *urge, float delta);
void		psiurgestimulate(OpenPsi *psi, int urgetype, float amount);
float		psiurgelevel(OpenPsi *psi, int urgetype);
void		psiurgedecay(OpenPsi *psi);

/* Goal management */
PsiGoal*	psigoalcreate(OpenPsi *psi, Atom *atom, float priority);
void		psigoalfree(PsiGoal *goal);
void		psigoalsetpriority(PsiGoal *goal, float priority);
void		psigoalsetstatus(PsiGoal *goal, int status);
PsiGoal*	psigoalselect(OpenPsi *psi);
int		psigoalstep(OpenPsi *psi, PsiGoal *goal);

/* Demand management */
PsiDemand*	psidemandcreate(OpenPsi *psi, PsiUrge *urge, PsiGoal *goal);
void		psidemandfree(PsiDemand *demand);
void		psidemandupdate(OpenPsi *psi);

/* Action management */
PsiAction*	psiactioncreate(OpenPsi *psi, char *name, int (*fn)(PsiAction*, void*));
void		psiactionfree(PsiAction *action);
int		psiactionexecute(OpenPsi *psi, PsiAction *action);

/* Modulator management */
void		psimodset(OpenPsi *psi, int modtype, float value);
float		psimodget(OpenPsi *psi, int modtype);

/* Main update cycle */
int		psiupdate(OpenPsi *psi);

/* Utility functions */
char*		psiurgename(int type);
char*		psigoalstatus(int status);
char*		psimodname(int type);

/* Statistics */
typedef struct PsiStats PsiStats;
struct PsiStats {
	ulong	cycles;
	ulong	goalsset;
	ulong	goalsmet;
	ulong	goalsfailed;
	int	activegoals;
	float	avgurge;
};

void		psistats(OpenPsi *psi, PsiStats *stats);
