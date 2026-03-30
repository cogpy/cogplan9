/*
 * OpenPsi - Goal-directed behavior system for Plan9Cog
 *
 * Implements OpenCog's motivational framework using Plan 9
 * architectural principles.
 */

#include <u.h>
#include <libc.h>
#include <plan9cog.h>
#include <plan9cog/openpsi.h>

static char *urgenames[] = {
	[UrgeNone]		"none",
	[UrgeCuriosity]		"curiosity",
	[UrgeCompetence]	"competence",
	[UrgeAffiliation]	"affiliation",
	[UrgeAvoidance]		"avoidance",
	[UrgeEnergy]		"energy",
	[UrgeCertainty]		"certainty",
};

static char *modnames[] = {
	[ModArousal]	"arousal",
	[ModValence]	"valence",
	[ModResolution]	"resolution",
	[ModSelection]	"selection",
	[ModSecuring]	"securing",
};

static char *statusnames[] = {
	[GoalPending]	"pending",
	[GoalPursuing]	"pursuing",
	[GoalAchieved]	"achieved",
	[GoalAbandoned]	"abandoned",
	[GoalFailed]	"failed",
};

static ulong nextgoalid = 1;

OpenPsi*
psiinit(AtomSpace *as)
{
	OpenPsi *psi;
	int i;

	psi = mallocz(sizeof(OpenPsi), 1);
	if(psi == nil)
		return nil;

	psi->as = as;

	/* Initialize urge array */
	psi->urges = nil;
	psi->nurges = 0;

	/* Initialize goals */
	psi->maxgoals = 64;
	psi->goals = mallocz(sizeof(PsiGoal*) * psi->maxgoals, 1);
	if(psi->goals == nil){
		free(psi);
		return nil;
	}
	psi->ngoals = 0;

	/* Initialize demands */
	psi->demands = nil;
	psi->ndemands = 0;

	/* Initialize actions */
	psi->actions = nil;
	psi->nactions = 0;

	/* Default modulators */
	psi->modulators[ModArousal] = 0.5;
	psi->modulators[ModValence] = 0.5;
	psi->modulators[ModResolution] = 0.5;
	psi->modulators[ModSelection] = 0.5;
	psi->modulators[ModSecuring] = 0.5;

	/* Configuration */
	psi->goalthreshold = 0.3;
	psi->urgethreshold = 0.4;
	psi->maxactive = 5;

	/* Statistics */
	psi->cycles = 0;
	psi->goalsset = 0;
	psi->goalsmet = 0;
	psi->goalsfailed = 0;

	/* Create default urges */
	psiurgecreate(psi, UrgeCuriosity, "curiosity");
	psiurgecreate(psi, UrgeCompetence, "competence");
	psiurgecreate(psi, UrgeCertainty, "certainty");

	return psi;
}

void
psifree(OpenPsi *psi)
{
	int i;

	if(psi == nil)
		return;

	lock(psi);

	/* Free urges */
	for(i = 0; i < psi->nurges; i++){
		if(psi->urges[i])
			psiurgefree(psi->urges[i]);
	}
	free(psi->urges);

	/* Free goals */
	for(i = 0; i < psi->ngoals; i++){
		if(psi->goals[i])
			psigoalfree(psi->goals[i]);
	}
	free(psi->goals);

	/* Free demands */
	for(i = 0; i < psi->ndemands; i++){
		if(psi->demands[i])
			psidemandfree(psi->demands[i]);
	}
	free(psi->demands);

	/* Free actions */
	for(i = 0; i < psi->nactions; i++){
		if(psi->actions[i])
			psiactionfree(psi->actions[i]);
	}
	free(psi->actions);

	unlock(psi);
	free(psi);
}

/* Urge management */

