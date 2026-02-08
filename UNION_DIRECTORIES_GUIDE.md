# Union Directories for Knowledge Layering

## Overview

Plan 9's bind and mount operations allow creating union directories where multiple filesystems are layered together. This enables hierarchical knowledge bases in Plan9Cog where general knowledge can be overlaid with domain-specific and user-specific knowledge.

## Concept

```
/mnt/cog/
├── [base layer]      ← Fundamental concepts
├── [domain layer]    ← Task-specific knowledge  
└── [user layer]      ← Personal preferences
```

When a file is accessed in a union directory, Plan 9 searches through all layers in order, allowing specialized knowledge to override or supplement general knowledge.

## Basic Usage

### Creating a Union Directory

```bash
# Mount base cognitive filesystem
mount /srv/cogfs /mnt/cog

# Add domain-specific layer (append with -a)
bind -a /mnt/cog/domain /mnt/cog

# Add user-specific layer (prepend with -b)
bind -b /mnt/cog/user /mnt/cog

# Result: reads search user → domain → base
```

### Bind Flags

- **MREPL** (no flag) - Replace old with new
- **-a** (MAFTER) - Add new after old in union
- **-b** (MBEFORE) - Add new before old in union  
- **-c** (MCREATE) - Create union directory if needed

## Cognitive Knowledge Layering

### Three-Tier Architecture

```bash
#!/bin/rc
# setup-cog-namespace.rc
# Sets up layered cognitive namespace

# 1. Base knowledge - fundamental concepts
mount /srv/cogfs.base /mnt/cog

# 2. Domain knowledge - task-specific
bind -a /mnt/cog.domain /mnt/cog

# 3. User knowledge - personal
bind -a /mnt/cog.user /mnt/cog

echo 'Cognitive namespace ready:'
echo '  Base:   /srv/cogfs.base'
echo '  Domain: /mnt/cog.domain'
echo '  User:   /mnt/cog.user'
echo '  Union:  /mnt/cog'
```

### Example Directory Structure

```
/mnt/cog/
  atoms              # Union of all atom stores
  concepts/          # Conceptual knowledge
    fundamental/     # From base layer
    medical/         # From domain layer  
    preferences/     # From user layer
  rules/             # Inference rules
    core/            # Base reasoning rules
    specialized/     # Domain-specific rules
    custom/          # User-defined rules
```

## Practical Examples

### Example 1: Medical AI System

```bash
#!/bin/rc
# Medical AI with specialized knowledge

# Base: General medical knowledge
mount /srv/cogfs.medical.base /n/medbase

# Domain: Cardiology specialization
mount /srv/cogfs.cardiology /n/cardio

# User: Doctor's preferences and cases
mount /srv/cogfs.drsmith /n/doctor

# Create union at /mnt/cog
mkdir -p /mnt/cog
mount /n/medbase /mnt/cog
bind -a /n/cardio /mnt/cog
bind -a /n/doctor /mnt/cog

# Now queries to /mnt/cog search all layers
cat /mnt/cog/concepts/heart_disease
# Searches: drsmith → cardiology → medical.base
```

### Example 2: Multi-Domain Agent

```bash
#!/bin/rc
# Agent with multiple domains of expertise

# Base cognitive functions
mount /srv/cogfs.base /mnt/cog

# Mathematics domain
bind -a /mnt/domains/math /mnt/cog

# Physics domain  
bind -a /mnt/domains/physics /mnt/cog

# Chemistry domain
bind -a /mnt/domains/chemistry /mnt/cog

# Result: Agent has access to all domains
# with base cognitive functions available to all
```

### Example 3: Personalized Learning System

```bash
#!/bin/rc
# Student learning environment

student=$1

# Curriculum base
mount /srv/cogfs.curriculum /n/curriculum

# Student progress and preferences
mount /srv/cogfs.student.$student /n/student

# Create personalized namespace
mkdir -p /mnt/learn
mount /n/curriculum /mnt/learn
bind -a /n/student /mnt/learn

# Student's view includes:
# - Standard curriculum (base)
# - Personal progress tracking (overlay)
# - Customized difficulty levels (overlay)
```

## Advanced Patterns

### Dynamic Layer Management

```c
#include <u.h>
#include <libc.h>

void
addlayer(char *name, char *path)
{
	char cmd[256];
	
	/* Mount new layer */
	snprint(cmd, sizeof cmd, "bind -a %s /mnt/cog", path);
	if(system(cmd) < 0)
		sysfatal("bind: %r");
	
	print("Added layer: %s at %s\n", name, path);
}

void
removelayer(char *path)
{
	char cmd[256];
	
	snprint(cmd, sizeof cmd, "unmount %s /mnt/cog", path);
	if(system(cmd) < 0)
		sysfatal("unmount: %r");
	
	print("Removed layer: %s\n", path);
}

void
main(int argc, char *argv[])
{
	if(argc < 3)
		sysfatal("usage: %s <add|remove> <layer> [path]", argv[0]);
	
	if(strcmp(argv[1], "add") == 0 && argc >= 4)
		addlayer(argv[2], argv[3]);
	else if(strcmp(argv[1], "remove") == 0)
		removelayer(argv[2]);
	else
		sysfatal("invalid command");
	
	exits(nil);
}
```

