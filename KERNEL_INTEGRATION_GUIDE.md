# Kernel Integration Guide for Cognitive Extensions

This guide explains how to integrate the cognitive kernel extensions into a Plan 9 kernel build.

## Overview

The cognitive extensions add four new kernel modules that must be compiled and linked with the kernel:

1. `devcog.c` - Cognitive device driver
2. `cogvm.c` - Cognitive Virtual Machine
3. `cogproc.c` - Cognitive process management
4. `cogmem.c` - Cognitive memory management

## Step 1: Kernel Data Structure Extensions

### Modify `/sys/src/9/port/portdat.h`

Add cognitive types after existing typedef declarations:

```c
typedef struct CogProcExt CogProcExt;
typedef struct CogMemBlock CogMemBlock;
typedef struct CogProc CogProc;
```

Add cognitive extension to the `Proc` structure:

```c
struct Proc
{
    // ... existing fields ...
    
    /* Cognitive extensions */
    CogProcExt  *cogext;    /* Cognitive process extension */
    ulong       atomid;     /* Process's atom ID in kernel AtomSpace */
    
    /* ... rest of fields ... */
};
```

### Modify `/sys/src/9/port/portfns.h`

Add function declarations for cognitive subsystems:

```c
/* Cognitive VM */
void    cogvminit(void);
CogProc* cogproccreate(Proc*);
int     cogvmrun(CogProc*);
CogProc* cogschedule(void);
void    cogallocate(CogProc*, short, short);
void    cogdecay(float);
void    cogvmstats(ulong*, ulong*, int*);

/* Cognitive Process Management */
void    cogprocinit(void);
CogProcExt* cogprocalloc(void);
void    cogprocfree(CogProcExt*);
int     cogpriority(CogProcExt*);
int     cogtimeslice(CogProcExt*);
void    cogthink(CogProcExt*);
void    coginfer(CogProcExt*);
void    coglearn(CogProcExt*);
void    cogupdate(CogProcExt*, short, short);
void    cogdecayprocs(void);

/* Cognitive Memory Management */
void    cogmeminit(void);
void*   cogalloc(ulong, int, short, short);
void    cogfree(void*);
void    cogmemupdate(void*, short, short);
int     cogreclaim(ulong);
void    cogmemdecay(void);
void    cogmemstats(ulong*, ulong*, int*);
void*   cogallocatom(ulong, short);
void*   cogalloclink(ulong, short);
int     coggc(void);
```

## Step 2: Device Table Registration

### Modify architecture-specific device table

For PC architecture, edit `/sys/src/9/pc/devs`:

```
dev
    root
    cons
    env
    pipe
    dup
    ssl
    cap
    kprof
    fs
    srv
    mnt
    uart
    sd
    audio
    draw
    ip    ether netif
    eia    netif
    cog         # Add cognitive device
```

This makes the cognitive device available at `#Σ/`.

## Step 3: Kernel Initialization

### Modify `/sys/src/9/port/main.c`

Add cognitive initialization to `main()`:

```c
void
main(void)
{
    // ... existing initialization ...
    
    chandevinit();
    
    /* Initialize cognitive subsystems */
    cogmeminit();       /* Cognitive memory first */
    cogprocinit();      /* Then cognitive processes */
    cogvminit();        /* Finally cognitive VM */
    
    if(!waserror()){
        ksetenv("cputype", conffile, 0);
        // ... rest of initialization ...
    }
    
    // ... rest of main ...
}
```

### Modify process creation in `/sys/src/9/port/proc.c`

Add cognitive extension allocation in `newproc()`:

```c
Proc*
newproc(void)
{
    Proc *p;
    
    // ... existing process allocation ...
    
    /* Allocate cognitive extension */
    p->cogext = cogprocalloc();
    if(p->cogext == nil)
        print("newproc: warning: no cognitive extension\n");
    else
        p->cogext->atomid = 0;  /* Will be assigned later */
    
    // ... rest of initialization ...
    
    return p;
}
```

Add cognitive cleanup in `pexit()`:

```c
void
pexit(char *note, int freemem)
{
    Proc *p;
    
    // ... existing cleanup ...
    
    /* Free cognitive extension */
    if(up->cogext != nil) {
        cogprocfree(up->cogext);
        up->cogext = nil;
    }
    
    // ... rest of cleanup ...
}
```

## Step 4: Scheduler Integration

### Modify `/sys/src/9/port/proc.c` scheduler

Enhance `schedinit()` to consider cognitive importance:

```c
void
schedinit(void)
{
    // ... existing scheduler init ...
    
    /* Start cognitive scheduler */
    kproc("cogdecay", cogdecayproc, nil);
}

/* Cognitive decay background process */
static void
cogdecayproc(void*)
{
    for(;;) {
        tsleep(&up->sleep, return0, 0, 10000);  /* Every 10 seconds */
        cogdecayprocs();
        cogmemdecay();
    }
}
```

Enhance `runproc()` to consider cognitive priority:

```c
Proc*
runproc(void)
{
    Proc *p;
    ulong start;
    int cogpri;
    
    start = perfticks();
    
    // ... existing scheduler logic ...
    
    /* Adjust priority based on cognitive importance */
    if(p->cogext != nil) {
        cogpri = cogpriority(p->cogext);
        if(cogpri > 100)
            p->priority += 10;  /* Boost important cognitive processes */
        else if(cogpri < 10)
            p->priority -= 5;   /* Reduce unimportant processes */
    }
    
    // ... rest of scheduling ...
}
```