PsiUrge*
psiurgecreate(OpenPsi *psi, int type, char *name)
{
	PsiUrge *urge;
	PsiUrge **newurges;

	if(psi == nil)
		return nil;

	urge = mallocz(sizeof(PsiUrge), 1);
	if(urge == nil)
		return nil;

	urge->type = type;
	urge->name = strdup(name ? name : urgenames[type]);
	urge->level = 0.5;		/* Start neutral */
	urge->target = 0.8;		/* Target satisfaction */
	urge->weight = 1.0;
	urge->decay = 0.01;		/* Slow decay */
	urge->demand = nil;

	/* Create demand atom in AtomSpace */
	if(psi->as != nil){
		char demandname[128];
		snprint(demandname, sizeof(demandname), "urge:%s", urge->name);
		urge->demand = atomcreate(psi->as, ConceptNode, demandname);
		if(urge->demand != nil){
			TruthValue tv;
			tv.strength = urge->level;
			tv.confidence = 0.9;
			tv.count = 1;
			atomsettruth(urge->demand, tv);
		}
	}

	/* Add to psi */
	lock(psi);
	newurges = realloc(psi->urges, sizeof(PsiUrge*) * (psi->nurges + 1));
	if(newurges != nil){
		psi->urges = newurges;
		psi->urges[psi->nurges++] = urge;
	}
	unlock(psi);

	return urge;
}

void
psiurgefree(PsiUrge *urge)
{
	if(urge == nil)
		return;

	free(urge->name);
	free(urge);
}

void
psiurgeupdate(OpenPsi *psi, PsiUrge *urge, float delta)
{
	if(urge == nil)
		return;

	lock(urge);

	urge->level += delta;
	if(urge->level < 0.0)
		urge->level = 0.0;
	if(urge->level > 1.0)
		urge->level = 1.0;

	/* Update demand atom truth value */
	if(urge->demand != nil){
		TruthValue tv = atomgettruth(urge->demand);
		tv.strength = urge->level;
		tv.count++;
		atomsettruth(urge->demand, tv);
	}

	unlock(urge);
}

void
psiurgestimulate(OpenPsi *psi, int urgetype, float amount)
{
	int i;

	if(psi == nil)
		return;

	lock(psi);
	for(i = 0; i < psi->nurges; i++){
		if(psi->urges[i] && psi->urges[i]->type == urgetype){
			psiurgeupdate(psi, psi->urges[i], amount);
			break;
		}
	}
	unlock(psi);
}

float
psiurgelevel(OpenPsi *psi, int urgetype)
{
	int i;
	float level = 0.0;

	if(psi == nil)
		return 0.0;

	lock(psi);
	for(i = 0; i < psi->nurges; i++){
		if(psi->urges[i] && psi->urges[i]->type == urgetype){
			level = psi->urges[i]->level;
			break;
		}
	}
	unlock(psi);

	return level;
}

void
psiurgedecay(OpenPsi *psi)
{
	int i;

	if(psi == nil)
		return;

	lock(psi);
	for(i = 0; i < psi->nurges; i++){
		if(psi->urges[i] == nil)
			continue;

		/* Decay toward 0.5 (neutral) */
		float diff = psi->urges[i]->level - 0.5;
		psi->urges[i]->level -= diff * psi->urges[i]->decay;
	}
	unlock(psi);
}

/* Goal management */

PsiGoal*
psigoalcreate(OpenPsi *psi, Atom *atom, float priority)
{
	PsiGoal *goal;
	PsiGoal **newgoals;

	if(psi == nil)
		return nil;

	goal = mallocz(sizeof(PsiGoal), 1);
	if(goal == nil)
		return nil;

	goal->id = nextgoalid++;
	goal->atom = atom;
	goal->priority = priority;
	goal->urgency = 0.5;
	goal->status = GoalPending;
	goal->plan = nil;
	goal->nplan = 0;
	goal->planidx = 0;
	goal->urge = nil;
	goal->created = nsec();
	goal->deadline = 0;

	/* Add to psi */
	lock(psi);
	if(psi->ngoals >= psi->maxgoals){
		psi->maxgoals *= 2;
		newgoals = realloc(psi->goals, sizeof(PsiGoal*) * psi->maxgoals);
		if(newgoals == nil){
			unlock(psi);
			free(goal);
			return nil;
		}
		psi->goals = newgoals;
	}
	psi->goals[psi->ngoals++] = goal;
	psi->goalsset++;
	unlock(psi);

	return goal;
}

void
psigoalfree(PsiGoal *goal)
{
	if(goal == nil)
		return;

	free(goal->plan);
	free(goal);
}

void
psigoalsetpriority(PsiGoal *goal, float priority)
{
	if(goal == nil)
		return;

	lock(goal);
	goal->priority = priority;
	if(goal->priority < 0.0)
		goal->priority = 0.0;
	if(goal->priority > 1.0)
		goal->priority = 1.0;
	unlock(goal);
}

