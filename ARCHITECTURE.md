# Plan9Cog Architecture

## Overview

Plan9Cog is an AGI-enabled extension of the Plan 9 operating system that integrates cognitive computing capabilities based on the OpenCog framework. This document describes the architecture, design decisions, and implementation details.

## Design Philosophy

Plan9Cog follows Plan 9's core principles:

1. **Everything is a file**: Cognitive services are exposed via 9P file servers
2. **Simplicity**: Clean, minimal interfaces with clear semantics
3. **Distributed by design**: Cognitive processing can span multiple systems
4. **Resource sharing**: Knowledge and reasoning shared across processes
5. **Namespace unification**: AtomSpace accessible via standard file operations

## Architectural Layers

### Layer 1: Cognitive Kernel Extensions

Extensions to the Plan9 kernel for cognitive operations (currently as headers for future kernel integration):

```
/sys/include/plan9cog/cogvm.h
├── CogSegment: Specialized memory regions
├── CogMemory: Cognitive memory manager
├── CogPatternCache: Pattern memory caching
└── AttentionAlloc: Attention-based allocation
```

**Key Features:**
- Cognitive memory attributes (hypergraph, pattern, attention)
- Optimized page fault handling for cognitive operations
- Attention-based memory allocation (ECAN integration)
- Cognitive garbage collection

### Layer 2: Cognitive Libraries

Core libraries implementing cognitive computing primitives:

#### libatomspace - Hypergraph Knowledge Representation

```
/sys/src/libatomspace/
├── atomspace.c: Core AtomSpace operations
├── pattern.c: Pattern matching
└── serialize.c: Import/export
```

**Structures:**
- `Atom`: Nodes and links with truth/attention values
- `AtomSpace`: Hypergraph container
- `TruthValue`: Probabilistic strength and confidence
- `AttentionValue`: STI, LTI, VLTI importance

**Operations:**
- Create/delete atoms and links
- Query and pattern matching
- Truth value manipulation
- Attention value management
- Serialization/deserialization

#### libpln - Probabilistic Logic Networks

```
/sys/src/libpln/
├── pln.c: PLN inference engine
└── ure.c: Unified Rule Engine
```

**Formulas:**
- Deduction: A→B, B→C ⇒ A→C
- Induction: A→B ⇒ B→A (probabilistic)
- Abduction: A→B, B→C ⇒ C→A (probabilistic)
- Revision: Combine multiple evidence sources
- Boolean: AND, OR, NOT with uncertainty

**Inference:**
- Forward chaining: Given premises, derive conclusions
- Backward chaining: Given goal, find supporting premises
- Direct evaluation: Compute truth value for query

#### libplan9cog - System Integration

```
/sys/src/libplan9cog/
├── plan9cog.c: Main system initialization
├── ecan.c: Economic Attention Network
├── machspace.c: Distributed hypergraph
└── reactor.c: Cognitive Fusion Reactor
```

**Components:**
- `Plan9Cog`: Global cognitive system state
- `EcanNetwork`: Attention allocation network
- `MachSpace`: Distributed AtomSpace
- `CogFusionReactor`: Multi-process cognitive tasks
- `CogGrip`: Universal object handling

### Layer 3: Cognitive Services

File servers and daemons providing cognitive services:

#### cogfs - Cognitive File Server

```
/sys/src/cmd/cogfs/cogfs.c
```

Exposes AtomSpace operations via 9P protocol at `/mnt/cog`:

```
/mnt/cog/
├── atoms: List all atoms
├── atomctl: Control atom operations
├── pln: PLN inference results
├── plnctl: Control PLN inference
├── ecan: ECAN state
├── ecanctl: Control ECAN
├── pattern: Pattern matching
├── stats: System statistics
└── ctl: General control
```

**Protocol:**
- Read files: Query cognitive state
- Write to ctl files: Issue commands
- 9P messages: Standard file operations
- Cognitive extensions: Custom Tcogatom, Tcogpln, etc.

### Layer 4: Applications and Tools