### Context-Sensitive Layering

```bash
#!/bin/rc
# Adjust knowledge layers based on context

fn settask {
	task=$1
	
	# Remove old task layers
	for(l in /mnt/cog.task.*) {
		if(test -d $l)
			unmount $l /mnt/cog
	}
	
	# Add new task layer
	if(test -d /mnt/cog.task.$task) {
		bind -a /mnt/cog.task.$task /mnt/cog
		echo 'Task context:' $task
	}
}

# Switch between tasks
settask diagnosis
# ... perform diagnosis ...

settask treatment
# ... plan treatment ...

settask research  
# ... research new approaches ...
```

### Collaborative Knowledge Building

```bash
#!/bin/rc
# Multiple agents contributing to shared knowledge

agent=$1

# Global shared knowledge
mount /srv/cogfs.shared /n/shared

# Agent-specific knowledge
mount /srv/cogfs.agent.$agent /n/agent

# Agent's working namespace
mkdir -p /mnt/work
mount /n/shared /mnt/work
bind -b /n/agent /mnt/work  # Agent's knowledge searched first

# Agent can:
# - Read from shared knowledge
# - Write to own knowledge (appears in union)
# - Contributions can be merged to shared later
```

## Implementation in Plan9Cog

### CogFS Union Support

The cogfs file server supports union directories naturally through 9P:

```c
/* From cogfs.c */
void
fsopen(Req *r)
{
	Fid *f = r->fid;
	
	/* Union directory - search all layers */
	if(f->qid.type & QTDIR){
		/* Collect entries from all bound layers */
		unionread(f);
	}
	
	respond(r, nil);
}
```

### MachSpace Layer Management

```c
#include <plan9cog.h>

void
addknowledgelayer(MachSpace *ms, char *name, char *path)
{
	/* Mount remote knowledge source */
	if(machspace9pconnect(ms, path) < 0)
		sysfatal("cannot add layer: %r");
	
	/* Layer automatically appears in union */
	print("Knowledge layer '%s' added\n", name);
}
```

## Performance Considerations

### Lookup Cost

- **Single layer**: O(1) file access
- **Union (n layers)**: O(n) in worst case
- **Cached**: Most lookups cached in kernel

### Best Practices

1. **Order matters** - Place most frequently accessed layers first
2. **Limit layers** - Keep to 3-5 layers for best performance
3. **Use caching** - Enable kernel caching for stable layers
4. **Consolidate** - Merge layers periodically

## Debugging Union Directories

### Inspecting Layers

```bash
# See what's bound to /mnt/cog
cat /proc/self/ns | grep /mnt/cog

# Test lookup order
echo test > /mnt/cog/testfile
grep -r testfile /n/*

# Remove all bindings
unmount /mnt/cog
```

### Tracing Lookups

```bash
# Enable 9P trace
echo 'trace' > /srv/cogfs.ctl

# Perform operation
cat /mnt/cog/concepts/test

# View trace
cat /srv/cogfs.trace
```

## Integration with Per-Process Namespaces

Each process can have its own union directory configuration:

```bash
#!/bin/rc
# Per-process cognitive context

rfork n  # New namespace

# Process gets its own union
mount /srv/cogfs.base /mnt/cog
bind -a /proc/self/cog /mnt/cog  # Process-specific layer

# Now this process has:
# - Base knowledge (shared)
# - Process-specific knowledge (private)
```

## Real-World Scenarios

### Scenario 1: Multi-Tenant System

```bash
# Each tenant gets base knowledge + tenant-specific
tenant=acme

mount /srv/cogfs.base /mnt/cog
bind -a /srv/cogfs.tenant.$tenant /mnt/cog

# Tenant sees: their data + base knowledge
# Security: tenant data isolated from others
```

### Scenario 2: Experimental Features

```bash
# Base stable knowledge
mount /srv/cogfs.stable /mnt/cog

# Experimental features layer
if(test $EXPERIMENTAL = 1) {
	bind -a /srv/cogfs.experimental /mnt/cog
	echo 'Experimental features enabled'
}

# Easy to enable/disable experiments
```

### Scenario 3: Temporal Knowledge

```bash
# Current knowledge
mount /srv/cogfs.current /mnt/cog

# Historical snapshots
bind -a /srv/cogfs.snapshot.yesterday /mnt/cog.history
bind -a /srv/cogfs.snapshot.lastweek /mnt/cog.history

# Compare current vs historical
diff /mnt/cog/beliefs /mnt/cog.history/beliefs
```

## See Also

- `bind(1)` - Bind and mount operations
- `mount(1)` - Mount filesystems
- `ns(1)` - Display namespace
- `proc(3)` - Process namespace
- `9p(2)` - 9P file protocol

## References

- Pike, R., et al. "Plan 9 from Bell Labs: Namespaces"
- Plan9Cog ROADMAP.md - Sprint 2 objectives
- PROC_COG_INTERFACE.md - Per-process cognitive state

---

**Status:** Complete  
**Sprint:** 2 (ROADMAP.md)  
**Integration:** Automatic via Plan 9 bind/mount
