/*
 * Cognitive Fusion Reactor - distributed cognitive processing
 */

#include <u.h>
#include <libc.h>
#include <plan9cog.h>

/* Task types for the reactor */
enum {
	TaskInference = 1,	/* PLN inference task */
	TaskPattern,		/* Pattern matching task */
	TaskEcan,		/* ECAN attention task */
	TaskMine,		/* Pattern mining task */
	TaskSync,		/* MachSpace sync task */
};

/* Reactor task structure */
typedef struct CogTask CogTask;
struct CogTask {
	int type;		/* Task type */
	void *data;		/* Task data */
	void *result;		/* Task result */
	int done;		/* Completion flag */
	Lock;			/* Task lock */
};

/* Task queue */
typedef struct TaskQueue TaskQueue;
struct TaskQueue {
	CogTask **tasks;	/* Task array */
	int ntasks;		/* Number of tasks */
	int maxtasks;		/* Maximum tasks */
	int head;		/* Queue head */
	int tail;		/* Queue tail */
	Lock;			/* Queue lock */
};

/* Global task queue */
static TaskQueue *globalqueue;

/* Initialize task queue */
static TaskQueue*
taskqueueinit(int size)
{
	TaskQueue *q;

	q = mallocz(sizeof(TaskQueue), 1);
	if(q == nil)
		return nil;

	q->maxtasks = size;
	q->tasks = mallocz(sizeof(CogTask*) * size, 1);
	if(q->tasks == nil){
		free(q);
		return nil;
	}

	q->ntasks = 0;
	q->head = 0;
	q->tail = 0;

	return q;
}

/* Free task queue */
static void
taskqueuefree(TaskQueue *q)
{
	int i;

	if(q == nil)
		return;

	for(i = 0; i < q->maxtasks; i++){
		if(q->tasks[i] != nil)
			free(q->tasks[i]);
	}
	free(q->tasks);
	free(q);
}

/* Enqueue task */
static int
taskqueuepush(TaskQueue *q, CogTask *task)
{
	if(q == nil || task == nil)
		return -1;

	lock(q);

	if(q->ntasks >= q->maxtasks){
		unlock(q);
		return -1;
	}

	q->tasks[q->tail] = task;
	q->tail = (q->tail + 1) % q->maxtasks;
	q->ntasks++;

	unlock(q);
	return 0;
}

/* Dequeue task */
static CogTask*
taskqueuepop(TaskQueue *q)
{
	CogTask *task;

	if(q == nil)
		return nil;

	lock(q);

	if(q->ntasks == 0){
		unlock(q);
		return nil;
	}

	task = q->tasks[q->head];
	q->tasks[q->head] = nil;
	q->head = (q->head + 1) % q->maxtasks;
	q->ntasks--;

	unlock(q);
	return task;
}

/* Process a single task */
static void
processtask(CogFusionReactor *cfr, CogTask *task)
{
	if(cfr == nil || task == nil)
		return;

	lock(task);

	switch(task->type){
	case TaskInference:
		/* Execute PLN inference */
		if(cfr->p9c != nil && cfr->p9c->pln != nil){
			Atom *target = (Atom*)task->data;
			int n;
			task->result = plnforward(cfr->p9c->pln, target, 10, &n);
		}
		break;

	case TaskPattern:
		/* Execute pattern matching */
		if(cfr->p9c != nil && cfr->p9c->atomspace != nil){
			Pattern *pat = (Pattern*)task->data;
			int n;
			task->result = atommatch(cfr->p9c->atomspace, pat, &n);
		}
		break;

	case TaskEcan:
		/* Execute ECAN update - result is focus atoms */
		/* Note: ECAN operations typically don't return results directly */
		task->result = nil;
		break;

	case TaskMine:
		/* Execute pattern mining */
		if(cfr->p9c != nil && cfr->p9c->atomspace != nil){
			PatternMiner *pm = patterninit(cfr->p9c->atomspace);
			if(pm != nil){
				patternmine(pm, 2);
				int n;
				task->result = patternget(pm, &n);
			}
		}
		break;

	case TaskSync:
		/* Execute MachSpace sync - no direct result */
		task->result = nil;
		break;
	}

	task->done = 1;
	unlock(task);
}

