# /proc/pid/cog - Per-Process Cognitive Namespace

## Overview

The `/proc/pid/cog/` directory provides a file-based interface to per-process cognitive state in Plan9Cog. This leverages Plan 9's philosophy of "everything is a file" to expose cognitive processing as a natural extension of the process model.

## Architecture

Each process can have its own cognitive namespace accessible via `/proc/<pid>/cog/`. This enables:

- **Process-specific cognitive contexts** - Each process maintains its own goals, beliefs, and attentional focus
- **Debuggable cognitive state** - Inspect what any process is "thinking" by reading files
- **Network transparency** - Cognitive state accessible over 9P protocol
- **Union directories** - Layer knowledge bases using Plan 9's bind/mount

## Directory Structure

```
/proc/<pid>/cog/
├── atoms      # Process's relevant atoms (read-only)
├── focus      # Attentional focus (read-write)
├── goals      # OpenPsi goals (read-write)
├── beliefs    # Belief state (read-write)
├── ctl        # Control interface (write-only)
└── stats      # Statistics (read-only)
```

## File Descriptions

### atoms (0444 - read-only)

Lists all atoms relevant to this process, including:
- Current focus atom
- Goal atoms
- Belief atoms

**Format:**
```
# Relevant atoms for process <pid>
focus <atom_id>
goal <atom_id>
goal <atom_id>
belief <atom_id>
belief <atom_id>
```

**Example:**
```bash
cat /proc/self/cog/atoms
# Relevant atoms for process 42
focus 12345
goal 23456
goal 34567
belief 45678
```

### focus (0644 - read-write)

Controls the process's attentional focus.

**Read format:**
```
atom <id> sti <threshold>
```

**Write commands:**
```
atom <id>          # Set focus to atom ID
sti <threshold>    # Set STI threshold
```

**Examples:**
```bash
# Read current focus
cat /proc/self/cog/focus
atom 12345 sti 100

# Set focus to atom 9999
echo 'atom 9999' > /proc/self/cog/focus

# Set STI threshold to 50
echo 'sti 50' > /proc/self/cog/focus
```

### goals (0644 - read-write)

Manages OpenPsi goals for the process.

**Read format:**
```
# OpenPsi goals for process <pid>
<atom_id>
<atom_id>
...
```

**Write commands:**
```
add <atom_id>      # Add a goal
remove <atom_id>   # Remove a goal
del <atom_id>      # Remove a goal (alias)
```

**Examples:**
```bash
# List current goals
cat /proc/self/cog/goals
# OpenPsi goals for process 42
12345
23456

# Add a new goal
echo 'add 34567' > /proc/self/cog/goals

# Remove a goal
echo 'remove 12345' > /proc/self/cog/goals
```

### beliefs (0644 - read-write)

Manages belief atoms for the process.

**Read format:**
```
# Beliefs for process <pid>
<atom_id>
<atom_id>
...
```

**Write commands:**
```
add <atom_id>      # Add a belief
remove <atom_id>   # Remove a belief
del <atom_id>      # Remove a belief (alias)
```

**Examples:**
```bash
# List current beliefs
cat /proc/self/cog/beliefs
# Beliefs for process 42
45678
56789

# Add a belief
echo 'add 67890' > /proc/self/cog/beliefs

# Remove a belief
echo 'del 45678' > /proc/self/cog/beliefs
```

### ctl (0220 - write-only)

Control interface for cognitive operations.

**Commands:**
```
inference          # Trigger an inference step
reset              # Reset all statistics
cleargoals         # Clear all goals
clearbeliefs       # Clear all beliefs
```

**Examples:**
```bash
# Trigger inference
echo 'inference' > /proc/self/cog/ctl

# Reset statistics
echo 'reset' > /proc/self/cog/ctl

# Clear all goals
echo 'cleargoals' > /proc/self/cog/ctl
```

### stats (0444 - read-only)

Displays cognitive statistics for the process.

**Format:**
```
pid <pid>
inferences <count>
focuses <count>
goalupdates <count>
ngoals <count>
nbeliefs <count>
focusatom <atom_id>
focussti <threshold>
```

**Example:**
```bash
cat /proc/self/cog/stats
pid 42
inferences 1234
focuses 56
goalupdates 78
ngoals 3
nbeliefs 5
focusatom 12345
focussti 100
```

## Usage Patterns

### Monitoring Cognitive State

```bash
# Watch a process's cognitive activity
while true; do
    clear
    echo "=== Process Cognitive State ==="
    cat /proc/42/cog/stats
    sleep 1
done
```

### Setting Process Goals

```bash
# Set goals for a cognitive task
echo 'add 1001' > /proc/self/cog/goals  # Goal: learn_pattern
echo 'add 1002' > /proc/self/cog/goals  # Goal: maximize_accuracy
echo 'add 1003' > /proc/self/cog/goals  # Goal: minimize_cost
```