User-facing tools for cognitive computing:

#### cogctl - Cognitive Control Utility

```
/sys/src/cmd/cogctl/cogctl.c
```

Command-line interface for cognitive operations:

```bash
cogctl atom create <type> <name>
cogctl atom list
cogctl atom info <id>
cogctl pln infer <query>
cogctl pln stats
cogctl ecan update
cogctl ecan focus
cogctl info
```

#### cogdemo - Demonstration Program

```
/sys/src/cmd/cogdemo/cogdemo.c
```

Interactive demonstrations of Plan9Cog capabilities:
- AtomSpace operations
- PLN inference examples
- ECAN attention allocation
- System information

## Data Structures

### Atom

The fundamental unit of knowledge:

```c
struct Atom {
    ulong id;           // Unique identifier
    int type;           // AtomNode, ConceptNode, etc.
    char *name;         // Name (for nodes)
    Atom **outgoing;    // Outgoing atoms (for links)
    int noutgoing;      // Number of outgoing atoms
    TruthValue tv;      // Probabilistic truth value
    AttentionValue av;  // Attention allocation
};
```

### TruthValue

Probabilistic assessment of truth:

```c
struct TruthValue {
    float strength;     // Probability of truth [0.0-1.0]
    float confidence;   // Confidence in assessment [0.0-1.0]
    ulong count;        // Evidence count
};
```

### AttentionValue

Economic importance allocation:

```c
struct AttentionValue {
    short sti;          // Short-Term Importance
    short lti;          // Long-Term Importance
    short vlti;         // Very Long-Term Importance
};
```

### AtomSpace

Hypergraph container:

```c
struct AtomSpace {
    Atom **atoms;       // Array of atoms
    int natoms;         // Number of atoms
    int maxatoms;       // Maximum capacity
    Lock;               // Thread safety
};
```

## Algorithms

### Pattern Matching

Recursive graph pattern matching:

1. Match wildcard patterns to any atom
2. Match type if specified
3. Match name if specified (for nodes)
4. Recursively match outgoing atoms (for links)
5. Return all matching atoms

Complexity: O(n*m) where n=atoms, m=pattern size

### PLN Deduction

Compute A→C from A→B and B→C:

```
strength(A→C) = strength(A→B) * strength(B→C)
confidence(A→C) = confidence(A→B) * confidence(B→C)
```

### ECAN Update

Update attentional focus:

1. Collect all atoms with STI values
2. Sort by STI (descending)
3. Select top N atoms for attentional focus
4. Update focus list

Complexity: O(n log n) for sorting

### Attention Spreading

Spread importance through graph:

1. Get source atom's STI
2. Find incoming links
3. Divide STI among incoming atoms
4. Update each atom's attention
5. Trigger ECAN update

## Threading and Concurrency

### Lock Protection

All shared data structures protected by locks:

- AtomSpace operations: Lock entire space
- ECAN updates: Lock network state
- MachSpace operations: Lock remote connections
- Plan9Cog system: Lock for initialization

### Thread Safety

- All public APIs are thread-safe
- Lock granularity: Per data structure
- Lock ordering: Prevent deadlocks
- Lock duration: Minimize critical sections

### Future: Lock-Free Operations

Potential optimizations:
- Read-mostly patterns with RCU
- Compare-and-swap for atom updates
- Lock-free pattern matching
- Per-atom locking

## Distribution

### MachSpace

Distributed hypergraph memory:

```
Local AtomSpace <---> MachSpace Bridge <---> Remote AtomSpaces
                             |
                             +--> Network Protocol (9P)
                             |
                             +--> Synchronization Engine
```

**Operations:**
- Connect to remote hosts
- Find atoms locally and remotely
- Synchronize changes across network
- Handle network partitions

### 9P Protocol

Standard Plan 9 file protocol for all operations:

- `Topen/Ropen`: Open cognitive files
- `Tread/Rread`: Read atom data
- `Twrite/Rwrite`: Write commands
- Custom: `Tcogatom/Rcogatom` for atom operations
- Custom: `Tcogpln/Rcogpln` for PLN inference