void
psigoalsetstatus(PsiGoal *goal, int status)
{
	if(goal == nil)
		return;

	lock(goal);
	goal->status = status;
	unlock(goal);
}

/*
 * Select best goal to pursue based on:
 * - Priority
 * - Urgency (from associated urge)
 * - Modulator values
 * - Current active goal count
 */
PsiGoal*
psigoalselect(OpenPsi *psi)
{
	int i, nactive;
	PsiGoal *best = nil;
	float bestscore = 0.0;

	if(psi == nil)
		return nil;

	lock(psi);

	/* Count active goals */
	nactive = 0;
	for(i = 0; i < psi->ngoals; i++){
		if(psi->goals[i] && psi->goals[i]->status == GoalPursuing)
			nactive++;
	}

	/* Don't exceed max active */
	if(nactive >= psi->maxactive){
		unlock(psi);
		return nil;
	}

	/* Find best pending goal */
	for(i = 0; i < psi->ngoals; i++){
		PsiGoal *g = psi->goals[i];
		if(g == nil || g->status != GoalPending)
			continue;

		/* Calculate goal score */
		float score = g->priority;

		/* Add urgency from associated urge */
		if(g->urge != nil)
			score += g->urge->level * 0.5;

		/* Modulate by arousal and selection threshold */
		score *= psi->modulators[ModArousal];
		score *= psi->modulators[ModSelection];

		/* Check threshold */
		if(score < psi->goalthreshold)
			continue;

		if(score > bestscore){
			bestscore = score;
			best = g;
		}
	}

	/* Start pursuing the best goal */
	if(best != nil)
		best->status = GoalPursuing;

	unlock(psi);
	return best;
}

/*
 * Execute one step of a goal's plan
 * Returns: 1 if complete, 0 if continuing, -1 on error
 */
int
psigoalstep(OpenPsi *psi, PsiGoal *goal)
{
	if(psi == nil || goal == nil)
		return -1;

	if(goal->status != GoalPursuing)
		return -1;

	lock(goal);

	/* No plan or plan complete */
	if(goal->plan == nil || goal->planidx >= goal->nplan){
		goal->status = GoalAchieved;
		unlock(goal);

		lock(psi);
		psi->goalsmet++;
		unlock(psi);

		return 1;
	}

	/* Execute current plan step (would call action here) */
	goal->planidx++;

	/* Check if complete */
	if(goal->planidx >= goal->nplan){
		goal->status = GoalAchieved;
		unlock(goal);

		lock(psi);
		psi->goalsmet++;
		unlock(psi);

		return 1;
	}

	unlock(goal);
	return 0;
}

/* Demand management */

PsiDemand*
psidemandcreate(OpenPsi *psi, PsiUrge *urge, PsiGoal *goal)
{
	PsiDemand *demand;
	PsiDemand **newdemands;

	if(psi == nil || urge == nil || goal == nil)
		return nil;

	demand = mallocz(sizeof(PsiDemand), 1);
	if(demand == nil)
		return nil;

	demand->urge = urge;
	demand->goal = goal;
	demand->strength = urge->level * goal->priority;
	demand->atom = nil;

	/* Link goal to urge */
	goal->urge = urge;

	/* Add to psi */
	lock(psi);
	newdemands = realloc(psi->demands, sizeof(PsiDemand*) * (psi->ndemands + 1));
	if(newdemands != nil){
		psi->demands = newdemands;
		psi->demands[psi->ndemands++] = demand;
	}
	unlock(psi);

	return demand;
}

void
psidemandfree(PsiDemand *demand)
{
	if(demand == nil)
		return;
	free(demand);
}

void
psidemandupdate(OpenPsi *psi)
{
	int i;

	if(psi == nil)
		return;

	lock(psi);
	for(i = 0; i < psi->ndemands; i++){
		PsiDemand *d = psi->demands[i];
		if(d == nil)
			continue;

		/* Update strength based on urge level */
		if(d->urge != nil && d->goal != nil){
			d->strength = d->urge->level * d->goal->priority;

			/* Boost priority if urge is high */
			if(d->urge->level > psi->urgethreshold){
				d->goal->urgency = d->urge->level;
			}
		}
	}
	unlock(psi);
}