## Step 5: System Call Integration

### Add system call numbers in `/sys/src/9/port/portfns.h`

```c
/* Cognitive system calls */
#define COGTHINK    90
#define COGWAIT     91
#define COGINFER    92
#define COGFOCUS    93
#define COGSPREAD   94
```

### Add system call implementations in `/sys/src/9/port/syscall.c`

```c
/* System call handler */
void*
syscall(Ar0* ar0)
{
    // ... existing system calls ...
    
    case COGTHINK:
        return (void*)syscogthink(
            (int)ar0->a0,      /* op */
            (int)ar0->a1,      /* arg1 */
            (int)ar0->a2,      /* arg2 */
            (void*)ar0->a3);   /* data */
    
    case COGWAIT:
        return (void*)syscogwait();
    
    case COGINFER:
        return (void*)syscoginfer(
            (int)ar0->a0,      /* rule */
            (ulong*)ar0->a1,   /* atoms */
            (int)ar0->a2);     /* natoms */
    
    case COGFOCUS:
        return (void*)syscogfocus((ulong)ar0->a0);
    
    case COGSPREAD:
        return (void*)syscogspread(
            (ulong)ar0->a0,    /* atomid */
            (short)ar0->a1);   /* amount */
    
    // ... rest of system calls ...
}
```

Add libc wrappers in `/sys/src/libc/9syscall/sys.h`:

```c
#define COGTHINK    90
#define COGWAIT     91
#define COGINFER    92
#define COGFOCUS    93
#define COGSPREAD   94
```

## Step 6: Build Configuration

### Modify architecture-specific mkfile

For PC, edit `/sys/src/9/pc/mkfile`:

```mk
# Add cognitive object files
COG_OBJ=\
    devcog.$O\
    cogvm.$O\
    cogproc.$O\
    cogmem.$O\

# Add to main object list
OBJ=\
    $CONF.root.$O\
    $CONF.$O\
    $DEVS\
    $PORT\
    $COG_OBJ\    # Add cognitive objects
```

## Step 7: Build and Install

```bash
cd /sys/src/9/pc  # Or your architecture
mk clean
mk
mk install
```

## Step 8: Test Cognitive Kernel

After rebooting with the new kernel:

```bash
# Check cognitive device exists
ls '#Σ'

# Test kernel AtomSpace
cat '#Σ/atomspace'

# Test cognitive VM
cat '#Σ/cogvm'

# Test statistics
cat '#Σ/stats'

# Create atom at kernel level
echo 'create 1 test_concept' > '#Σ/atomspace'

# Verify creation
cat '#Σ/atomspace'
```

## Troubleshooting

### Device not found

If `#Σ` doesn't exist:
1. Check `devs` file includes `cog`
2. Verify `devcog.c` compiled successfully
3. Check device table registration

### Kernel panics

If kernel panics during boot:
1. Check cognitive init order (memory first, then proc, then VM)
2. Verify all function declarations match implementations
3. Add debug prints to track initialization

### System calls fail

If cognitive system calls return errors:
1. Check system call numbers don't conflict
2. Verify system call handler registered
3. Test with simple operations first

## Performance Monitoring

Monitor cognitive kernel performance:

```bash
# Watch cognitive statistics
while true; do cat '#Σ/stats'; sleep 5; done

# Monitor cognitive VM
cat '#Σ/cogvm'

# Monitor memory usage
cat '#Σ/atomspace' | grep 'Memory:'
```

## Security Considerations

### Access Control

The cognitive device should be protected:

```bash
# Set permissions on cognitive device
chmod 600 '#Σ/atomspace'  # Only owner can modify
chmod 400 '#Σ/cogvm'       # Read-only VM state
```

### Resource Limits

Set cognitive resource limits in `/sys/src/9/port/devcog.c`:

```c
enum {
    MaxAtoms = 1000000,      // Maximum atoms in kernel
    MaxCogMem = 1024*1024*1024,  // 1GB cognitive memory
    MaxInferDepth = 100,     // Maximum inference depth
};
```

## Integration Checklist

- [ ] Modify `portdat.h` - Add cognitive types and Proc extensions
- [ ] Modify `portfns.h` - Add function declarations
- [ ] Update device table - Add cognitive device
- [ ] Modify `main.c` - Add initialization
- [ ] Modify `proc.c` - Add process extensions
- [ ] Modify scheduler - Add cognitive priority
- [ ] Add system calls - Implement cognitive syscalls
- [ ] Update mkfile - Add cognitive objects
- [ ] Build kernel - Compile and link
- [ ] Test device - Verify `#Σ/` works
- [ ] Test operations - Create atoms, run inference
- [ ] Monitor performance - Check statistics

## Future Enhancements

1. **Persistent AtomSpace** - Save/restore kernel knowledge
2. **Network transparency** - Share cognitive state across systems
3. **GPU acceleration** - Hardware-accelerated inference
4. **Self-optimization** - Kernel learns and adapts
5. **Formal verification** - Prove cognitive correctness

## References

- Plan 9 Device Driver Guide
- Plan 9 Kernel Programming
- Inferno Virtual Machine Specification
- OpenCog AtomSpace Documentation

---

**Note:** This integration makes cognitive processing a fundamental kernel service. All processes inherit cognitive capabilities automatically.
