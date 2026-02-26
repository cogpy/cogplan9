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
 *   arg1  - first argument (operation-specific)
 *   arg2  - second argument (operation-specific)
 *   data  - operation data pointer
 *
 * Returns:
 *   0 on success, -1 on error
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

	USED(arg1);
	USED(arg2);
	USED(data);

	/* Get cognitive extension for current process */
	ce = up->cogext;
	if(ce == nil)
		error("no cognitive extension");

	/* Execute cognitive operation */
	switch(op) {
	case COGnop:
		/* No operation */
		break;

	case COGcreate:
		/* Create atom in process's cognitive context */
		cogthink(ce);
		break;

	case COGlink:
		/* Link atoms */
		cogthink(ce);
		break;

	case COGquery:
		/* Query AtomSpace */
		cogthink(ce);
		break;

	case COGinfer:
		/* Perform inference */
		coginfer(ce);
		break;

	case COGfocus:
		/* Update attention */
		cogthink(ce);
		break;

	case COGspread:
		/* Spread activation */
		cogthink(ce);
		break;

	case COGpattern:
		/* Pattern match */
		cogthink(ce);
		break;

	case COGmine:
		/* Pattern mining */
		cogthink(ce);
		break;

	case COGreason:
		/* Symbolic reasoning */
		coginfer(ce);
		break;

	case COGlearn:
		/* Learning operation */
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

/*
 * Wrapper functions for systab interface
 *
 * These functions wrap the syscall implementations to match
 * the Syscall typedef expected by systab.
 */
long
sysrcogthink(ulong *arg)
{
	return syscogthink(arg);
}

long
sysrcogwait(ulong *arg)
{
	return syscogwait(arg);
}

long
sysrcoginfer(ulong *arg)
{
	return syscoginfer(arg);
}

long
sysrcogfocus(ulong *arg)
{
	return syscogfocus(arg);
}

long
sysrcogspread(ulong *arg)
{
	return syscogspread(arg);
}