### Debugging Cognitive Processes

```bash
# Inspect what a process is "thinking about"
cat /proc/42/cog/atoms
cat /proc/42/cog/focus
cat /proc/42/cog/goals

# Check cognitive statistics
cat /proc/42/cog/stats
```

### Building Cognitive Agents

```c
#include <u.h>
#include <libc.h>

void
main(void)
{
	int fd;
	char buf[256];
	
	/* Set initial goal */
	fd = open("/proc/self/cog/goals", OWRITE);
	if(fd >= 0){
		fprint(fd, "add 5001\n");  /* Goal: explore */
		close(fd);
	}
	
	/* Set attentional focus */
	fd = open("/proc/self/cog/focus", OWRITE);
	if(fd >= 0){
		fprint(fd, "atom 6001\n");  /* Focus on concept */
		fprint(fd, "sti 75\n");     /* Threshold */
		close(fd);
	}
	
	/* Trigger inference */
	fd = open("/proc/self/cog/ctl", OWRITE);
	if(fd >= 0){
		fprint(fd, "inference\n");
		close(fd);
	}
	
	/* Check results */
	fd = open("/proc/self/cog/stats", OREAD);
	if(fd >= 0){
		read(fd, buf, sizeof buf);
		print("%s\n", buf);
		close(fd);
	}
	
	exits(nil);
}
```

## Integration with Kernel

The `/proc/pid/cog/` interface is implemented by the `devcogproc.c` device driver. It integrates with:

1. **Process Structure** - `Proc->cogext` points to `CogProcState`
2. **Process Exit** - `cogprocstatefree()` called from `pexit()`
3. **Kernel AtomSpace** - Accesses `#Σ/` device for atom operations
4. **OpenPsi** - Goals managed via `/proc/pid/cog/goals`
5. **ECAN** - Focus managed via `/proc/pid/cog/focus`

## Union Directories for Knowledge Layering

Plan 9's bind/mount operations allow layering cognitive knowledge:

```bash
# Create base cognitive namespace
mkdir /mnt/cog
mount /srv/cogfs /mnt/cog

# Layer domain-specific knowledge
bind -a /mnt/cog/domain /mnt/cog

# Add user-specific knowledge
bind -a /mnt/cog/user /mnt/cog

# Now reads from /mnt/cog search through all layers
```

This enables:
- **Base knowledge** - Fundamental concepts
- **Domain overlays** - Task-specific knowledge
- **User customization** - Personal preferences

## Network Transparency

Access cognitive state over the network:

```bash
# Mount remote process namespace
import remotehost /proc /n/remote

# Inspect remote process cognitive state
cat /n/remote/42/cog/stats
cat /n/remote/42/cog/goals

# Modify remote process goals
echo 'add 9999' > /n/remote/42/cog/goals
```

## Performance Considerations

- **Lightweight** - Cognitive state stored in kernel, minimal overhead
- **Zero-copy** - File reads return formatted strings, no serialization
- **Scalable** - Each process maintains small state (< 1KB)
- **Fast access** - Direct pointer from Proc to CogProcState

## Security Model

File permissions control access:
- **atoms** (0444) - Anyone can inspect
- **stats** (0444) - Anyone can read statistics
- **focus** (0644) - Owner can read/write
- **goals** (0644) - Owner can read/write
- **beliefs** (0644) - Owner can read/write
- **ctl** (0220) - Owner can write only

## Comparison with Traditional Models

| Traditional | Plan9Cog /proc/cog |
|------------|-------------------|
| IPC messages | File reads/writes |
| Serialization | Direct string format |
| API calls | `cat`/`echo` commands |
| Custom protocols | Standard 9P |
| Complex debugging | Simple file inspection |

## Future Extensions

- **Temporal views** - `/proc/pid/cog/t-1h/` for historical state
- **Inference traces** - `/proc/pid/cog/trace` for debugging
- **Rule activation** - `/proc/pid/cog/rules/` for active rules
- **Pattern matches** - `/proc/pid/cog/patterns/` for matched patterns

## See Also

- `devcog(4)` - Kernel cognitive device (#Σ/)
- `devproc(4)` - Process filesystem
- `proc(3)` - Process management
- `bind(1)` - Namespace manipulation
- `import(4)` - Remote namespace import

## References

- Pike, R., et al. "Plan 9 from Bell Labs"
- Goertzel, B., et al. "OpenCog: A Software Framework for Integrative AGI"
- Plan9Cog Architecture Documentation

---

**Implementation:** Sprint 2 (ROADMAP.md)  
**Status:** Complete  
**File:** `sys/src/9/port/devcogproc.c`  
**Lines:** ~500
