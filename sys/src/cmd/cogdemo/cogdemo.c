/*
 * cogdemo - Plan9Cog demonstration program
 * Shows basic AtomSpace, PLN, and ECAN operations
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <plan9cog.h>

void
demo_atomspace(Plan9Cog *p9c)
{
	Atom *cat, *animal, *mammal;
	Atom *cat_animal, *mammal_animal;
	Atom *outgoing[2];
	TruthValue tv;
	
	print("\n=== AtomSpace Demo ===\n\n");
	
	/* Create concept nodes */
	cat = atomcreate(p9c->atomspace, ConceptNode, "cat");
	animal = atomcreate(p9c->atomspace, ConceptNode, "animal");
	mammal = atomcreate(p9c->atomspace, ConceptNode, "mammal");
	
	print("Created atoms:\n");
	print("  %s\n", cogatomstr(cat));
	print("  %s\n", cogatomstr(animal));
	print("  %s\n", cogatomstr(mammal));
	
	/* Create inheritance links */
	outgoing[0] = cat;
	outgoing[1] = animal;
	cat_animal = linkcreate(p9c->atomspace, InheritanceLink, outgoing, 2);
	
	outgoing[0] = mammal;
	outgoing[1] = animal;
	mammal_animal = linkcreate(p9c->atomspace, InheritanceLink, outgoing, 2);
	
	/* Set truth values */
	tv.strength = 0.9;
	tv.confidence = 0.85;
	tv.count = 15;
	atomsettruth(cat_animal, tv);
	
	tv.strength = 0.95;
	tv.confidence = 0.9;
	tv.count = 20;
	atomsettruth(mammal_animal, tv);
	
	print("\nCreated inheritance links:\n");
	print("  %s  %s\n", cogatomstr(cat_animal), cogtvstr(atomgettruth(cat_animal)));
	print("  %s  %s\n", cogatomstr(mammal_animal), cogtvstr(atomgettruth(mammal_animal)));
	
	print("\nTotal atoms in AtomSpace: %d\n", p9c->atomspace->natoms);
}

void
demo_pln(Plan9Cog *p9c)
{
	TruthValue tv_ab, tv_bc, result;
	
	print("\n=== PLN Demo ===\n\n");
	
	/* Set up truth values for A->B and B->C */
	tv_ab.strength = 0.9;
	tv_ab.confidence = 0.8;
	tv_ab.count = 10;
	
	tv_bc.strength = 0.85;
	tv_bc.confidence = 0.75;
	tv_bc.count = 8;
	
	print("Given:\n");
	print("  A -> B: %s\n", cogtvstr(tv_ab));
	print("  B -> C: %s\n", cogtvstr(tv_bc));
	
	/* Deduction: A->B, B->C => A->C */
	result = plndeduction(tv_ab, tv_bc);
	print("\nDeduction (A -> C): %s\n", cogtvstr(result));
	
	/* Induction: A->B => B->A */
	result = plninduction(tv_ab, tv_bc);
	print("Induction (B -> A): %s\n", cogtvstr(result));
	
	/* Abduction: A->B, B->C => C->A */
	result = plnabduction(tv_ab, tv_bc);
	print("Abduction (C -> A): %s\n", cogtvstr(result));
	
	/* Revision: combine evidence */
	TruthValue tv2;
	tv2.strength = 0.88;
	tv2.confidence = 0.82;
	tv2.count = 12;
	
	result = plnrevision(tv_ab, tv2);
	print("\nRevision (combining evidence): %s\n", cogtvstr(result));
	
	/* Boolean operations */
	result = plnand(tv_ab, tv_bc);
	print("AND: %s\n", cogtvstr(result));
	
	result = plnor(tv_ab, tv_bc);
	print("OR: %s\n", cogtvstr(result));
	
	result = plnnot(tv_ab);
	print("NOT: %s\n", cogtvstr(result));
}

