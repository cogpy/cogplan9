# Enhanced URE with File Interface - Sprint 3

## Overview

Sprint 3 enhances the Unified Rule Engine (URE) with a file-based interface for rule management and inference debugging. This makes PLN inference transparent, controllable, and debuggable through Plan 9's file interface.

## Architecture

### Rules as Files

The `/mnt/cog/rules/` hierarchy exposes inference rules as files:

```
/mnt/cog/rules/
├── deduction/
│   ├── ModusPonens/
│   │   ├── ctl          # Enable/disable, set weight
│   │   ├── definition   # Rule pattern
│   │   ├── stats        # Usage statistics
│   │   └── trace        # Execution trace
│   ├── Syllogism/
│   └── ...
├── induction/
│   ├── Direct/
│   ├── Abductive/
│   └── ...
├── abduction/
│   └── Simple/
└── custom/
    └── ...
```

### Inference Debugging

The `/mnt/cog/inference/` hierarchy provides debugging facilities:

```
/mnt/cog/inference/
├── active       # List of running inferences
├── trace        # Inference execution trace
├── breakpoints  # Breakpoint management
├── ctl          # Control interface
├── stats        # Inference statistics
└── <id>/        # Per-inference state
    ├── state    # Current state
    ├── steps    # Inference steps
    ├── result   # Inference result
    └── ctl      # Control this inference
```

## Rule Management

### Inspecting Rules

```bash
# List all rules
ls /mnt/cog/rules/*/*/

# View rule definition
cat /mnt/cog/rules/deduction/ModusPonens/definition

# Check rule statistics
cat /mnt/cog/rules/deduction/ModusPonens/stats
```

### Controlling Rules

```bash
# Disable a rule
echo 'disable' > /mnt/cog/rules/deduction/ModusPonens/ctl

# Set rule weight
echo 'weight 1.5' > /mnt/cog/rules/deduction/ModusPonens/ctl

# Enable a rule
echo 'enable' > /mnt/cog/rules/induction/Direct/ctl

# Reset statistics
echo 'reset' > /mnt/cog/rules/deduction/Syllogism/ctl
```

### Rule Statistics

```bash
$ cat /mnt/cog/rules/deduction/ModusPonens/stats
name deduction.ModusPonens
type 1
enabled 1
weight 1.000
applied 1234
succeeded 1150
failed 84
successrate 93.19%
totaltime 456789 µs
avgtime 370.12 µs
```

### Rule Definition Format

```bash
$ cat /mnt/cog/rules/deduction/ModusPonens/definition
# Rule: deduction.ModusPonens
# Type: 1
# Pattern:
Implication(A,B) & A => B
```

## Inference Debugging

### Monitoring Active Inferences

```bash
# List active inferences
cat /mnt/cog/inference/active

# Output:
# Active inferences: 3
# id state goal steps rules time
42 running 12345 15 23 1234
43 blocked 23456 8 10 567
44 complete 34567 25 35 2345
```

### Watching Inference Trace

```bash
# Read trace buffer
cat /mnt/cog/inference/trace

# Continuous monitoring
tail -f /mnt/cog/inference/trace
```

### Setting Breakpoints

```bash
# Break on any rule application
echo 'break * 0' > /mnt/cog/inference/breakpoints

# Break on specific rule
echo 'break deduction.ModusPonens 0' > /mnt/cog/inference/breakpoints

# Break on specific atom
echo 'break * 12345' > /mnt/cog/inference/breakpoints

# List breakpoints
cat /mnt/cog/inference/breakpoints

# Output:
# Breakpoints (2)
# id enabled rule atom hits
0 E deduction.ModusPonens 0 5
1 E * 12345 2
```

### Managing Breakpoints

```bash
# Enable breakpoint
echo 'enable 0' > /mnt/cog/inference/ctl

# Disable breakpoint
echo 'disable 1' > /mnt/cog/inference/ctl

# Clear all breakpoints
echo 'clearbreakpoints' > /mnt/cog/inference/ctl
```

### Controlling Trace

```bash
# Enable tracing
echo 'trace on' > /mnt/cog/inference/ctl

# Disable tracing
echo 'trace off' > /mnt/cog/inference/ctl

# Clear trace buffer
echo 'cleartrace' > /mnt/cog/inference/ctl
```

### Per-Inference Inspection

```bash
# View inference state
cat /mnt/cog/inference/42/state

# View inference steps
cat /mnt/cog/inference/42/steps

# Output:
# Step 0: deduction.ModusPonens -> 9999 (123456 µs)
# Step 1: deduction.Syllogism -> 10000 (234567 µs)
# ...

# View inference result
cat /mnt/cog/inference/42/result

# Control inference
echo 'pause' > /mnt/cog/inference/42/ctl
echo 'resume' > /mnt/cog/inference/42/ctl
echo 'step' > /mnt/cog/inference/42/ctl  # Single step
echo 'abort' > /mnt/cog/inference/42/ctl
```

## Programming Interface

### C API

```c
#include <u.h>
#include <libc.h>

/* Start inference with debugging */
int
startinference(ulong goalatom)
{
	int infid;
	
	/* Start inference session */
	infid = coginfstart(goalatom);
	
	/* Set breakpoint on specific rule */
	coginfaddbreakpoint("deduction.ModusPonens", 0);
	
	return infid;
}

/* Record inference step */
void
recordstep(int infid, char *rule, ulong atomid)
{
	char detail[256];
	
	snprint(detail, sizeof detail, "Applied %s to produce %ld", rule, atomid);
	coginfstep(infid, rule, atomid, detail);
	
	/* Update rule statistics */
	cogurestats(rule, 1, 123);  /* succeeded, 123µs */
}

/* Complete inference */
void
completeinference(int infid, ulong result, float confidence)
{
	coginfcomplete(infid, result, confidence);
}
```

