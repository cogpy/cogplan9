# Plan9Cog Dev Container

This directory contains the development container configuration for Plan9Cog, providing a complete Plan 9 development environment.

## What's Inside

### Dockerfile

The Dockerfile creates a container with:

- **Ubuntu 22.04** base image
- **Plan 9 from User Space (plan9port)** - Complete Plan 9 toolchain for Unix/Linux
- **Development tools:** gcc, make, git, gdb, valgrind, etc.
- **GitHub CLI** for GitHub integration
- **Non-root user** (`vscode`) for secure development

### devcontainer.json

Configuration for VS Code Dev Containers and GitHub Codespaces:

- Container build settings
- VS Code extensions for C/C++ development
- Plan 9 environment variables
- Terminal configuration (bash and rc shell)
- Editor settings (tabs, Plan 9 style)

## Quick Start

### GitHub Codespaces

1. Click "Code" → "Codespaces" → "Create codespace on main"
2. Wait for build (~5-10 minutes first time)
3. Start developing!

### Local Dev Containers

1. Install Docker and VS Code with Dev Containers extension
2. Open repository in VS Code
3. Click "Reopen in Container"
4. Wait for build (~10-15 minutes first time)
5. Start developing!

## What You Get

After the container builds, you'll have:

### Plan 9 Tools

- `mk` - Plan 9 build system
- `9c` - Plan 9 C compiler
- `9l` - Plan 9 linker
- `rc` - Plan 9 shell
- `acme` - Plan 9 text editor
- All other plan9port utilities

### Environment Variables

```bash
PLAN9=/usr/local/plan9
PATH=$PLAN9/bin:$PATH
```

### Shells Available

- `bash` (default) - Standard Linux shell with Plan 9 tools in PATH
- `rc` - Plan 9 shell for authentic Plan 9 experience

### VS Code Extensions

- C/C++ IntelliSense and debugging
- GitHub Copilot (if available)
- Git integration tools
- Markdown preview
- Spell checker

## Directory Structure

```
.devcontainer/
├── Dockerfile              # Container image definition
├── devcontainer.json       # VS Code/Codespaces configuration
└── README.md              # This file
```

## Customization

### Adding More Tools

Edit `Dockerfile` to add packages:

```dockerfile
RUN apt-get update && apt-get install -y \
    your-package-here \
    && rm -rf /var/lib/apt/lists/*
```

### Changing VS Code Settings

Edit `devcontainer.json` under `customizations.vscode.settings`:

```json
"customizations": {
  "vscode": {
    "settings": {
      "your.setting": "value"
    }
  }
}
```

### Adding Extensions

Edit `devcontainer.json` under `customizations.vscode.extensions`:

```json
"extensions": [
  "publisher.extension-name"
]
```

## Building Locally

Test the container locally:

```bash
# Build
docker build -t cogplan9-dev .

# Run
docker run -it --rm -v $(pwd)/..:/workspace cogplan9-dev

# Test Plan 9 tools
docker run --rm cogplan9-dev mk --version
```

## Architecture

The container provides a **hybrid environment**:

- **Linux base:** Ubuntu 22.04 for compatibility and package availability
- **Plan 9 tools:** Complete plan9port installation for Plan 9 development
- **Git integration:** Native Git for version control
- **Build tools:** Both modern (gcc, make) and Plan 9 (9c, mk) tools

This approach allows:
- Developing Plan 9 software on modern systems
- Using Git for version control
- Running Plan 9 programs in user space
- Building Plan9Cog components with mk

## Plan 9 from User Space

**plan9port** is a port of Plan 9 to Unix-like systems. It provides:

- Plan 9 C compiler (`9c`)
- Plan 9 build system (`mk`)
- Plan 9 shell (`rc`)
- Plan 9 libraries (lib9, libbio, etc.)
- Plan 9 applications (acme, sam, etc.)
- 9P file system protocol tools

While not identical to native Plan 9, plan9port provides an excellent development environment for Plan 9 software.

## Why This Approach?

### Why Dev Containers?

✅ **Consistency:** Same environment for everyone
✅ **Reproducibility:** Container = exact environment
✅ **Isolation:** No conflicts with host system
✅ **Portability:** Works on Linux, Mac, Windows
✅ **Quick setup:** No manual installation

### Why plan9port?

✅ **Availability:** Runs on modern systems
✅ **Compatibility:** Close to original Plan 9
✅ **Active development:** Well-maintained
✅ **Git integration:** Works with modern tools
✅ **Complete toolchain:** Everything needed

### Why Not Native Plan 9?

Native Plan 9 would be ideal but:
- Hard to set up in containers
- Limited hardware support
- Difficult Git integration
- Steep learning curve
- Not practical for CI/CD

plan9port provides 95% of Plan 9 with 100% more practicality.

## Troubleshooting

### Container Won't Build

```bash
# Clean Docker cache
docker system prune -a

# Rebuild from scratch
docker build --no-cache -t cogplan9-dev .
```

### Plan 9 Tools Not Found

```bash
# Check environment
echo $PLAN9          # Should be /usr/local/plan9
echo $PATH           # Should include $PLAN9/bin

# Source profile
source ~/.bashrc

# Verify installation
ls -la $PLAN9/bin/
```

### Permission Issues

The container runs as user `vscode` (UID 1000) to avoid permission issues with mounted volumes.

If you see permission errors:
```bash
# Fix ownership (in container)
sudo chown -R vscode:vscode /workspace
```

### X11/GUI Issues

For GUI apps like `acme`:

On Linux:
```bash
# Install xvfb
sudo apt-get install xvfb

# Run with xvfb
xvfb-run acme
```

On Mac/Windows:
- Install X server (XQuartz/VcXsrv)
- Configure X11 forwarding

## Performance

### Build Times

- **First build:** ~10-15 minutes (downloads, compiles plan9port)
- **Rebuild:** ~2-5 minutes (uses Docker cache)
- **With prebuild:** ~30 seconds (Codespaces only)

### Image Size

- **Final image:** ~1.5-2 GB
- **plan9port:** ~800 MB
- **System tools:** ~700 MB

### Optimization

The Dockerfile includes:
- Multi-stage build potential
- Cleanup of build artifacts
- Removal of package caches
- Efficient layer ordering

## CI/CD Integration

This container is also used in GitHub Actions (`.github/workflows/ci.yml`):

```yaml
- name: Build dev container
  uses: docker/build-push-action@v5
  with:
    context: .devcontainer
    file: .devcontainer/Dockerfile
```

This ensures CI uses the same environment as development.

## Further Reading

- [Development Guide](../DEVELOPMENT.md) - Complete development setup guide
- [Plan9Cog Guide](../PLAN9COG_GUIDE.md) - Plan9Cog API documentation
- [plan9port](https://9fans.github.io/plan9port/) - Plan 9 from User Space
- [Dev Containers](https://containers.dev/) - Development Containers specification

## Contributing

To improve this container:

1. Edit `Dockerfile` or `devcontainer.json`
2. Test locally: `docker build -t test .`
3. Test in VS Code: "Rebuild Container"
4. Submit PR with changes

Please ensure:
- Container builds successfully
- Plan 9 tools work: `mk`, `9c`, etc.
- Tests pass: `./test-plan9cog.sh`

---

**Plan9Cog Dev Container**: Where modern development meets Plan 9 elegance! 🚀
