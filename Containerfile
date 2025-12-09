FROM debian:13
MAINTAINER Kristian Larsson <kristian@spritelink.net>

COPY dist/ /usr/lib/acton/

RUN cd /usr/bin \
 && ln -s ../lib/acton/bin/actonc \
 && ln -s ../lib/acton/bin/actondb \
 && ln -s ../lib/acton/bin/runacton

ENTRYPOINT ["/usr/bin/actonc"]
