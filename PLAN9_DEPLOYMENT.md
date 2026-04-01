# Pure Plan 9 Deployment Guide

This document answers the question: **"How could a pure Plan 9 deployment of cogplan9 be achieved?"**

## Overview

cogplan9 ships two complementary build paths:

| Path | Toolchain | Target |
|------|-----------|--------|
| CMake (`CMakeLists.txt`) | Any C99 compiler (GCC, Clang, MSVC) | Linux / macOS / Windows |
| Plan 9 `mk` (`sys/src/*/mkfile`) | Plan 9 `8c`/`8l` or plan9port `9c`/`9l` | Native Plan 9 or plan9port |

A *pure Plan 9 deployment* means building and running cogplan9 using only
Plan 9 tools — either on real Plan 9 fourth edition, or on
[plan9port](https://github.com/9fans/plan9port) (Plan 9 from User Space).

---

## Option 1 – Real Plan 9 (Fourth Edition)

### Prerequisites

Boot a Plan 9 installation from a disk image or physical hardware.
The 386, amd64, arm, and power architectures are all supported.

### Build steps

```
% cd /sys/src/libatomspace
% mk install

% cd /sys/src/libpln
% mk install

% cd /sys/src/libplan9cog
% mk install

% cd /sys/src/cmd/cogctl
% mk install

% cd /sys/src/cmd/cogfs
% mk install
```

`mk install` compiles with the appropriate Plan 9 C compiler for the
current architecture (`8c` on 386, `6c` on amd64, etc.) and installs
the results under `/386/lib/`, `/386/bin/`, etc.

### Running cogfs as a file server

Mount the cognitive file server at `/mnt/cog`:

```
% cogfs /mnt/cog &
% ls /mnt/cog
atomspace  pln  ecan  ctl  stats
```

Interact with the kernel AtomSpace through the 9P hierarchy:

```
% echo '(ConceptNode "hello")' > /mnt/cog/atomspace
% cat /mnt/cog/stats
```

### Kernel build (optional)

To include the cognitive kernel modules (`devcog.c`, `cogvm.c`,
`cogproc.c`, `cogmem.c`) in a custom kernel:

```
% cd /sys/src/9/pc
% mk 'COGFLAGS=-DCOGPLAN9'
% mk install
```

`-DCOGPLAN9` is the preprocessor flag that activates all `#ifdef COGPLAN9`
blocks in the kernel source — enabling the cognitive device driver `#Σ`,
the cognitive VM, cognitive process extensions, and the AtomSpace
allocator.  Without this flag the standard Plan 9 kernel is built with
no cognitive components.

The cognitive device `#Σ` will then appear in `/dev/` after boot.

---

## Option 2 – plan9port on Linux / macOS / WSL

[plan9port](https://github.com/9fans/plan9port) provides all Plan 9
tools (`mk`, `9c`, `rc`, `acme`, `9P`) on POSIX systems.

### Install plan9port

```sh
cd /usr/local
sudo git clone https://github.com/9fans/plan9port plan9
cd plan9 && sudo ./INSTALL
export PLAN9=/usr/local/plan9
export PATH=$PLAN9/bin:$PATH
```

### Build with plan9port mk

```sh
# Build the cognitive libraries
for lib in libatomspace libpln libplan9cog; do
    (cd sys/src/$lib && mk install)
done

# Build the command-line tools
for cmd in cogctl cogfs cogdemo; do
    (cd sys/src/cmd/$cmd && mk install)
done
```

The `mk` tool reads each directory's `mkfile` (the Plan 9 equivalent of
a `Makefile`) and compiles with `9c` / links with `9l`.

### Running tests with plan9port

```sh
rc test-plan9cog.rc
```

or with `sh`:

```sh
./test-plan9cog.sh
```

---

## Option 3 – GitHub Codespaces (Recommended for contributors)

The repository ships a pre-configured Dev Container
(`.devcontainer/`) that installs plan9port automatically.

1. Open the repository on GitHub.
2. Click **Code → Codespaces → Create codespace on main**.
3. Wait ~5 minutes for the container to build.
4. All Plan 9 and CMake tools are immediately available in the terminal.

```sh
# Build with CMake (cross-platform C library)
cmake -B build && cmake --build build

# Build with Plan 9 mk (native Plan 9 ABI)
for lib in libatomspace libpln libplan9cog; do
    (cd sys/src/$lib && mk install)
done
```

---

## Deployment Topology

```
                  ┌─────────────────────────────┐
                  │      Plan 9 kernel           │
                  │  ┌──────────┐  ┌──────────┐  │
                  │  │ devcog.c │  │ cogvm.c  │  │
                  │  │  (#Σ/)   │  │ cogproc  │  │
                  │  └──────────┘  └──────────┘  │
                  │        AtomSpace (kernel)      │
                  └──────────────┬──────────────┘
                                 │ 9P file interface
              ┌──────────────────▼──────────────────┐
              │              cogfs                   │
              │  /mnt/cog/atomspace   /mnt/cog/pln  │
              │  /mnt/cog/ecan        /mnt/cog/ctl  │
              └──────────────────┬──────────────────┘
                                 │
           ┌─────────────────────┼──────────────────┐
           │                     │                  │
    ┌──────▼──────┐     ┌────────▼───────┐  ┌──────▼──────┐
    │   cogctl    │     │ user programs  │  │  acme / rc  │
    │  (CLI tool) │     │  (any lang)    │  │  (scripts)  │
    └─────────────┘     └────────────────┘  └─────────────┘
```

Everything is accessible as ordinary files.  No special libraries or
runtime bindings are needed; any program that can open and read/write
files can use the cognitive services.

---

## Network-Transparent Distribution

Because cogfs speaks standard 9P, the cognitive file hierarchy can be
exported over the network with `exportfs` / `import`:

```
# On the server (Plan 9 box running cogfs):
cogfs /mnt/cog &
exportfs -r /mnt/cog tcp!*!17010

# On any client (Plan 9 or plan9port):
import tcp!cogserver!17010 /mnt/cog
```

The client sees the remote AtomSpace as local files.  Multiple clients
can share a single kernel-resident knowledge base, satisfying the Plan 9
philosophy of *network transparency*.

---

## Summary

| Scenario | Steps |
|----------|-------|
| Real Plan 9 | `mk install` in each `sys/src/` directory |
| plan9port (Linux/macOS) | Install plan9port → `mk install` |
| Dev Container / Codespaces | Just open the repo — everything is pre-installed |
| Windows | CMake build with sparse checkout (see CI workflow) |
| Distributed | Run `cogfs`, export with `exportfs`, import on clients |
