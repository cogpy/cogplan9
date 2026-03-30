# Plan9Cog Development Roadmap
## Next Phase: OpenCog Features via Plan 9 Architecture

**Version:** 0.2 Development
**Date:** 2026-01-14

---

## Vision

Leverage Plan 9's unique architectural qualities to implement OpenCog cognitive features in ways impossible on traditional operating systems:

| Plan 9 Quality | OpenCog Application |
|----------------|---------------------|
| Everything is a file (9P) | Cognitive services as file servers |
| Per-process namespaces | Process-specific cognitive contexts |
| Network transparency | Distributed cognition across machines |
| Union directories | Layered knowledge bases |
| /proc filesystem | Debuggable cognitive processes |

---

## Sprint 1: Foundation Fixes (Current)

### 1.1 Complete AtomSpace Persistence
**File:** `sys/src/libatomspace/serialize.c`

Fix `atomspaceimport` to properly restore atoms from file:
- Two-pass parsing (nodes first, then links)
- Support any Plan 9 file descriptor (local, network, pipe)

### 1.2 Implement Distributed Cognition via 9P
**File:** `sys/src/libplan9cog/machspace.c`

True network transparency using Plan 9's mount:
- `machspace9pconnect()` - Mount remote cognitive services
- `machspace9pfind()` - Find atoms across network
- `machspace9psync()` - Synchronize distributed state

### 1.3 Async Cognitive Reactor
**File:** `sys/src/libplan9cog/reactor.c`

Use Plan 9's `rfork` for true parallel processing:
- Worker process pool via `rfork(RFPROC|RFMEM)`
- Pipe-based task distribution
- `alt()` for multiplexed result collection

---

## Sprint 2: Per-Process Cognitive Namespaces

### 2.1 Cognitive /proc Extensions
**New File:** `sys/src/9/port/devcogproc.c`

```
/proc/pid/cog/
  ├── atoms      # Process's relevant atoms
  ├── focus      # Attentional focus
  ├── goals      # OpenPsi goals
  ├── beliefs    # Belief state
  ├── ctl        # Control allocation
  └── stats      # Statistics
```

### 2.2 Union Directories for Knowledge Layering
Layer cognitive knowledge like filesystems:
```sh
bind /mnt/cog/base /mnt/cog         # Base knowledge
bind -a /mnt/cog/domain /mnt/cog    # Domain overlay
bind -a /mnt/cog/user /mnt/cog      # User overlay
```

---

## Sprint 3: Enhanced URE with File Interface

### 3.1 Rules as Files
```
/mnt/cog/rules/
  ├── deduction/
  │   ├── ctl          # Enable/disable, weight
  │   ├── definition   # Rule pattern
  │   └── stats        # Usage statistics
  ├── induction/
  └── custom/
```

### 3.2 Inference Debugging via Files
```
/mnt/cog/inference/
  ├── active      # Running inferences
  ├── trace       # Trace log
  ├── breakpoints # Breakpoints
  └── n/          # Per-inference state
```

---

## Sprint 4: OpenPsi Goal-Directed Behavior

**New Files:**
- `sys/src/libplan9cog/openpsi.c`
- `sys/src/cmd/openpsi/openpsi.c`
- `sys/include/plan9cog/openpsi.h`

```
/mnt/psi/
  ├── urges/         # Basic drives
  │   ├── curiosity/
  │   │   ├── level
  │   │   └── target
  │   └── competence/
  ├── goals/         # Active goals
  ├── demands/       # Unsatisfied demands
  └── modulators/    # Global modulation
```

---

## Sprint 5: Temporal Reasoning

**New Files:**
- `sys/src/libplan9cog/temporal.c`
- `sys/include/plan9cog/temporal.h`

Access AtomSpace at different time points:
```
/mnt/cog/temporal/
  ├── now/        # Current state
  ├── t-1h/       # One hour ago
  ├── t-1d/       # One day ago
  └── snap/       # Named snapshots
```

---

## Implementation Priority

| Priority | Feature | Plan 9 Leverage |
|----------|---------|-----------------|
| HIGH | AtomSpace import | File I/O |
| HIGH | 9P distribution | Network transparency |
| HIGH | Async reactor | rfork processes |
| MEDIUM | Per-process namespaces | /proc extensions |
| MEDIUM | Rule file hierarchy | Everything is a file |
| MEDIUM | OpenPsi | File server |
| LOW | Temporal reasoning | Versioned namespaces |

---

## File Summary

### New Files to Create
| File | Purpose |
|------|---------|
| `sys/src/libplan9cog/openpsi.c` | OpenPsi library |
| `sys/src/cmd/openpsi/openpsi.c` | OpenPsi file server |
| `sys/src/libplan9cog/temporal.c` | Temporal reasoning |
| `sys/include/plan9cog/openpsi.h` | OpenPsi header |
| `sys/include/plan9cog/temporal.h` | Temporal header |

### Existing Files to Modify
| File | Changes |
|------|---------|
| `sys/src/libatomspace/serialize.c` | Complete import |
| `sys/src/libplan9cog/machspace.c` | 9P connections |
| `sys/src/libplan9cog/reactor.c` | Async processing |
| `sys/src/cmd/cogfs/cogfs.c` | Union dirs, rules |
| `sys/src/libplan9cog/plan9cog.c` | Statistics |

---

## Success Metrics

1. **Persistence:** Save/restore AtomSpace across sessions
2. **Distribution:** Mount remote cognitive services transparently
3. **Parallelism:** Process cognitive tasks across multiple cores
4. **Debuggability:** Inspect cognitive state via file reads
5. **Modularity:** Layer knowledge bases via union directories
