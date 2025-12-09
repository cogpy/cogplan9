# Plan9Cog Development Environment Guide

This guide explains how to set up and use the Plan9Cog development environment using GitHub Codespaces, Dev Containers, or local development.

## Table of Contents

- [Quick Start](#quick-start)
- [Environment Options](#environment-options)
- [Using GitHub Codespaces](#using-github-codespaces)
- [Using Dev Containers Locally](#using-dev-containers-locally)
- [Using GitHub Actions](#using-github-actions)
- [Manual Setup](#manual-setup)
- [Development Workflow](#development-workflow)
- [Troubleshooting](#troubleshooting)

## Quick Start

The **fastest way** to start developing Plan9Cog:

1. Open this repository on GitHub
2. Click the green "Code" button
3. Select "Codespaces" tab
4. Click "Create codespace on main"
5. Wait for the environment to build (~5-10 minutes first time)
6. Start coding!

## Environment Options

We provide **four ways** to develop Plan9Cog, each with different trade-offs:

### 1. GitHub Codespaces (Recommended) ⭐

**Best for:** Quick start, cloud development, consistent environment

**Pros:**
- Zero local setup required
- Pre-configured with all tools
- Accessible from anywhere
- Fast with prebuilds

**Cons:**
- Requires internet connection
- Limited by GitHub quotas
- Monthly usage limits (free tier)

**How to use:** See [Using GitHub Codespaces](#using-github-codespaces)

### 2. Dev Containers (VS Code)

**Best for:** Local development, offline work, full control

**Pros:**
- Works offline
- Uses local resources
- Full control over environment
- Consistent with Codespaces

**Cons:**
- Requires Docker installed
- Initial setup time
- Uses local disk space

**How to use:** See [Using Dev Containers Locally](#using-dev-containers-locally)

### 3. GitHub Actions

**Best for:** CI/CD, automated testing, pull request checks

**Pros:**
- Automatic testing on push
- No local resources needed
- Consistent test environment
- Free for public repos

**Cons:**
- Only for automated tasks
- Not for interactive development

**How to use:** See [Using GitHub Actions](#using-github-actions)

### 4. Manual Setup

**Best for:** Native Plan 9 systems, custom configurations

**Pros:**
- No containers needed
- Maximum performance
- Custom configuration

**Cons:**
- Complex setup
- Platform-specific issues
- Hard to reproduce

**How to use:** See [Manual Setup](#manual-setup)

## Using GitHub Codespaces

### First Time Setup

1. **Create a Codespace:**
   ```
   GitHub Repository → Code → Codespaces → Create codespace on main
   ```

2. **Wait for build:**
   - First build takes ~5-10 minutes (installs Plan 9 tools)
   - Subsequent starts are faster (~30 seconds) with prebuilds

3. **Verify environment:**
   ```bash
   # In the Codespace terminal
   echo $PLAN9              # Should show /usr/local/plan9
   which mk                 # Should find mk
   which 9c                 # Should find 9c compiler
   ./test-plan9cog.sh       # Run integration tests
   ```

### Development in Codespaces

```bash
# Navigate to Plan9Cog sources
cd sys/src/libatomspace

# Build a library
mk install

# Run tests
cd ../../..
./test-plan9cog.sh

# Use Plan 9 tools
9c myfile.c              # Compile with Plan 9 C compiler
mk                       # Build with mk
rc                       # Use Plan 9 shell
acme                     # Launch Acme editor (GUI)
```

### Codespace Configuration

The Codespace is configured via `.devcontainer/devcontainer.json`:

- **Base image:** Ubuntu 22.04 with Plan 9 from User Space
- **Plan 9 tools:** mk, 9c, 9l, rc, acme, and more
- **Extensions:** C/C++ tools, GitHub Copilot, Git tools
- **Environment:** PLAN9=/usr/local/plan9

### Prebuild Configuration

Prebuilds are automatically created on pushes to `main`. This speeds up Codespace creation significantly.

To trigger a prebuild manually:
```
GitHub Actions → Codespaces Prebuild → Run workflow
```

## Using Dev Containers Locally

### Prerequisites

- [Docker Desktop](https://www.docker.com/products/docker-desktop) or Docker Engine
- [Visual Studio Code](https://code.visualstudio.com/)
- [Dev Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)

### Setup

1. **Clone the repository:**
   ```bash
   git clone https://github.com/cogpy/cogplan9.git
   cd cogplan9
   ```

2. **Open in VS Code:**
   ```bash
   code .
   ```

3. **Open in container:**
   - VS Code will detect `.devcontainer/`
   - Click "Reopen in Container" notification
   - Or: `Cmd/Ctrl+Shift+P` → "Dev Containers: Reopen in Container"

4. **Wait for build:**
   - First build: ~10-15 minutes (downloads and builds Plan 9 tools)
   - Subsequent builds: Much faster with Docker cache

5. **Start developing:**
   - Terminal will have Plan 9 tools available
   - All extensions pre-installed

### Local Development Tips

```bash
# Build the dev container image manually
docker build -t cogplan9-dev .devcontainer/

# Run container manually
docker run -it --rm -v $(pwd):/workspace cogplan9-dev

# Clean rebuild (if issues occur)
# In VS Code: Cmd/Ctrl+Shift+P → "Dev Containers: Rebuild Container"
```

## Using GitHub Actions

### Automatic Testing

Every push and pull request automatically:

1. Installs Plan 9 from User Space
2. Runs integration tests (`test-plan9cog.sh`)
3. Validates file structure
4. Checks mkfiles
5. Tests syntax of C files

### Manual Workflow Trigger

```
GitHub Actions → Plan9Cog CI → Run workflow
```

### Using Actions for Custom Builds

Create a custom workflow in `.github/workflows/`:

```yaml
name: Custom Build

on:
  push:
    branches: [ my-feature ]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Setup Plan 9
        run: |
          cd /usr/local
          sudo git clone https://github.com/9fans/plan9port plan9
          cd plan9
          sudo ./INSTALL
          echo "PLAN9=/usr/local/plan9" >> $GITHUB_ENV
          echo "/usr/local/plan9/bin" >> $GITHUB_PATH
      
      - name: Build
        run: |
          # Your build commands here
          ./test-plan9cog.sh
```

## Manual Setup

### Install Plan 9 from User Space

For Linux/macOS:

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install build-essential libx11-dev libxt-dev \
  libxext-dev libfontconfig1-dev git

# Clone and install plan9port
cd /usr/local
sudo git clone https://github.com/9fans/plan9port plan9
cd plan9
sudo ./INSTALL

# Set up environment
echo 'export PLAN9=/usr/local/plan9' >> ~/.bashrc
echo 'export PATH=$PLAN9/bin:$PATH' >> ~/.bashrc
source ~/.bashrc
```

For macOS with Homebrew:

```bash
brew install plan9port
export PLAN9=/usr/local/plan9
export PATH=$PLAN9/bin:$PATH
```

### Verify Installation

```bash
which mk
which 9c
9c --help
```

### Clone and Test

```bash
git clone https://github.com/cogpy/cogplan9.git
cd cogplan9
./test-plan9cog.sh
```

## Development Workflow

### Building Plan9Cog Components

```bash
# Build all libraries
cd sys/src
for lib in libatomspace libpln libplan9cog; do
  cd $lib
  mk install
  cd ..
done

# Build commands
cd cmd
for cmd in cogctl cogfs cogdemo; do
  cd $cmd
  mk install
  cd ..
done
```

### Running Tests

```bash
# Run integration tests
./test-plan9cog.sh

# Or with Plan 9 rc shell
rc test-plan9cog.rc
```

### Code Style

Plan9Cog follows Plan 9 conventions:

- **Tabs, not spaces** (8-character tabs)
- **K&R brace style**
- **Lowercase function names**
- **CamelCase types**

Example:

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

### Using Plan 9 Tools

```bash
# Compile C code
9c myfile.c

# Link
9l -o myprogram myfile.o

# Build with mk
mk

# Use Plan 9 shell
rc

# Edit with Acme (GUI required)
acme myfile.c
```

## Troubleshooting

### Codespace Issues

**Problem:** Codespace build fails

**Solution:**
- Check GitHub Actions logs
- Try rebuilding: Settings → Rebuild Container
- Delete and create new Codespace

**Problem:** Plan 9 tools not found

**Solution:**
```bash
# Check environment
echo $PLAN9
echo $PATH

# Re-source profile
source ~/.bashrc

# Verify installation
ls -la /usr/local/plan9/bin/
```

### Dev Container Issues

**Problem:** Container build fails

**Solution:**
```bash
# Clean Docker cache
docker system prune -a

# Rebuild without cache
# VS Code: Cmd/Ctrl+Shift+P → "Dev Containers: Rebuild Container Without Cache"
```

**Problem:** X11 forwarding issues (for Acme)

**Solution:**
- On Linux: Install `xvfb` and use `xvfb-run acme`
- On macOS: Install XQuartz
- On Windows: Install VcXsrv

### Build Issues

**Problem:** `mk: command not found`

**Solution:**
```bash
# Ensure Plan 9 is in PATH
export PLAN9=/usr/local/plan9
export PATH=$PLAN9/bin:$PATH

# Verify
which mk
```

**Problem:** `cannot find -latomspace`

**Solution:**
```bash
# Build libraries in order
cd sys/src/libatomspace && mk install
cd ../libpln && mk install
cd ../libplan9cog && mk install
```

**Problem:** Header files not found

**Solution:**
```bash
# Check if headers exist
ls -la sys/include/plan9cog.h
ls -la sys/include/plan9cog/

# If using plan9port, headers should be in:
# /usr/local/plan9/include/
```

### GitHub Actions Issues

**Problem:** Action fails with "mk not found"

**Solution:**
- Check that Plan 9 setup step completed
- Verify PLAN9 environment variable is set
- Ensure PATH includes $PLAN9/bin

**Problem:** Tests fail in CI but pass locally

**Solution:**
- Check file permissions: `chmod +x test-plan9cog.sh`
- Verify line endings: should be LF, not CRLF
- Check for platform-specific assumptions

## Additional Resources

### Documentation

- [Plan9Cog Guide](PLAN9COG_GUIDE.md) - Complete Plan9Cog API reference
- [Architecture](ARCHITECTURE.md) - System architecture details
- [Plan9Cog README](PLAN9COG_README.md) - Quick start guide

### Plan 9 Resources

- [Plan 9 from User Space](https://9fans.github.io/plan9port/)
- [Plan 9 Documentation](http://doc.cat-v.org/plan_9/)
- [Plan 9 Wiki](https://9p.io/wiki/plan9/)

### Support

- Open an issue on GitHub
- Check existing issues and discussions
- Consult Plan 9 and Plan9Cog documentation

## Summary

The **recommended approach** for Plan9Cog development:

1. **For quick start:** Use GitHub Codespaces
2. **For regular development:** Use Dev Containers with VS Code
3. **For CI/CD:** Use GitHub Actions (automatic)
4. **For native Plan 9:** Use manual setup

All approaches use the same base configuration (`.devcontainer/Dockerfile`) ensuring consistency across development, testing, and deployment.

Happy coding with Plan9Cog! 🚀
