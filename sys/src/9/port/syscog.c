/*
 * Cognitive System Calls
 *
 * Implements kernel system calls for cognitive operations.
 * These system calls provide direct access to the cognitive
 * subsystem from user processes, enabling:
 *
 *   COGTHINK   - Perform cognitive operation
 *   COGWAIT    - Wait for cognitive event
 *   COGINFER   - Trigger inference
 *   COGFOCUS   - Set attentional focus
 *   COGSPREAD  - Spread activation
 *
 * This makes cognitive processing a first-class OS primitive,
 * not an application layered on top of the kernel.
 *
 * Implementation Note:
 *   This is an initial implementation that establishes the syscall
 *   infrastructure and basic cognitive state management. Operation-
 *   specific arguments are extracted but not fully utilized in this
 *   phase. Future enhancements will add full operation dispatch to
 *   the kernel AtomSpace, PLN inference engine, and ECAN system.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

/*
 * syscogthink - perform cognitive operation
 *
 * Executes a cognitive operation on the current process.
 * Operations include: think, reason, learn, perceive.
 *
 * Arguments:
 *   op    - operation code (COGnop, COGcreate, COGlink, etc.)
 *   arg1  - first argument (reserved for future operation-specific use)
 *   arg2  - second argument (reserved for future operation-specific use)
 *   data  - operation data pointer (reserved for future use)
 *
 * Returns:
 *   0 on success, -1 on error
 *
 * Note: Arguments are extracted but cognitive state transitions are
 * uniform in this phase. Full operation dispatch will be added in
 * Phase 3 when kernel AtomSpace operations are implemented.
 */
long
syscogthink(ulong *arg)
{
	int op;
	int arg1;
	int arg2;
	void *data;
	CogProcExt *ce;

	op = arg[0];
	arg1 = arg[1];
	arg2 = arg[2];
	data = (void*)arg[3];

	/* Reserved for future operation-specific handling */
	USED(arg1);
	USED(arg2);
	USED(data);

	/* Get cognitive extension for current process */
	ce = up->cogext;
	if(ce == nil)
		error("no cognitive extension");

	/*
	 * Execute cognitive operation.
	 * 
	 * In this initial implementation, operations are dispatched to
	 * cognitive state transition functions (cogthink, coginfer, coglearn).
	 * This tracks cognitive cycles and updates process state.
	 * 
	 * Future phases will add operation-specific logic using arg1, arg2,
	 * and data to interact with kernel AtomSpace, PLN, and ECAN.
	 */
	switch(op) {
	case COGnop:
		/* No operation */
		break;

	case COGcreate:
		/* Create atom in process's cognitive context */
		/* TODO: Use arg1 as atom type, data as atom name */
		cogthink(ce);
		break;

	case COGlink:
		/* Link atoms */
		/* TODO: Use arg1, arg2 as atom IDs, data as link type */
		cogthink(ce);
		break;

	case COGquery:
		/* Query AtomSpace */
		/* TODO: Use arg1 as query type, data as query pattern */
		cogthink(ce);
		break;

	case COGinfer:
		/* Perform inference */
		/* TODO: Use arg1 as rule type, arg2 as depth limit */
		coginfer(ce);
		break;

	case COGfocus:
		/* Update attention */
		/* TODO: Use arg1 as atom ID to focus on */
		cogthink(ce);
		break;

	case COGspread:
		/* Spread activation */
		/* TODO: Use arg1 as source atom, arg2 as spread amount */
		cogthink(ce);
		break;

	case COGpattern:
		/* Pattern match */
		/* TODO: Use data as pattern specification */
		cogthink(ce);
		break;

	case COGmine:
		/* Pattern mining */
		/* TODO: Use arg1 as minimum support threshold */
		cogthink(ce);
		break;

	case COGreason:
		/* Symbolic reasoning */
		/* TODO: Use arg1 as reasoning strategy */
		coginfer(ce);
		break;

	case COGlearn:
		/* Learning operation */
		/* TODO: Use arg1 as learning type, data as training info */
		coglearn(ce);
		break;

	case COGhalt:
		/* Halt cognitive processing */
		break;

	default:
		error("unknown cognitive operation");
	}

	return 0;
}

/*
 * syscogwait - wait for cognitive event
 *
 * Blocks the calling process until a cognitive event occurs.
 * Events include: inference completion, attention threshold,
 * pattern match, etc.
 *
 * Returns:
 *   Event type on success, -1 on error
 */
long
syscogwait(ulong *arg)
{
	CogProcExt *ce;

	USED(arg);

	/* Get cognitive extension for current process */
	ce = up->cogext;
	if(ce == nil)
		error("no cognitive extension");

	/* Enter waiting state */
	lock(ce);
	ce->cogstate = CogWaiting;
	unlock(ce);

	/*
	 * In a full implementation, this would use a Rendez
	 * to sleep until a cognitive event occurs.
	 * For now, just yield to allow other processes to run.
	 */
	sched();

	/* Return to thinking state */
	lock(ce);
	ce->cogstate = CogThinking;
	unlock(ce);

	return 0;
}

/*
 * syscoginfer - trigger inference
 *
 * Performs PLN inference on the specified atoms.
 *
 * Arguments:
 *   rule   - inference rule to apply
 *   atoms  - array of atom IDs
 *   natoms - number of atoms
 *
 * Returns:
 *   Number of inferences performed, -1 on error
 */
long
syscoginfer(ulong *arg)
{
	int rule;
	ulong *atoms;
	int natoms;
	CogProcExt *ce;

	rule = arg[0];
	atoms = (ulong*)arg[1];
	natoms = arg[2];

	USED(rule);
	USED(atoms);
	USED(natoms);

	/* Get cognitive extension for current process */
	ce = up->cogext;
	if(ce == nil)
		error("no cognitive extension");

	/* Perform inference */
	coginfer(ce);

	return 1;	/* One inference performed */
}

/*
 * syscogfocus - set attentional focus
 *
 * Updates the attentional focus to the specified atom.
 * This affects cognitive scheduling and processing priority.
 *
 * Arguments:
 *   atomid - atom ID to focus on
 *
 * Returns:
 *   0 on success, -1 on error
 */
long
syscogfocus(ulong *arg)
{
	ulong atomid;
	CogProcExt *ce;

	atomid = arg[0];

	/* Get cognitive extension for current process */
	ce = up->cogext;
	if(ce == nil)
		error("no cognitive extension");

	/* Update process's focus atom */
	lock(ce);
	ce->atomid = atomid;
	/* Boost STI for focused processing */
	ce->sti += 10;
	if(ce->sti > 32767)
		ce->sti = 32767;
	unlock(ce);

	return 0;
}

/*
 * syscogspread - spread activation
 *
 * Spreads activation from one atom to connected atoms.
 * This implements ECAN's spreading activation mechanism.
 *
 * Arguments:
 *   atomid - source atom ID
 *   amount - activation amount to spread
 *
 * Returns:
 *   Number of atoms affected, -1 on error
 */
long
syscogspread(ulong *arg)
{
	ulong atomid;
	short amount;
	CogProcExt *ce;

	atomid = arg[0];
	amount = (short)arg[1];

	USED(atomid);

	/* Get cognitive extension for current process */
	ce = up->cogext;
	if(ce == nil)
		error("no cognitive extension");

	/* Update attention values */
	cogupdate(ce, amount, 0);

	/* Count as cognitive cycle */
	cogthink(ce);

	return 1;	/* Simplified: one atom affected */
}