### Shell Script Example

```bash
#!/bin/rc
# Monitor inference performance

while(true) {
	clear
	echo '=== Inference Statistics ==='
	cat /mnt/cog/inference/stats
	echo
	echo '=== Active Inferences ==='
	cat /mnt/cog/inference/active
	echo
	echo '=== Top Rules ==='
	for(rule in /mnt/cog/rules/*/*/stats) {
		cat $rule | grep 'applied'
	}
	sleep 1
}
```

## Default PLN Rules

The system includes these default rules:

### Deduction Rules

1. **ModusPonens**: `Implication(A,B) & A => B`
2. **Syllogism**: `Inheritance(A,B) & Inheritance(B,C) => Inheritance(A,C)`
3. **Contraposition**: `Implication(A,B) => Implication(Not(B),Not(A))`

### Induction Rules

1. **Direct**: `Evaluation(P,A) & Evaluation(P,B) => Similarity(A,B)`
2. **Abductive**: `Implication(A,B) & B => A [weak]`

### Abduction Rules

1. **Simple**: `Implication(A,B) & B => A [hypothesis]`

### Revision Rules

1. **TruthValue**: `Atom(A,TV1) & Atom(A,TV2) => Atom(A,Revised(TV1,TV2))`

## Performance Monitoring

### Rule Performance

```bash
# Find slowest rules
for(rule in /mnt/cog/rules/*/*/stats) {
	cat $rule | grep avgtime
} | sort -n -k 2 | tail -5

# Find least successful rules
for(rule in /mnt/cog/rules/*/*/stats) {
	cat $rule | grep successrate
} | sort -n -k 2 | head -5

# Total rules applied
for(rule in /mnt/cog/rules/*/*/stats) {
	cat $rule | grep applied
} | awk '{sum += $2} END {print sum}'
```

### Inference Performance

```bash
# Average inference time
cat /mnt/cog/inference/active | awk '{sum += $7; count++} END {print sum/count}'

# Longest running inference
cat /mnt/cog/inference/active | sort -n -k 7 | tail -1

# Most complex inference (most steps)
cat /mnt/cog/inference/active | sort -n -k 4 | tail -1
```

## Integration with PLN

The URE file interface integrates with the PLN inference engine:

```c
/* In PLN inference engine */
void
applyrule(char *rulename, Atom *premise, Atom *conclusion)
{
	vlong starttime, endtime;
	int succeeded;
	
	starttime = nsec();
	
	/* Attempt rule application */
	succeeded = tryapplyrule(rulename, premise, conclusion);
	
	endtime = nsec();
	
	/* Update statistics */
	cogurestats(rulename, succeeded, (endtime - starttime) / 1000);
	
	/* Record trace */
	if(succeeded){
		char trace[512];
		snprint(trace, sizeof trace,
			"Rule %s: %ld + %ld => %ld\n",
			rulename, premise->id, premise->id, conclusion->id);
		coguretrace(rulename, trace, strlen(trace));
	}
}
```

## Debugging Workflow

### 1. Enable Tracing

```bash
echo 'trace on' > /mnt/cog/inference/ctl
```

### 2. Set Breakpoints

```bash
# Break on problematic rule
echo 'break deduction.Syllogism 0' > /mnt/cog/inference/breakpoints
```

### 3. Run Inference

```bash
# Trigger inference (from your application)
echo 'infer 12345' > /mnt/cog/ctl
```

### 4. Monitor Progress

```bash
# Watch trace
tail -f /mnt/cog/inference/trace

# Check active inferences
watch -n 1 cat /mnt/cog/inference/active
```

### 5. Inspect Blocked Inference

```bash
# Find blocked inference
cat /mnt/cog/inference/active | grep blocked

# View its state
cat /mnt/cog/inference/43/state
cat /mnt/cog/inference/43/steps
```

### 6. Step Through

```bash
# Single step
echo 'step' > /mnt/cog/inference/43/ctl

# Check result
cat /mnt/cog/inference/43/steps | tail -1
```

### 7. Continue or Abort

```bash
# Continue execution
echo 'resume' > /mnt/cog/inference/43/ctl

# Or abort
echo 'abort' > /mnt/cog/inference/43/ctl
```

## Benefits

### Transparency

- **Visible state** - All inference state accessible as files
- **Observable execution** - Watch inference in real-time
- **Inspectable results** - Examine outputs step-by-step

### Controllability

- **Rule management** - Enable/disable/tune rules dynamically
- **Execution control** - Pause/resume/step through inference
- **Breakpoints** - Stop at specific points

### Debuggability

- **Trace analysis** - Review complete execution history
- **Performance profiling** - Identify slow rules
- **Correctness verification** - Validate inference steps

### Plan 9 Integration

- **Network transparency** - Debug remote inferences via 9P
- **Tool integration** - Use standard Unix tools (grep, awk, etc.)
- **Scriptability** - Automate with shell scripts

## Implementation Files

- `sys/src/9/port/devcogure.c` - URE file interface (400 lines)
- `sys/src/9/port/devcoginf.c` - Inference debugging (500 lines)
- Integration with `sys/src/libpln/ure.c`
- Integration with `sys/src/cmd/cogfs/cogfs.c`

## See Also

- `pln(3)` - PLN inference engine
- `ure(3)` - Unified Rule Engine
- `cogfs(4)` - Cognitive file server
- ROADMAP.md - Sprint 3 objectives

## References

- Goertzel, B., et al. "Probabilistic Logic Networks"
- Plan 9 from Bell Labs documentation
- Plan9Cog Architecture Documentation

---

**Status:** Complete  
**Sprint:** 3 (ROADMAP.md)  
**Lines:** ~900 C code + documentation
