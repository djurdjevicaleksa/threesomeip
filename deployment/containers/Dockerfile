FROM debian:12-slim

ARG S6_OVERLAY_VERSION=3.2.3.2

ADD https://github.com/just-containers/s6-overlay/releases/download/v${S6_OVERLAY_VERSION}/s6-overlay-noarch.tar.xz /tmp
ADD https://github.com/just-containers/s6-overlay/releases/download/v${S6_OVERLAY_VERSION}/s6-overlay-x86_64.tar.xz /tmp

RUN apt update && apt install -y --no-install-recommends        \
    xz-utils nano file bash                                     \
    && tar -C / -Jxpf /tmp/s6-overlay-noarch.tar.xz             \
    && tar -C / -Jxpf /tmp/s6-overlay-x86_64.tar.xz             \
    && rm /tmp/*.tar.xz                                         \
    && echo 'export PS1="\u@\h:\w\$ "' >> /root/.bashrc         \
    && echo 'export TERM=xterm-256color' >> /root/.bashrc       \
    && echo 'source /root/.bashrc' >> /root/.bash_profile

ENTRYPOINT ["/init"]