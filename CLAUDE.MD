# CLAUDE.MD - Cogplan9 Development Guide

## Project Overview

**cogplan9** (also known as **Plan9Cog**) is an AGI-enabled extension of the Plan 9 operating system that integrates cognitive computing capabilities. This is a revolutionary cognitive kernel where thinking, reasoning, and intelligence emerge from the operating system itself.

### Core Vision
> "This is not an AI system running on an OS. This is an OS that IS AI."

## Quick Start

```bash
# Run integration tests
./test-plan9cog.sh

# Or with Plan 9 rc shell
rc test-plan9cog.rc
```

## Repository Structure

```
cogplan9/
├── sys/
│   ├── src/
│   │   ├── 9/port/           # Kernel modules (cognitive kernel)
│   │   │   ├── devcog.c      # Cognitive device driver (#Σ/)
│   │   │   ├── cogvm.c       # Cognitive Virtual Machine
│   │   │   ├── cogproc.c     # Cognitive process extensions
│   │   │   └── cogmem.c      # Cognitive memory management
│   │   ├── libatomspace/     # Hypergraph knowledge library
│   │   ├── libpln/           # Probabilistic Logic Networks
│   │   ├── libplan9cog/      # System integration library
│   │   └── cmd/
│   │       ├── cogctl/       # Cognitive control CLI
│   │       ├── cogfs/        # Cognitive file server
│   │       ├── cogdemo/      # Demo application
│   │       └── cogkernel/    # Kernel demo program
│   └── include/
│       ├── plan9cog.h        # Main cognitive header
│       └── plan9cog/         # Cognitive subsystem headers
├── .github/workflows/ci.yml  # CI/CD pipeline
├── .devcontainer/            # Dev container configuration
└── Documentation files (*.md)
```

## Build Commands

```bash
# Build a library (example: libatomspace)
cd sys/src/libatomspace && mk install

# Build all libraries
for lib in libatomspace libpln libplan9cog; do
  cd sys/src/$lib && mk install && cd ..
done

# Build commands
for cmd in cogctl cogfs cogdemo; do
  cd sys/src/cmd/$cmd && mk install && cd ..
done
```

## Key Architectural Concepts

### 1. Kernel-Native AtomSpace
- Global hypergraph knowledge base in kernel memory
- 1 million atom capacity, zero-copy access
- All processes share knowledge automatically

### 2. Cognitive VM (cogvm.c)
11 cognitive instructions:
- `COGcreate` - Create atom in kernel
- `COGlink` - Link atoms together
- `COGquery` - Query AtomSpace
- `COGinfer` - Perform PLN inference
- `COGfocus` - Update attention
- `COGspread` - Spread activation
- `COGpattern` - Pattern matching
- `COGmine` - Mine patterns
- `COGreason` - Symbolic reasoning
- `COGlearn` - Learning operation

### 3. Cognitive Device (#Σ/)
```
#Σ/                  # Cognitive device (Σ = sigma)
├── clone            # Allocate cognitive context
├── atomspace        # Kernel AtomSpace operations
├── pln              # PLN inference engine
├── ecan             # Attention allocation
├── cogvm            # Cognitive VM state
├── stats            # System statistics
└── ctl              # Control interface
```

### 4. Attention-Based Scheduling
- CPU allocation by cognitive importance (STI/LTI)
- Important thoughts processed first

### 5. Cognitive Memory Management
- 6 specialized memory types
- Importance-based garbage collection

## Code Style (Plan 9 Conventions)

- **Tabs, not spaces** (8-character tabs)
- **K&R brace style**
- **Lowercase function names**
- **CamelCase types**

```c
#include <u.h>
#include <libc.h>
#include <plan9cog.h>

void
myfunction(void)
{
	int x;

	x = 42;
	print("x = %d\n", x);
}
```

## Key Data Structures

### Atom (Knowledge Unit)
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

### TruthValue (Probabilistic Truth)
```c
struct TruthValue {
    float strength;     // Probability [0.0-1.0]
    float confidence;   // Confidence [0.0-1.0]
    ulong count;        // Evidence count
};
```

### AttentionValue (ECAN)
```c
struct AttentionValue {
    short sti;          // Short-Term Importance
    short lti;          // Long-Term Importance
    short vlti;         // Very Long-Term Importance
};
```

## Current Development Status

