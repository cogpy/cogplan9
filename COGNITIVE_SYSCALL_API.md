# Cognitive System Call API Reference

## Introduction

This document describes the revolutionary cognitive system calls that make thinking, reasoning, and intelligence fundamental kernel operations in Plan 9.

Traditional operating systems provide system calls for file I/O, process management, and memory operations. Our cognitive kernel extends this with **thinking primitives** - system calls that perform cognitive operations at kernel level.

## Philosophy

```c
/* Traditional approach - thinking in userspace */
atom = malloc(sizeof(Atom));
atom->id = generate_id();
atom->type = ConceptNode;
strcpy(atom->name, "cat");
atomspace_add(as, atom);

/* Revolutionary approach - thinking in kernel */
atomid = syscogthink(COGcreate, ConceptNode, 0, "cat");
```

The difference:
- **Traditional:** Application manages knowledge in private memory
- **Revolutionary:** Kernel manages knowledge in shared AtomSpace

## System Call Overview

| Syscall | Number | Purpose |
|---------|--------|---------|
| `cogthink` | 90 | Execute cognitive operation |
| `cogwait` | 91 | Wait for cognitive result |
| `coginfer` | 92 | Perform PLN inference |
| `cogfocus` | 93 | Set attention focus |
| `cogspread` | 94 | Spread activation |

## Cognitive Instruction Set

The `cogthink` syscall executes cognitive instructions:

```c
enum {
    COGnop = 0,     // No operation
    COGcreate,      // Create atom
    COGlink,        // Create link
    COGquery,       // Query AtomSpace
    COGinfer,       // Perform inference
    COGfocus,       // Update attention
    COGspread,      // Spread activation
    COGpattern,     // Pattern match
    COGmine,        // Mine patterns
    COGreason,      // Symbolic reasoning
    COGlearn,       // Learning operation
};
```

## System Call Specifications

### cogthink - Execute Cognitive Operation

**Prototype:**
```c
int cogthink(int op, int arg1, int arg2, void *data);
```

**Parameters:**
- `op` - Cognitive operation code (from enum above)
- `arg1` - First integer argument
- `arg2` - Second integer argument  
- `data` - Pointer to operation-specific data

**Returns:**
- Operation result (interpretation depends on `op`)
- -1 on error, sets `errstr`

**Description:**

Executes a cognitive operation at kernel level. The operation is performed on the kernel AtomSpace with kernel-level efficiency.

**Examples:**

```c
// Create a concept node
ulong atomid = cogthink(COGcreate, ConceptNode, 0, "cat");

// Create a link between atoms
ulong linkid = cogthink(COGlink, InheritanceLink, 0, (ulong[]){atom1, atom2});

// Query AtomSpace
int natoms = cogthink(COGquery, ConceptNode, 0, NULL);

// Perform inference
cogthink(COGinfer, Deduction, 0, (ulong[]){atom1, atom2});

// Update attention
cogthink(COGfocus, atomid, 100, NULL);  // Set STI to 100

// Spread activation
cogthink(COGspread, atomid, 50, NULL);  // Spread 50 STI units

// Pattern matching
cogthink(COGpattern, 0, 0, pattern_data);

// Start learning
cogthink(COGlearn, algorithm, 0, learning_data);
```

### cogwait - Wait for Cognitive Result

**Prototype:**
```c
int cogwait(void);
```

**Returns:**
- 0 on success
- -1 on error

**Description:**

Waits for asynchronous cognitive operation to complete. Similar to `wait()` for processes, but for cognitive operations.

**Example:**

```c
// Start asynchronous inference
cogthink(COGreason, ASYNC, 0, reasoning_task);

// Continue other work
do_other_work();

// Wait for reasoning to complete
if(cogwait() < 0)
    fprint(2, "cognitive wait failed: %r\n");
```

### coginfer - Perform PLN Inference

**Prototype:**
```c
int coginfer(int rule, ulong *atoms, int natoms);
```

**Parameters:**
- `rule` - PLN inference rule to apply
- `atoms` - Array of atom IDs
- `natoms` - Number of atoms

**Returns:**
- Result atom ID
- -1 on error

**Description:**

Performs Probabilistic Logic Networks inference at kernel level. Applies the specified inference rule to the given atoms and returns the result.

**Rules:**

```c
enum {
    PLNDeduction = 1,     // A→B, B→C ⇒ A→C
    PLNInduction,         // A→B ⇒ B→A
    PLNAbduction,         // A→B, B→C ⇒ C→A
    PLNRevision,          // Combine evidence
    PLNAnd,               // Boolean AND
    PLNOr,                // Boolean OR
    PLNNot,               // Boolean NOT
};
```

**Examples:**

