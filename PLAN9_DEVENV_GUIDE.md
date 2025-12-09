# Plan 9 Development Environment: Best Practices and Recommendations

This document answers the question: **"What is the best approach with GitHub Actions and/or Codespaces when implementing a Plan 9 based environment?"**

## TL;DR - Recommended Approach

**Use a multi-layered strategy:**

1. ✅ **Dev Container (Primary)** - Best developer experience for Codespaces and local development
2. ✅ **GitHub Actions** - Automated CI/CD using the same container base
3. ✅ **Custom Docker Image** - Shared base for both dev and CI, optionally published to GHCR
4. ✅ **Prebuilds** - Significantly speeds up Codespace initialization

**Why this works best:** Provides consistency across all environments (local, Codespaces, CI) while giving developers the best experience and fastest iteration time.

## Detailed Analysis

### Option 1: Dev Container with plan9port (Recommended ⭐)

**Implementation:** `.devcontainer/Dockerfile` + `.devcontainer/devcontainer.json`

**What it is:**
- Docker container with Ubuntu base
- Plan 9 from User Space (plan9port) installed
- All Plan 9 development tools (mk, 9c, rc, acme)
- Git integration for version control
- VS Code extensions pre-configured

**Pros:**
- ✅ Best developer experience (Codespaces + local)
- ✅ Consistent environment for everyone
- ✅ Works with Git natively
- ✅ Fast iteration (rebuilds are quick)
- ✅ VS Code integration (IntelliSense, debugging)
- ✅ Can use same image for CI
- ✅ Supports prebuilds for faster Codespace startup

**Cons:**
- ⚠️ Not "real" Plan 9 (but plan9port is very close)
- ⚠️ Initial build takes 10-15 minutes
- ⚠️ Requires Docker (for local dev)

**When to use:**
- Primary development environment
- Team collaboration
- Need Git integration
- Want modern tools + Plan 9

**Example workflow:**
```bash
# In Codespace or dev container
cd sys/src/libatomspace
mk install
cd ../cmd/cogctl
mk install
./test-plan9cog.sh
```

### Option 2: GitHub Actions with Custom Image

**Implementation:** `.github/workflows/ci.yml` using same Dockerfile

**What it is:**
- GitHub Actions workflows
- Installs plan9port in CI
- Runs tests automatically
- Can use pre-built image from GHCR

**Pros:**
- ✅ Automated testing on every push
- ✅ Uses same environment as dev
- ✅ No manual CI configuration needed
- ✅ Free for public repos
- ✅ Can cache Docker layers

**Cons:**
- ⚠️ Not for interactive development
- ⚠️ Build time counts against CI minutes
- ⚠️ Limited to Linux runners

**When to use:**
- Automated testing
- Pull request validation
- Release builds
- Continuous integration

**Example workflow:**
```yaml
jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Setup Plan 9
        run: |
          cd /usr/local
          sudo git clone https://github.com/9fans/plan9port plan9
          cd plan9 && sudo ./INSTALL
      - name: Test
        run: ./test-plan9cog.sh
```

### Option 3: Custom Runner with Native Plan 9 (Not Recommended)

**Implementation:** Self-hosted GitHub Actions runner on Plan 9 system

**What it is:**
- Self-hosted runner on real Plan 9
- Direct access to native tools
- "Authentic" Plan 9 environment

**Pros:**
- ✅ Real Plan 9 (not emulation)
- ✅ Native performance
- ✅ Authentic development experience

**Cons:**
- ❌ Very difficult to set up
- ❌ Git integration is painful
- ❌ Limited hardware support
- ❌ Hard to maintain
- ❌ No Codespaces support
- ❌ Not portable
- ❌ Complex networking
- ❌ Steep learning curve

**When to use:**
- Research into Plan 9 itself
- Need absolute authenticity
- Have dedicated Plan 9 hardware
- Not practical for this project

### Option 4: GitHub App for IDE Integration (Unnecessary)

**Implementation:** Custom GitHub App with Plan 9 IDE

**What it is:**
- GitHub App that provides IDE
- Custom integration with Plan 9 tools
- Browser-based or desktop app

**Pros:**
- ✅ Could provide custom Plan 9 UI
- ✅ Tight GitHub integration

**Cons:**
- ❌ Massive development effort
- ❌ Dev containers already solve this
- ❌ Maintenance burden
- ❌ Reinventing the wheel
- ❌ Limited adoption
- ❌ Not needed when VS Code + containers work

**When to use:**
- Never for this project
- Only if building a commercial Plan 9 IDE

### Option 5: Custom 'runs-on' Image (Overkill)

**Implementation:** Custom GitHub Actions runner image

**What it is:**
- Custom Docker image for `runs-on`
- Pre-installed Plan 9 tools
- Published to container registry

**Pros:**
- ✅ Fast CI startup (pre-installed tools)
- ✅ Consistent CI environment
- ✅ Can be reused across repos

**Cons:**
- ⚠️ More complex setup initially
- ⚠️ Need to maintain image
- ⚠️ Still need dev container separately

**When to use:**
- Many repos need same environment
- CI builds take too long
- Want fastest CI possible
- Complement to dev container, not replacement

## Recommended Implementation Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Source Repository                         │
│                     (cogplan9)                               │
└───────────────┬─────────────────────────────────────────────┘
                │
        ┌───────┴────────┐
        │                │
        ▼                ▼
