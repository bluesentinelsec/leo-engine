FROM debian:bookworm

# Install dependencies
RUN apt-get update -y && \
    apt-get install -y vim build-essential cmake git python3 python3-pip curl pkg-config zip

# Install emsdk (Emscripten)
RUN git clone https://github.com/emscripten-core/emsdk.git \
 && cd emsdk \
 && ./emsdk install latest \
 && ./emsdk activate latest

# Add Emscripten to the environment for all future RUN/CMD
ENV EMSDK=/emsdk
ENV PATH="/emsdk:/emsdk/upstream/emscripten:$PATH"
ENV EMSDK_NODE=/emsdk/node/12.18.1_64bit/bin/node
ENV EM_CONFIG=/emsdk/.emscripten
ENV EM_CACHE=/emsdk/.emscripten_cache

# Copy your game source into the image
COPY . /leo/
WORKDIR /leo

# Compile the game (adjust emcc flags as needed)
RUN emcmake cmake . -DCMAKE_BUILD_TYPE=Release -DLEO_BUILD_SHARED=OFF -DLEO_BUILD_TESTS=OFF
RUN cmake --build .

RUN ls -lsah

# Expose HTTP port
EXPOSE 8000

# Serve index.html and related files

CMD ["python3", "-m", "http.server", "8000"]