/* Action management */

PsiAction*
psiactioncreate(OpenPsi *psi, char *name, int (*fn)(PsiAction*, void*))
{
	PsiAction *action;
	PsiAction **newactions;

	if(psi == nil || name == nil)
		return nil;

	action = mallocz(sizeof(PsiAction), 1);
	if(action == nil)
		return nil;

	action->name = strdup(name);
	action->atom = nil;
	action->cost = 0.1;
	action->utility = 0.5;
	action->execute = fn;
	action->arg = nil;

	/* Add to psi */
	lock(psi);
	newactions = realloc(psi->actions, sizeof(PsiAction*) * (psi->nactions + 1));
	if(newactions != nil){
		psi->actions = newactions;
		psi->actions[psi->nactions++] = action;
	}
	unlock(psi);

	return action;
}

void
psiactionfree(PsiAction *action)
{
	if(action == nil)
		return;
	free(action->name);
	free(action);
}

int
psiactionexecute(OpenPsi *psi, PsiAction *action)
{
	if(psi == nil || action == nil)
		return -1;

	if(action->execute != nil)
		return action->execute(action, action->arg);

	return 0;
}

/* Modulator management */

void
psimodset(OpenPsi *psi, int modtype, float value)
{
	if(psi == nil || modtype < 0 || modtype >= ModMax)
		return;

	lock(psi);
	psi->modulators[modtype] = value;
	if(psi->modulators[modtype] < 0.0)
		psi->modulators[modtype] = 0.0;
	if(psi->modulators[modtype] > 1.0)
		psi->modulators[modtype] = 1.0;
	unlock(psi);
}

float
psimodget(OpenPsi *psi, int modtype)
{
	float value = 0.0;

	if(psi == nil || modtype < 0 || modtype >= ModMax)
		return 0.0;

	lock(psi);
	value = psi->modulators[modtype];
	unlock(psi);

	return value;
}

/*
 * Main update cycle - called periodically
 *
 * 1. Decay urges toward neutral
 * 2. Update demand strengths
 * 3. Select and pursue goals
 * 4. Execute plan steps for active goals
 */
int
psiupdate(OpenPsi *psi)
{
	int i;
	PsiGoal *goal;

	if(psi == nil)
		return -1;

	psi->cycles++;

	/* Decay urges */
	psiurgedecay(psi);

	/* Update demands */
	psidemandupdate(psi);

	/* Select new goals if capacity available */
	goal = psigoalselect(psi);

	/* Step active goals */
	lock(psi);
	for(i = 0; i < psi->ngoals; i++){
		PsiGoal *g = psi->goals[i];
		if(g == nil || g->status != GoalPursuing)
			continue;

		unlock(psi);
		psigoalstep(psi, g);
		lock(psi);
	}
	unlock(psi);

	return 0;
}

/* Utility functions */

char*
psiurgename(int type)
{
	if(type < 0 || type >= UrgeMax)
		return "unknown";
	return urgenames[type];
}

char*
psigoalstatus(int status)
{
	if(status < 0 || status > GoalFailed)
		return "unknown";
	return statusnames[status];
}

char*
psimodname(int type)
{
	if(type < 0 || type >= ModMax)
		return "unknown";
	return modnames[type];
}

void
psistats(OpenPsi *psi, PsiStats *stats)
{
	int i, nactive;
	float totalurge;

	if(psi == nil || stats == nil)
		return;

	memset(stats, 0, sizeof(PsiStats));

	lock(psi);

	stats->cycles = psi->cycles;
	stats->goalsset = psi->goalsset;
	stats->goalsmet = psi->goalsmet;
	stats->goalsfailed = psi->goalsfailed;

	/* Count active goals */
	nactive = 0;
	for(i = 0; i < psi->ngoals; i++){
		if(psi->goals[i] && psi->goals[i]->status == GoalPursuing)
			nactive++;
	}
	stats->activegoals = nactive;

	/* Average urge level */
	totalurge = 0.0;
	for(i = 0; i < psi->nurges; i++){
		if(psi->urges[i])
			totalurge += psi->urges[i]->level;
	}
	if(psi->nurges > 0)
		stats->avgurge = totalurge / psi->nurges;

	unlock(psi);
}
