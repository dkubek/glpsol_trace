FROM debian:buster as base

RUN set -ex; \
    apt-get update; \
    apt-get install -y libgmp-dev 

FROM base as builder

RUN set -ex; \
    apt-get install -y g++ cmake make; \
    mkdir -p /usr/src

COPY . /usr/src/mcfglpk

RUN set -ex; \
    cd /usr/src/mcfglpk; \
    cmake .; make; make install

FROM base AS runtime

COPY --from=builder /usr/local/bin /usr/local/bin

ENTRYPOINT ["mcfglpk"]