### Completed
- Kernel modules (devcog.c, cogvm.c, cogproc.c, cogmem.c)
- Cognitive device driver
- Cognitive VM with 11 instructions
- Attention-based scheduling design
- Cognitive memory management
- Documentation suite
- CI/CD pipeline
- **Phase 2A: Kernel Header Integration** (COMPLETE)
  - portdat.h: Added CogProcExt, CogMemBlock, CogProc, CogAtom, CogAtomSpace types
  - portdat.h: Extended Proc structure with cogext and atomid fields
  - portfns.h: Added 50+ cognitive function declarations
  - master.local: Registered cognitive device (#Σ)
- **Phase 2B: Kernel Initialization** (COMPLETE)
  - main.c: Added cognitive subsystem initialization calls
  - main.c: Added cogdecay background kproc
  - proc.c: Cognitive extension allocation in newproc()
  - proc.c: Cognitive cleanup in pexit()
- **Phase 2C: Scheduler Integration** (COMPLETE)
  - proc.c: Enhanced runproc() with cognitive priority (STI-based)
  - proc.c: Cognitive state tracking in scheduler
  - proc.c: Attention-based process selection within priority queues
- **Tensor Logic Integration** (COMPLETE)
  - cogtensor.c: Tensor data structures for kernel-level reasoning
  - cogtensor.c: Einstein summation (tensoreinsum) operations
  - cogtensor.c: Symbol embedding (tensorembed) for neural-symbolic AI
  - cogtensor.c: Tensor-based truth value computation (tensortv)
  - cogtensor.c: Attention mechanism (tensorattention)
  - Based on tensor-logic.org: unifying deep learning with symbolic AI
- **Unit Tests** (COMPLETE)
  - cogtest.c: Comprehensive unit tests for all cognitive modules
  - test-cognitive-build.sh: Build validation script (76 tests, all passing)
- **Phase 2D: System Calls** (COMPLETE)
  - syscog.c: Cognitive system call implementations
  - systab.h: Registered COGTHINK, COGWAIT, COGINFER, COGFOCUS, COGSPREAD
  - sys.h: Added cognitive syscall numbers (90-94)
  - portfns.h: Updated syscall function declarations

### Next Phase: Full Validation

#### Phase 2E: Full Validation (Priority: High)
- Kernel boot testing
- Performance benchmarking
- Integration testing

See `KERNEL_INTEGRATION_GUIDE.md` for detailed implementation steps.

### Future Phases

#### Phase 3: Persistence & Distribution
- Persistent AtomSpace (save/restore kernel knowledge)
- Network transparency (distributed cognitive state)
- MachSpace full implementation

#### Phase 4: Optimization & Hardening
- GPU acceleration for inference
- Lock-free data structures
- Security hardening

## Testing

```bash
# Run all integration tests
./test-plan9cog.sh

# Tests include:
# - File structure validation
# - Header presence check
# - mkfile validation
# - C syntax checking
```

## Documentation Index

| Document | Description |
|----------|-------------|
| `ARCHITECTURE.md` | System architecture details |
| `ARCHITECTURE_DIAGRAM.md` | Visual architecture diagrams |
| `COGNITIVE_SYSCALL_API.md` | System call reference |
| `DEVELOPMENT.md` | Development environment guide |
| `IMPLEMENTATION_COMPLETE.md` | Implementation summary |
| `INFERNO_COG_ARCHITECTURE.md` | Inferno-inspired design |
| `INFERNO_KERNEL_README.md` | Kernel overview |
| `KERNEL_INTEGRATION_GUIDE.md` | Integration steps |
| `PLAN9COG_GUIDE.md` | Complete API reference |
| `PLAN9COG_README.md` | Quick start guide |
| `QUICKSTART.md` | Getting started |
| `REVOLUTIONARY_SUMMARY.md` | Paradigm explanation |

## Development Environment

### Recommended: GitHub Codespaces
1. Click "Code" > "Codespaces" > "Create codespace"
2. Wait for environment build (~5-10 min first time)
3. Start developing

### Local: Dev Container
```bash
# Open in VS Code, then "Reopen in Container"
code .
```

### Manual Setup
```bash
# Install plan9port
cd /usr/local
sudo git clone https://github.com/9fans/plan9port plan9
cd plan9 && sudo ./INSTALL

# Set environment
export PLAN9=/usr/local/plan9
export PATH=$PLAN9/bin:$PATH
```

## Common Tasks

### Adding a new Atom type
1. Define type constant in `sys/include/plan9cog.h`
2. Add creation logic in `sys/src/libatomspace/atomspace.c`
3. Update `devcog.c` if kernel-level handling needed

### Adding a new PLN inference rule
1. Add rule definition in `sys/src/libpln/pln.c`
2. Register in URE (Unified Rule Engine) in `sys/src/libpln/ure.c`
3. Update documentation in `COGNITIVE_SYSCALL_API.md`

### Adding a new Cognitive VM instruction
1. Define opcode in `sys/include/plan9cog/cogvm.h`
2. Implement in `sys/src/9/port/cogvm.c`
3. Add documentation

## Performance Targets

| Operation | Target | Notes |
|-----------|--------|-------|
| Create atom | 100 cycles | Kernel-level |
| Find atom | 50 cycles | O(1) with hash table |
| Inference | 200 cycles | Single rule |
| Attention update | 80 cycles | ECAN update |

## Branch Strategy

- `main` - Stable releases
- `develop` - Integration branch
- `claude/*` - AI-assisted development branches
- `feature/*` - Feature development

## Commit Message Format

```
type(scope): description

- detail 1
- detail 2
```

Types: `feat`, `fix`, `docs`, `refactor`, `test`, `chore`

## License

Plan 9 Foundation License (see LICENSE file)
