ARG builder_image
ARG builder_image_tag
FROM ${builder_image}:${builder_image_tag} AS builder

WORKDIR /jrtc/build
RUN rm -rf /jrtc/build/*
RUN cmake .. && make jbpf_io jrtc_router_stream_id jrtc
ENV PATH="$PATH:/jrtc/out/bin"

ENTRYPOINT [ "/jrtc/deploy/entrypoint.sh", "jrtc-ctl" ]
