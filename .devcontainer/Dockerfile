# Plan9Cog Development Container
# Provides a complete environment for developing Plan 9 4th Edition with Plan9Cog extensions

FROM ubuntu:22.04

# Avoid prompts from apt
ENV DEBIAN_FRONTEND=noninteractive

# Install basic dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    git \
    curl \
    wget \
    vim \
    nano \
    less \
    man-db \
    sudo \
    ca-certificates \
    libx11-dev \
    libxt-dev \
    libxext-dev \
    libfontconfig1-dev \
    libfreetype6-dev \
    xvfb \
    x11-apps \
    # Additional tools for development
    gdb \
    valgrind \
    strace \
    ltrace \
    file \
    tree \
    jq \
    python3 \
    python3-pip \
    # Libraries needed for Plan 9 tools
    libc6-dev \
    gcc \
    g++ \
    make \
    flex \
    bison \
    && rm -rf /var/lib/apt/lists/*

# Install Plan 9 from User Space (plan9port)
# This provides Plan 9 development tools on Unix/Linux
RUN cd /usr/local && \
    git clone https://github.com/9fans/plan9port plan9 && \
    cd plan9 && \
    ./INSTALL && \
    # Clean up build artifacts to reduce image size
    find . -name '*.o' -delete && \
    find . -name '*.a' -delete

# Set up Plan 9 environment
ENV PLAN9=/usr/local/plan9
ENV PATH=$PLAN9/bin:$PATH

# Create a wrapper for mk that works with the repository structure
RUN echo '#!/bin/sh\n\
# Wrapper for Plan 9 mk to work with 4th Edition source tree\n\
exec $PLAN9/bin/mk "$@"' > /usr/local/bin/mk && \
    chmod +x /usr/local/bin/mk

# Install GitHub CLI for GitHub integration
RUN curl -fsSL https://cli.github.com/packages/githubcli-archive-keyring.gpg | \
    dd of=/usr/share/keyrings/githubcli-archive-keyring.gpg && \
    chmod go+r /usr/share/keyrings/githubcli-archive-keyring.gpg && \
    echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/githubcli-archive-keyring.gpg] https://cli.github.com/packages stable main" | \
    tee /etc/apt/sources.list.d/github-cli.list > /dev/null && \
    apt-get update && \
    apt-get install -y gh && \
    rm -rf /var/lib/apt/lists/*

# Create a non-root user for development
ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=$USER_UID

RUN groupadd --gid $USER_GID $USERNAME && \
    useradd --uid $USER_UID --gid $USER_GID -m $USERNAME && \
    echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME && \
    chmod 0440 /etc/sudoers.d/$USERNAME

# Set up Git for the container
RUN git config --system core.autocrlf false && \
    git config --system core.eol lf && \
    git config --system pull.rebase false

# Create workspace directory
RUN mkdir -p /workspace && chown $USERNAME:$USERNAME /workspace

# Set up shell environment for Plan 9 development
RUN echo '# Plan 9 Environment' >> /home/$USERNAME/.bashrc && \
    echo 'export PLAN9=/usr/local/plan9' >> /home/$USERNAME/.bashrc && \
    echo 'export PATH=$PLAN9/bin:$PATH' >> /home/$USERNAME/.bashrc && \
    echo 'export MANPATH=$PLAN9/man:$MANPATH' >> /home/$USERNAME/.bashrc && \
    echo '' >> /home/$USERNAME/.bashrc && \
    echo '# Aliases for Plan 9 development' >> /home/$USERNAME/.bashrc && \
    echo 'alias 9c="$PLAN9/bin/9c"' >> /home/$USERNAME/.bashrc && \
    echo 'alias 9l="$PLAN9/bin/9l"' >> /home/$USERNAME/.bashrc && \
    echo 'alias acme="$PLAN9/bin/acme"' >> /home/$USERNAME/.bashrc && \
    echo '' >> /home/$USERNAME/.bashrc && \
    echo '# Welcome message' >> /home/$USERNAME/.bashrc && \
    echo 'echo "Plan9Cog Development Environment"' >> /home/$USERNAME/.bashrc && \
    echo 'echo "Plan 9 tools available: 9c, 9l, mk, rc, acme"' >> /home/$USERNAME/.bashrc && \
    echo 'echo "PLAN9=$PLAN9"' >> /home/$USERNAME/.bashrc

# Switch to non-root user
USER $USERNAME
WORKDIR /workspace

# Set the default shell
ENV SHELL=/bin/bash

# Healthcheck for the container
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD which mk && which 9c || exit 1

# Default command
CMD ["/bin/bash"]
