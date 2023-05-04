FROM debian:buster as base

RUN set -ex; \
    apt-get update; \
    apt-get install -y libgmp-dev; \
    apt-get clean; \
    rm -rf /var/lib/apt/lists/*

FROM base as builder

RUN set -ex; \
    apt-get update; \
    apt-get install -y g++ cmake make

RUN mkdir -p /usr/src
COPY . /usr/src/glpsol-trace

RUN set -ex; \
    cd /usr/src/glpsol-trace; \
    cmake . ; make; make install

FROM base AS runtime

COPY --from=builder /usr/local/bin /usr/local/bin

WORKDIR /home
ENTRYPOINT [ "/bin/bash", "-l", "-c" ]