## Memory Management

### Cognitive Memory Regions

Specialized memory for cognitive operations:

- **Hypergraph memory**: Optimized for graph traversal
- **Pattern memory**: Cached pattern matching results
- **Attention memory**: ECAN allocation tables
- **Shared memory**: Cross-process AtomSpace

### Allocation Strategy

1. Normal memory: Standard malloc/free
2. Cognitive memory: CogMemory allocator
3. Attention-based: Higher STI = higher priority
4. Garbage collection: Mark-and-sweep with attention

### Future: Memory Mapping

- mmap() for large AtomSpaces
- Copy-on-write for snapshots
- Memory-mapped file backing
- Persistent AtomSpace storage

## Performance Characteristics

### AtomSpace Operations

- Create atom: O(1) amortized
- Find atom: O(n) linear search
- Delete atom: O(n) linear search
- Query: O(n) with predicate
- Pattern match: O(n*m)

**Future Optimizations:**
- Hash table for O(1) lookup
- Index by type for faster queries
- B-tree for sorted access

### PLN Inference

- Single rule: O(1)
- Forward chaining: O(r*d) where r=rules, d=depth
- Backward chaining: O(r*d)
- URE chaining: O(r*i) where i=iterations

### ECAN

- Update: O(n log n) for sorting
- Allocate: O(1)
- Spread: O(m) where m=incoming links
- Decay: O(n)

## Integration Points

### Plan9 Services

- **File servers**: Mount cognitive services
- **Namespace**: Union cognitive and standard files
- **Plumber**: Route cognitive messages
- **Acme**: Edit AtomSpace in text editor
- **Rio**: Visualize cognitive graphs (future)

### External Systems

- **Import/Export**: Serialize AtomSpace to files
- **Network**: Connect to remote Plan9Cog systems
- **Databases**: Persistent storage backend
- **OpenCog**: Compatible AtomSpace format

## Security Considerations

### Access Control

- File permissions on cognitive files
- Namespace isolation per user
- Capability-based access (future)
- Factotum integration for auth

### Resource Limits

- Maximum atoms per AtomSpace
- Maximum inference depth
- Attention budget constraints
- Memory usage limits

### Network Security

- TLS for remote connections
- Authentication for MachSpace
- Encryption for atom data
- Signed AtomSpace exports

## Future Directions

### Near Term

1. **Build system**: Complete mk files, test builds
2. **Testing**: Unit tests, integration tests
3. **Examples**: More demonstration programs
4. **Documentation**: Man pages, tutorials

### Medium Term

1. **Pattern mining**: Learn patterns from data
2. **Natural language**: Link Grammar integration
3. **Visualization**: Graph viewer in Rio
4. **Persistence**: Database-backed AtomSpace
5. **Network**: Full MachSpace implementation

### Long Term

1. **Neural networks**: Sub-symbolic learning
2. **Embodiment**: Robot/agent integration
3. **Multi-modal**: Vision, audio, sensor data
4. **Real-time**: Hard real-time guarantees
5. **Formal verification**: Prove correctness
6. **Hardware**: FPGA/GPU acceleration

## References

### Plan 9

- Pike et al., "Plan 9 from Bell Labs", USENIX 1990
- Pike et al., "The Use of Name Spaces in Plan 9", Operating Systems Review 1993

### OpenCog

- Goertzel et al., "OpenCog: A Software Framework for Integrative AGI", AGI 2008
- Goertzel et al., "Probabilistic Logic Networks", Springer 2009

### Cognitive Computing

- Goertzel, "A Cosmist Manifesto", 2010
- Hart & Goertzel, "OpenCog: A Software Framework for AGI", 2014

## Conclusion

Plan9Cog represents a unique integration of cognitive computing with operating system design. By following Plan 9's principles of simplicity and resource sharing, it creates a clean, extensible platform for AGI research and development. The layered architecture allows components to be used independently or as a unified system, supporting a wide range of cognitive computing applications.