CogFusionReactor*
cogreactorinit(Plan9Cog *p9c, int nworkers)
{
	CogFusionReactor *cfr;

	cfr = mallocz(sizeof(CogFusionReactor), 1);
	if(cfr == nil)
		return nil;

	cfr->p9c = p9c;
	cfr->nworkers = nworkers;
	cfr->workers = mallocz(sizeof(void*) * nworkers, 1);
	if(cfr->workers == nil){
		free(cfr);
		return nil;
	}

	/* Initialize global task queue */
	if(globalqueue == nil){
		globalqueue = taskqueueinit(256);
	}

	/* In a real Plan 9 environment, we would use rfork or procs here */
	/* For now, we use synchronous task processing */

	return cfr;
}

void
cogreactorfree(CogFusionReactor *cfr)
{
	if(cfr == nil)
		return;

	lock(cfr);

	/* Free worker resources */
	free(cfr->workers);

	unlock(cfr);

	/* Free global queue if this is the last reactor */
	if(globalqueue != nil){
		taskqueuefree(globalqueue);
		globalqueue = nil;
	}

	free(cfr);
}

void
cogreactorsubmit(CogFusionReactor *cfr, void *task)
{
	CogTask *ctask;

	if(cfr == nil || task == nil)
		return;

	/* Create task wrapper */
	ctask = mallocz(sizeof(CogTask), 1);
	if(ctask == nil)
		return;

	ctask->type = TaskInference;  /* Default to inference */
	ctask->data = task;
	ctask->result = nil;
	ctask->done = 0;

	/* Enqueue task */
	if(globalqueue != nil){
		taskqueuepush(globalqueue, ctask);
	}

	/* Process synchronously for now */
	processtask(cfr, ctask);
}

void*
cogreactorresult(CogFusionReactor *cfr)
{
	CogTask *task;
	void *result;

	if(cfr == nil || globalqueue == nil)
		return nil;

	/* Get completed task */
	task = taskqueuepop(globalqueue);
	if(task == nil)
		return nil;

	/* Wait for completion */
	while(!task->done){
		sleep(1);
	}

	result = task->result;
	free(task);

	return result;
}

/* Submit specific task types */
void
cogreactorsubmitinference(CogFusionReactor *cfr, Atom *target)
{
	CogTask *ctask;

	if(cfr == nil)
		return;

	ctask = mallocz(sizeof(CogTask), 1);
	if(ctask == nil)
		return;

	ctask->type = TaskInference;
	ctask->data = target;
	ctask->done = 0;

	if(globalqueue != nil)
		taskqueuepush(globalqueue, ctask);

	processtask(cfr, ctask);
}

void
cogreactorsubmitpattern(CogFusionReactor *cfr, Pattern *pat)
{
	CogTask *ctask;

	if(cfr == nil)
		return;

	ctask = mallocz(sizeof(CogTask), 1);
	if(ctask == nil)
		return;

	ctask->type = TaskPattern;
	ctask->data = pat;
	ctask->done = 0;

	if(globalqueue != nil)
		taskqueuepush(globalqueue, ctask);

	processtask(cfr, ctask);
}

void
cogreactorsubmitmine(CogFusionReactor *cfr)
{
	CogTask *ctask;

	if(cfr == nil)
		return;

	ctask = mallocz(sizeof(CogTask), 1);
	if(ctask == nil)
		return;

	ctask->type = TaskMine;
	ctask->data = nil;
	ctask->done = 0;

	if(globalqueue != nil)
		taskqueuepush(globalqueue, ctask);

	processtask(cfr, ctask);
}

/* Get reactor statistics */
typedef struct ReactorStats ReactorStats;
struct ReactorStats {
	int pending;		/* Pending tasks */
	int completed;		/* Completed tasks */
	int workers;		/* Active workers */
};

void
cogreactorstats(CogFusionReactor *cfr, ReactorStats *stats)
{
	if(cfr == nil || stats == nil)
		return;

	stats->workers = cfr->nworkers;
	stats->pending = globalqueue != nil ? globalqueue->ntasks : 0;
	stats->completed = 0;  /* Would need to track this */
}
