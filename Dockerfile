# Base image
FROM buildpack-deps:stable

# Install Venus SDK
RUN --mount=type=bind,source=./build/toolchain.sh,target=/tmp/toolchain.sh \
    cp /tmp/toolchain.sh . && \
    chmod +x ./toolchain.sh && \
    ./toolchain.sh -y && \
    rm ./toolchain.sh

# Install dependencies
RUN apt-get update && apt-get install -y libdbus-1-dev cmake lcov

WORKDIR /workspace