```c
// Deduction: Given cat→animal, animal→living, infer cat→living
ulong atoms[] = {cat_animal_id, animal_living_id};
ulong result = coginfer(PLNDeduction, atoms, 2);

// Revision: Combine multiple evidence sources
ulong evidence[] = {fact1, fact2, fact3};
ulong combined = coginfer(PLNRevision, evidence, 3);

// Boolean inference
ulong operands[] = {premise1, premise2};
ulong conclusion = coginfer(PLNAnd, operands, 2);
```

### cogfocus - Set Attention Focus

**Prototype:**
```c
int cogfocus(ulong atomid);
```

**Parameters:**
- `atomid` - Atom to focus attention on

**Returns:**
- 0 on success
- -1 on error

**Description:**

Sets the attentional focus to the specified atom. This increases the atom's STI (Short-Term Importance) and makes it more likely to be used in cognitive operations.

**Example:**

```c
// Focus on important concept
cogfocus(critical_atom_id);

// Kernel scheduler gives more resources to operations on this atom

// Reasoning using focused atom
coginfer(PLNDeduction, (ulong[]){critical_atom_id, other_atom}, 2);
```

### cogspread - Spread Activation

**Prototype:**
```c
int cogspread(ulong atomid, short amount);
```

**Parameters:**
- `atomid` - Source atom for spreading
- `amount` - Amount of STI to spread

**Returns:**
- Number of atoms activated
- -1 on error

**Description:**

Spreads activation (STI) from the source atom to connected atoms in the knowledge graph. This implements spreading activation networks at kernel level.

**Example:**

```c
// Spread activation from source concept
int activated = cogspread(source_atom, 100);
print("activated %d atoms\n", activated);

// Atoms connected to source now have higher STI
// They will be considered in future reasoning
```

## Device File Interface

Cognitive operations can also be performed via device files:

### /dev/cog/atomspace

**Read:**
```bash
cat /dev/cog/atomspace
```
Returns list of all atoms in kernel AtomSpace with their properties.

**Write:**
```bash
echo 'create 1 myatom' > /dev/cog/atomspace
```
Creates new atom. Format: `create <type> <name>`

### /dev/cog/pln

**Read:**
```bash
cat /dev/cog/pln
```
Returns PLN inference results.

**Write:**
```bash
echo 'deduction 123 456' > /dev/cog/pln
```
Performs PLN deduction on atoms 123 and 456.

### /dev/cog/ecan

**Read:**
```bash
cat /dev/cog/ecan
```
Returns attention allocation state.

**Write:**
```bash
echo 'update' > /dev/cog/ecan
echo 'allocate 123 100' > /dev/cog/ecan
```
Updates attention or allocates STI to atom.

### /dev/cog/cogvm

**Read:**
```bash
cat /dev/cog/cogvm
```
Returns Cognitive VM state (cycles, inferences, etc.)

**Write:**
Not writable (read-only VM state).

### /dev/cog/stats

**Read:**
```bash
cat /dev/cog/stats
```
Returns cognitive statistics.

**Write:**
Not writable (read-only statistics).

### /dev/cog/ctl

**Write:**
```bash
echo 'start' > /dev/cog/ctl    # Start cognitive VM
echo 'stop' > /dev/cog/ctl     # Stop cognitive VM
echo 'reset' > /dev/cog/ctl    # Reset cognitive state
```
Control cognitive system.

## Library Wrappers

For convenience, libc provides wrappers:

```c
/* In libc.h */
int     cogthink(int, int, int, void*);
int     cogwait(void);
int     coginfer(int, ulong*, int);
int     cogfocus(ulong);
int     cogspread(ulong, short);

/* Device file helpers */
int     cogopen(int mode);          // Open #Σ
ulong   cogatomcreate(int, char*);  // Create atom via device
int     cogatomlist(Atom**, int);   // List atoms via device
```

## Process Cognitive Extensions

Each process automatically has cognitive extensions:

```c
struct Proc {
    /* ... existing fields ... */
    
    CogProcExt *cogext;     // Cognitive extension
    ulong      atomid;      // Process's atom in knowledge graph
};
```

**Automatic behavior:**

1. **Creation:** Process gets cognitive extension at fork
2. **Scheduling:** Scheduler considers cognitive importance
3. **Termination:** Cognitive state cleaned up at exit

**Accessing extensions:**

```c
// Get current process's cognitive state
CogProcExt *ce = up->cogext;

// Check cognitive priority
int pri = cogpriority(ce);

// Update attention
cogupdate(ce, +10, +5);  // Boost STI by 10, LTI by 5
```

## Memory Allocation

Cognitive memory allocation:

```c
/* Allocate memory with cognitive importance */
void *cogalloc(ulong size, int type, short sti, short lti);

/* Free cognitive memory */
void cogfree(void *addr);

/* Update memory importance */
void cogmemupdate(void *addr, short dsti, short dlti);

/* Specialized allocators */
void *cogallocatom(ulong size, short sti);
void *cogalloclink(ulong size, short sti);
void *cogallocpattern(ulong size);
void *cogallocinfer(ulong size);
```

