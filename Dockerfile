# =============================================================================
# Stage 1: Сборка Geant4 из исходников (без GUI — headless batch-режим)
# =============================================================================
FROM ubuntu:22.04 AS geant4-builder

ARG GEANT4_VERSION=11.2.2
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    cmake \
    make \
    g++ \
    gcc \
    libexpat1-dev \
    zlib1g-dev \
    wget \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/geant4-src

# Скачиваем исходники Geant4
RUN wget -q https://gitlab.cern.ch/geant4/geant4/-/archive/v${GEANT4_VERSION}/geant4-v${GEANT4_VERSION}.tar.gz \
    && tar -xzf geant4-v${GEANT4_VERSION}.tar.gz \
    && rm geant4-v${GEANT4_VERSION}.tar.gz

WORKDIR /opt/geant4-build

# Конфигурируем без GUI/OpenGL (headless), с загрузкой данных
RUN cmake \
    -DCMAKE_INSTALL_PREFIX=/opt/geant4 \
    -DCMAKE_BUILD_TYPE=Release \
    -DGEANT4_INSTALL_DATA=ON \
    -DGEANT4_USE_QT=OFF \
    -DGEANT4_USE_OPENGL_X11=OFF \
    -DGEANT4_USE_MOTIF=OFF \
    -DGEANT4_USE_RAYTRACER_X11=OFF \
    -DGEANT4_BUILD_MULTITHREADED=ON \
    -DGEANT4_USE_SYSTEM_EXPAT=ON \
    /opt/geant4-src/geant4-v${GEANT4_VERSION} \
    && make -j$(nproc) \
    && make install

# =============================================================================
# Stage 2: Сборка симуляции
# =============================================================================
FROM ubuntu:22.04 AS sim-builder

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    cmake \
    make \
    g++ \
    gcc \
    libexpat1-dev \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

# Копируем установленный Geant4
COPY --from=geant4-builder /opt/geant4 /opt/geant4

WORKDIR /app

# Копируем исходники проекта
COPY CMakeLists.txt .
COPY sim.cc .
COPY include/ ./include/
COPY src/ ./src/
COPY macros/ ./macros/

# Патчим CMakeLists.txt: убираем требование ui_all vis_all (недоступны без GUI)
RUN sed -i 's/find_package(Geant4 REQUIRED ui_all vis_all)/find_package(Geant4 REQUIRED)/' CMakeLists.txt

WORKDIR /app/build

RUN cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DGeant4_DIR=/opt/geant4/lib/Geant4-*/cmake \
    .. \
    && make -j$(nproc)

# =============================================================================
# Stage 3: Runtime-образ
# =============================================================================
FROM ubuntu:22.04 AS runtime

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    libexpat1 \
    zlib1g \
    python3 \
    python3-pip \
    python3-numpy \
    python3-pandas \
    python3-matplotlib \
    python3-scipy \
    && rm -rf /var/lib/apt/lists/*

# Копируем Geant4 runtime (библиотеки + данные)
COPY --from=geant4-builder /opt/geant4 /opt/geant4

# Копируем собранный бинарник и макросы
COPY --from=sim-builder /app/build/sim /app/sim
COPY --from=sim-builder /app/build/*.mac /app/

# Копируем скрипт постобработки
COPY build/img.py /app/img.py

# Создаём папку для результатов
RUN mkdir -p /app/results/images

WORKDIR /app

# Переменные окружения Geant4
ENV G4ENSDFSTATEDATA=/opt/geant4/share/Geant4/data/G4ENSDFSTATE2.3
ENV G4LEVELGAMMADATA=/opt/geant4/share/Geant4/data/PhotonEvaporation5.7
ENV G4RADIOACTIVEDATA=/opt/geant4/share/Geant4/data/RadioactiveDecay5.6
ENV G4PARTICLEXSDATA=/opt/geant4/share/Geant4/data/G4PARTICLEXS4.0
ENV G4PIIDATA=/opt/geant4/share/Geant4/data/G4PII1.3
ENV G4REALSURFACEDATA=/opt/geant4/share/Geant4/data/RealSurface2.2
ENV G4SAIDXSDATA=/opt/geant4/share/Geant4/data/G4SAIDDATA2.0
ENV G4ABLADATA=/opt/geant4/share/Geant4/data/G4ABLA3.3
ENV G4INCLDATA=/opt/geant4/share/Geant4/data/G4INCL1.2
ENV G4LEDATA=/opt/geant4/share/Geant4/data/G4EMLOW8.5
ENV G4NEUTRONHPDATA=/opt/geant4/share/Geant4/data/G4NDL4.7
ENV LD_LIBRARY_PATH=/opt/geant4/lib:${LD_LIBRARY_PATH}

# Entrypoint: запуск симуляции с указанным макросом
# По умолчанию — run.mac
ENTRYPOINT ["/app/sim"]
CMD ["run.mac"]
