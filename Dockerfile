# =====================================================================
# Stage 1: The "Builder" Stage
# =====================================================================
# Use a specific Ubuntu LTS version for reproducibility
FROM ubuntu:22.04 AS builder

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# --- Install Up-to-Date CMake ---
# The default Ubuntu 22.04 apt repository has an old version of CMake.
# We will add the official Kitware repository to get a modern version.
RUN apt-get update && \
    apt-get install -y \
    apt-transport-https \
    ca-certificates \
    gnupg \
    software-properties-common \
    wget && \
    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc | gpg --dearmor -o /etc/apt/trusted.gpg.d/kitware.gpg && \
    apt-add-repository 'deb https://apt.kitware.com/ubuntu/ jammy main' && \
    apt-get update && \
    apt-get install -y cmake

# Install other essential build tools and git
RUN apt-get install -y build-essential git curl zip unzip tar pkg-config

# --- VCPKG Setup ---
# Clone vcpkg and set it up
WORKDIR /opt
RUN git clone https://github.com/microsoft/vcpkg.git && \
    ./vcpkg/bootstrap-vcpkg.sh

# Set environment variable for the vcpkg root
ENV VCPKG_ROOT=/opt/vcpkg

# --- Application Build ---
# Set the working directory for the application source
WORKDIR /app

# Copy dependency manifests first to leverage Docker layer caching.
COPY vcpkg.json .

# Copy the rest of the source code
COPY CMakeLists.txt .
COPY src/ ./src/
COPY test/ ./test/

# Configure the project with CMake. This will now use the modern version.
# vcpkg will automatically read vcpkg.json and install dependencies.
RUN cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake

# Build the project using all available CPU cores
RUN cmake --build build --config Release --parallel $(nproc)

# =====================================================================
# Stage 2: The "Final/Runner" Stage
# =====================================================================
# Start from a clean base image
FROM ubuntu:22.04

# Install only essential runtime dependencies.
RUN apt-get update && \
    apt-get install -y ca-certificates && \
    rm -rf /var/lib/apt/lists/*

# Set the working directory
WORKDIR /app

# === FIX IS HERE ===
# Copy the compiled application executable from the correct location in the builder stage.
# The build log shows the output is /app/build/Main, not /app/build/bin/Main.
COPY --from=builder /app/build/Main .

# Copy the required shared libraries (.so files) from vcpkg in the builder stage
COPY --from=builder /app/build/vcpkg_installed/x64-linux/lib/*.so* /usr/local/lib/

# Update the linker cache to find the newly added shared libraries
RUN ldconfig

# Set the command to run the application
ENTRYPOINT ["./Main"]