void
demo_ecan(Plan9Cog *p9c)
{
	EcanNetwork *ecan;
	Atom *a1, *a2, *a3, *a4;
	AttentionValue av;
	Atom **focus;
	int nfocus, i;
	
	print("\n=== ECAN Demo ===\n\n");
	
	/* Create some atoms with varying importance */
	a1 = atomcreate(p9c->atomspace, ConceptNode, "urgent_task");
	a2 = atomcreate(p9c->atomspace, ConceptNode, "normal_task");
	a3 = atomcreate(p9c->atomspace, ConceptNode, "low_priority_task");
	a4 = atomcreate(p9c->atomspace, ConceptNode, "background_task");
	
	/* Assign attention values */
	av.sti = 100;
	av.lti = 50;
	av.vlti = 20;
	atomsetattention(a1, av);
	
	av.sti = 60;
	av.lti = 40;
	av.vlti = 15;
	atomsetattention(a2, av);
	
	av.sti = 30;
	av.lti = 20;
	av.vlti = 10;
	atomsetattention(a3, av);
	
	av.sti = 10;
	av.lti = 10;
	av.vlti = 5;
	atomsetattention(a4, av);
	
	print("Assigned attention values:\n");
	print("  %s  STI=%d LTI=%d\n", cogatomstr(a1), a1->av.sti, a1->av.lti);
	print("  %s  STI=%d LTI=%d\n", cogatomstr(a2), a2->av.sti, a2->av.lti);
	print("  %s  STI=%d LTI=%d\n", cogatomstr(a3), a3->av.sti, a3->av.lti);
	print("  %s  STI=%d LTI=%d\n", cogatomstr(a4), a4->av.sti, a4->av.lti);
	
	/* Initialize ECAN with total attention funds */
	ecan = ecaninit(p9c->atomspace, 1000, 1000);
	if(ecan == nil)
		sysfatal("ecaninit: %r");
	
	/* Update attention allocation */
	ecanupdate(ecan);
	
	/* Get attentional focus */
	focus = ecanfocus(ecan, &nfocus);
	
	print("\nAttentional Focus (top %d atoms):\n", nfocus);
	for(i = 0; i < nfocus && i < 10; i++){
		print("  %d. %s  STI=%d\n", i+1, cogatomstr(focus[i]), focus[i]->av.sti);
	}
	
	/* Spread attention from high-importance atom */
	print("\nSpreading attention from: %s\n", cogatomstr(a1));
	ecanspread(ecan, a1);
	
	/* Apply attention decay */
	print("\nApplying 10%% attention decay...\n");
	ecandecay(ecan, 0.1);
	
	/* Update and show new focus */
	ecanupdate(ecan);
	focus = ecanfocus(ecan, &nfocus);
	
	print("Attentional Focus after decay:\n");
	for(i = 0; i < nfocus && i < 10; i++){
		print("  %d. %s  STI=%d\n", i+1, cogatomstr(focus[i]), focus[i]->av.sti);
	}
	
	ecanfree(ecan);
}

void
demo_system(Plan9Cog *p9c)
{
	CogInfo info;
	
	print("\n=== System Information ===\n\n");
	
	coginfo(p9c, &info);
	
	print("Version: %s\n", info.version);
	print("Uptime: %ld seconds\n", info.uptime);
	print("Total atoms: %ld\n", info.natoms);
	print("Total rules: %ld\n", info.nrules);
	print("Total inferences: %ld\n", info.ninferences);
	print("Cognitive memory: %ld bytes\n", info.cogmem);
}

void
usage(void)
{
	fprint(2, "usage: cogdemo [-a] [-p] [-e] [-s]\n");
	fprint(2, "  -a  Run AtomSpace demo\n");
	fprint(2, "  -p  Run PLN demo\n");
	fprint(2, "  -e  Run ECAN demo\n");
	fprint(2, "  -s  Show system information\n");
	fprint(2, "  (no options: run all demos)\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	Plan9Cog *p9c;
	int run_atomspace, run_pln, run_ecan, run_system;
	int any_specified;
	
	run_atomspace = 0;
	run_pln = 0;
	run_ecan = 0;
	run_system = 0;
	
	ARGBEGIN{
	case 'a':
		run_atomspace = 1;
		break;
	case 'p':
		run_pln = 1;
		break;
	case 'e':
		run_ecan = 1;
		break;
	case 's':
		run_system = 1;
		break;
	default:
		usage();
	}ARGEND
	
	any_specified = run_atomspace || run_pln || run_ecan || run_system;
	
	/* If no options specified, run all demos */
	if(!any_specified){
		run_atomspace = 1;
		run_pln = 1;
		run_ecan = 1;
		run_system = 1;
	}
	
	print("Plan9Cog Demonstration Program\n");
	print("===============================\n");
	
	/* Initialize Plan9Cog */
	p9c = plan9coginit();
	if(p9c == nil)
		sysfatal("plan9coginit: %r");
	
	/* Run selected demos */
	if(run_atomspace)
		demo_atomspace(p9c);
	
	if(run_pln)
		demo_pln(p9c);
	
	if(run_ecan)
		demo_ecan(p9c);
	
	if(run_system)
		demo_system(p9c);
	
	print("\n=== Demo Complete ===\n");
	
	/* Clean up */
	plan9cogfree(p9c);
	exits(nil);
}