┌───────────────┐  ┌──────────────┐
│ Dev Container │  │ GitHub       │
│ Configuration │  │ Actions      │
│               │  │ Workflows    │
└───────┬───────┘  └──────┬───────┘
        │                 │
        │   ┌─────────────┼─────────────┐
        │   │             │             │
        ▼   ▼             ▼             ▼
    ┌──────────┐     ┌─────────┐  ┌─────────┐
    │Codespace │     │Local Dev│  │   CI    │
    │ (Cloud)  │     │Container│  │ Runner  │
    └──────────┘     └─────────┘  └─────────┘
         │                │             │
         └────────────────┴─────────────┘
                      │
              ┌───────▼────────┐
              │  Shared Base   │
              │ Docker Image   │
              │  (plan9port)   │
              └────────────────┘
```

All environments share the same base configuration, ensuring consistency.

## Implementation Steps

### Step 1: Create Dev Container Configuration

```
.devcontainer/
├── Dockerfile              # Container definition
├── devcontainer.json       # VS Code/Codespaces config
└── README.md              # Documentation
```

**Key components:**
- Ubuntu 22.04 base
- plan9port installation
- Development tools
- VS Code extensions
- Environment variables

### Step 2: Create GitHub Actions Workflows

```
.github/workflows/
├── ci.yml                 # Main CI workflow
└── codespaces.yml         # Prebuild workflow
```

**Key workflows:**
- Build and test on push
- Validate pull requests
- Build container images
- Create prebuilds for Codespaces

### Step 3: Add Documentation

```
DEVELOPMENT.md             # Complete dev guide
.devcontainer/README.md    # Container docs
```

**Include:**
- Quick start guide
- All environment options
- Troubleshooting
- Best practices

### Step 4: Optional - Publish Container Image

```yaml
# In .github/workflows/codespaces.yml
- name: Push to GHCR
  uses: docker/build-push-action@v5
  with:
    push: true
    tags: ghcr.io/cogpy/cogplan9-dev:latest
```

**Benefits:**
- Faster CI (no rebuild)
- Faster Codespace startup
- Can be reused by other repos

## Comparison Matrix

| Feature                 | Dev Container | GitHub Actions | Custom Runner | GitHub App |
|-------------------------|---------------|----------------|---------------|------------|
| Developer Experience    | ⭐⭐⭐⭐⭐    | ⭐⭐          | ⭐⭐⭐       | ⭐⭐⭐    |
| Setup Complexity        | ⭐⭐⭐⭐      | ⭐⭐⭐⭐⭐    | ⭐            | ⭐         |
| Maintenance Effort      | ⭐⭐⭐⭐      | ⭐⭐⭐⭐⭐    | ⭐⭐          | ⭐         |
| Git Integration         | ⭐⭐⭐⭐⭐    | ⭐⭐⭐⭐⭐    | ⭐⭐          | ⭐⭐⭐⭐   |
| Codespaces Support      | ⭐⭐⭐⭐⭐    | N/A            | N/A           | ⭐⭐⭐⭐   |
| CI/CD Integration       | ⭐⭐⭐⭐⭐    | ⭐⭐⭐⭐⭐    | ⭐⭐⭐⭐⭐    | ⭐⭐⭐    |
| Cost                    | Free/Low      | Free           | Medium        | High       |
| Portability             | ⭐⭐⭐⭐⭐    | ⭐⭐⭐⭐⭐    | ⭐            | ⭐⭐⭐    |

## Real-World Example: This Repository

The cogplan9 repository now includes:

### 1. Dev Container (`.devcontainer/`)
- Dockerfile with plan9port
- devcontainer.json with VS Code config
- Full Plan 9 toolchain
- Works in Codespaces and locally

### 2. GitHub Actions (`.github/workflows/`)
- `ci.yml` - Runs tests on every push
- `codespaces.yml` - Builds prebuild images
- Uses same base as dev container

### 3. Documentation (`DEVELOPMENT.md`)
- Complete setup guide
- All options explained
- Troubleshooting tips

### To Use:

**Codespaces (Easiest):**
```
1. Click "Code" → "Codespaces" → "Create codespace"
2. Wait ~5 minutes
3. Start coding!
```

**Local Dev Container:**
```bash
1. Install Docker + VS Code + Dev Containers extension
2. Open repo in VS Code
3. Click "Reopen in Container"
4. Wait ~10 minutes
5. Start coding!
```

**GitHub Actions (Automatic):**
```
1. Push code
2. Actions run automatically
3. Check test results
```

## Best Practices

### DO:
- ✅ Use dev containers for consistency
- ✅ Share base image between dev and CI
- ✅ Use prebuilds for Codespaces
- ✅ Document all environment options
- ✅ Test locally before pushing
- ✅ Use plan9port for practicality

### DON'T:
- ❌ Try to run native Plan 9 in CI (too hard)
- ❌ Create custom GitHub App (unnecessary)
- ❌ Skip documentation (others need to onboard)
- ❌ Use different environments for dev and CI (inconsistency)
- ❌ Forget to test the container itself

## Conclusion

**The best approach is a dev container with plan9port**, used by:
1. **Codespaces** for cloud development
2. **Local VS Code** with Dev Containers extension
3. **GitHub Actions** for CI/CD
4. **Optional prebuilds** for faster startup

This provides:
- Consistent environment everywhere
- Best developer experience
- Git integration that actually works
- Practical Plan 9 development
- Minimal maintenance
- Fast iteration
- Easy onboarding

Native Plan 9 would be ideal in theory but impractical in reality. plan9port provides 95% of Plan 9 with 100% more practicality.

**Start here:**
1. Open in Codespace
2. Run `./test-plan9cog.sh`
3. Start coding!

That's it. Everything else is automatic. 🚀

---

For detailed instructions, see [DEVELOPMENT.md](DEVELOPMENT.md).