**Example:**

```c
// Allocate important atom
Atom *atom = cogallocatom(sizeof(Atom), 100);  // High STI

// Use atom
atom->id = generate_id();
atom->type = ConceptNode;

// Promote importance
cogmempromote(atom);

// When done
cogfree(atom);
```

## Error Handling

Cognitive system calls use standard Plan 9 error handling:

```c
ulong atomid = cogthink(COGcreate, ConceptNode, 0, "test");
if(atomid == -1)
    fprint(2, "create failed: %r\n");

if(coginfer(PLNDeduction, atoms, 2) < 0)
    fprint(2, "inference failed: %r\n");
```

**Common errors:**

- `"out of cognitive memory"` - Kernel AtomSpace full
- `"bad cognitive command"` - Invalid operation
- `"atom not found"` - Invalid atom ID
- `"inference failed"` - PLN inference error

## Performance Considerations

### Kernel vs Userspace

**Kernel cognitive operations:**
- ✅ Faster (no context switch)
- ✅ Shared knowledge (zero copy)
- ✅ System-wide visible
- ⚠️ Limited by kernel memory

**Userspace cognitive operations:**
- ✅ Unlimited memory
- ✅ Process-private
- ⚠️ Slower (library overhead)
- ⚠️ Requires serialization

### Optimization Tips

1. **Batch operations:**
```c
// Bad: Many small operations
for(i = 0; i < 1000; i++)
    cogthink(COGcreate, ConceptNode, 0, names[i]);

// Good: Batch create
cogthink(COGcreatebatch, ConceptNode, 1000, names);
```

2. **Use attention wisely:**
```c
// Focus on important atoms
cogfocus(critical_atom);

// Let unimportant atoms decay
// (automatic via cogdecayprocs())
```

3. **Prefer kernel operations for shared knowledge:**
```c
// Shared knowledge - use kernel
atomid = cogthink(COGcreate, ConceptNode, 0, "shared");

// Private data - use userspace
Atom *private = atomcreate(local_as, ConceptNode, "private");
```

## Security

### Access Control

Cognitive operations respect process permissions:

```c
// Only processes with CAP_COGNITIVE can modify kernel AtomSpace
if(!(up->privs & CAP_COGNITIVE)) {
    error("permission denied");
}
```

### Resource Limits

Kernel limits cognitive resources:

```c
// Maximum atoms per process
#define MAXATOMSPERPROC 10000

// Maximum inference depth
#define MAXINFERDEPTH 100

// Maximum STI allocation per process
#define MAXSTIBUDGET 1000
```

## Examples

### Complete Cognitive Program

```c
#include <u.h>
#include <libc.h>

void
main(void)
{
    ulong cat, animal, living, link1, link2, result;
    int fd;
    char buf[256];

    /* Method 1: System calls */
    cat = cogthink(COGcreate, ConceptNode, 0, "cat");
    animal = cogthink(COGcreate, ConceptNode, 0, "animal");
    living = cogthink(COGcreate, ConceptNode, 0, "living");
    
    link1 = cogthink(COGlink, InheritanceLink, 0, 
                     (ulong[]){cat, animal});
    link2 = cogthink(COGlink, InheritanceLink, 0, 
                     (ulong[]){animal, living});
    
    result = coginfer(PLNDeduction, (ulong[]){link1, link2}, 2);
    
    print("Inferred: cat -> living (atom %ld)\n", result);
    
    /* Method 2: Device files */
    fd = open("#Σ/atomspace", ORDWR);
    if(fd < 0)
        sysfatal("cannot open cognitive device");
    
    write(fd, "create 1 dog", 12);
    write(fd, "create 1 mammal", 15);
    
    seek(fd, 0, 0);
    while(read(fd, buf, sizeof buf) > 0)
        print("%s", buf);
    
    close(fd);
    exits(nil);
}
```

### Cognitive Background Process

```c
void
cognitive_daemon(void)
{
    int fd;
    
    fd = open("#Σ/ecan", ORDWR);
    
    for(;;) {
        /* Update attention allocation */
        write(fd, "update", 6);
        
        /* Decay attention values */
        cogdecayprocs();
        cogmemdecay();
        
        /* Collect garbage */
        coggc();
        
        sleep(10000);  /* 10 seconds */
    }
}
```

## Future Extensions

Planned cognitive system calls:

```c
int cogpatternmatch(Pattern *pat, Atom **results, int max);
int cogmine(int minsupport, float minconf, Pattern **patterns);
int coglearn(int algorithm, void *data);
int cograeson(int maxdepth, ulong goal, ulong *premises, int npm);
int cogevolve(Population *pop, int generations);
```

## References

- Plan 9 System Call Manual
- Inferno Dis VM Specification
- OpenCog AtomSpace API
- PLN Inference Rules

---

**Revolution:** Thinking as a system call, not a library function.
