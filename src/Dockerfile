# Dockerfile para ChompChamps (TP1 SO)
# Imagen base oficial de la cátedra
FROM agodio/itba-so-multi-platform:3.0

# Instala ncurses y utilidades de compilación (en caso de faltar)
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
       libncurses-dev \
       ncurses-term \
       pkg-config \
       build-essential \
       ca-certificates \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Directorio de trabajo (se recomienda montar el proyecto en /root)
WORKDIR /root

# Entrar en shell interactiva por defecto
CMD ["/bin/bash"]


