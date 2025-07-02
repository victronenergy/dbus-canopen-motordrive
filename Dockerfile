# Base image
FROM buildpack-deps:stable

# Install Venus SDK
RUN wget https://updates.victronenergy.com/feeds/venus/release/sdk/venus-scarthgap-x86_64-arm-cortexa8hf-neon-toolchain-v3.62.sh \
    && chmod +x venus-scarthgap-x86_64-arm-cortexa8hf-neon-toolchain-v3.62.sh \
    && ./venus-scarthgap-x86_64-arm-cortexa8hf-neon-toolchain-v3.62.sh -y

# Install dependencies
RUN apt-get update && apt-get install -y \
    apt-get install libdbus-1-dev

WORKDIR /workspace