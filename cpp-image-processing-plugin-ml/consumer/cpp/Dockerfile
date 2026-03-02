FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    zlib1g-dev \
    libcfitsio-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY CMakeLists.txt .
COPY src/ src/

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j$(nproc)

# --- Runtime stage ---
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libstdc++6 \
    zlib1g \
    libcfitsio9 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /app/build/custom-image-processing /usr/local/bin/custom-image-processing

EXPOSE 8099
CMD ["custom-image-processing"]
