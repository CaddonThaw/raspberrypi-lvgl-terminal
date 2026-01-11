FROM arm64v8/ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    libopencv-dev \
    libiw-dev \
    git \
    wget \
    curl \
    && rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/WiringPi/WiringPi.git \
    && cd WiringPi \
    && ./build \
    && cd .. \
    && rm -rf WiringPi

WORKDIR /app

COPY . .

RUN make

CMD ["./demo"]