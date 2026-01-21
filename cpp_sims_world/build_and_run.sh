#!/bin/bash

# Crea la cartella di build se non esiste
if [ ! -d "build" ]; then
    mkdir build
fi

cd build

# Esegui cmake solo se necessario (se non c'è il la cache)
# Questo previene la riconfigurazione inutile
if [ ! -f "CMakeCache.txt" ]; then
    echo "Configurazione iniziale CMake..."
    cmake ..
fi

# Compila usando tutti i core disponibili per velocizzare (-j)
# Make è incrementale: se Raylib è già compilato, non lo rifarà.
make -j$(sysctl -n hw.ncpu)

# Esegui l'eseguibile se la compilazione ha successo
if [ $? -eq 0 ]; then
    echo "Build successful! Running simulation..."
    ./NationSimulator
else
    echo "Build failed."
fi
