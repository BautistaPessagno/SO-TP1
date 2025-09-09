# Base image with common tooling for ITBA SO
FROM agodio/itba-so:2.0

ENV DEBIAN_FRONTEND=noninteractive

# Install build essentials and ncurses (both wide and non-wide variants)
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
       build-essential \
       make \
       pkg-config \
       libncurses5-dev \
       libncursesw5-dev \
    && rm -rf /var/lib/apt/lists/*

# Workdir for mounting the source tree
WORKDIR /work

# Default to an interactive shell so you can run make inside the container
CMD ["bash"]



