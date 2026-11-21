# CMake Build für RethinkDB

Dieses Verzeichnis enthält jetzt ein CMake-basiertes Build-System mit Conan für Dependency-Management.

## Voraussetzungen

- CMake >= 3.15
- Conan >= 2.0
- C++17-kompatibler Compiler (GCC >= 4.7.4 oder Clang)
- Python 3

## Installation der Build-Tools

### macOS

```bash
brew install cmake conan
```

### Linux (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install cmake python3 python3-pip
pip3 install conan
```

## Build-Anleitung

### 1. Conan Profile konfigurieren (einmalig)

```bash
conan profile detect --force
```

### 2. Dependencies installieren und Projekt bauen

#### Option A: Mit conanfile.txt (einfacher)

```bash
# Build-Verzeichnis erstellen
mkdir -p build && cd build

# Conan-Dependencies installieren
conan install .. --output-folder=. --build=missing

# CMake konfigurieren
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release

# Bauen
cmake --build . -j$(nproc)
```

#### Option B: Mit conanfile.py (flexibler)

```bash
# Build-Verzeichnis erstellen
mkdir -p build && cd build

# Conan-Dependencies installieren mit Options
conan install .. --output-folder=. --build=missing \
    -o with_jemalloc=True \
    -o with_tests=False

# CMake konfigurieren
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release

# Bauen
cmake --build . -j$(nproc)
```

### 3. Installation (optional)

```bash
sudo cmake --install .
```

## Build-Optionen

### CMake-Optionen

- `USE_CCACHE`: Verwende ccache für schnellere Builds (Standard: ON)
- `BUILD_TESTING`: Baue Tests (Standard: OFF)
- `USE_JEMALLOC`: Verwende jemalloc Allocator (Standard: ON)
- `STATIC_LINK`: Statisches Linken von Dependencies (Standard: OFF)

Beispiel:

```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=ON \
    -DUSE_JEMALLOC=ON
```

### Conan-Optionen (nur mit conanfile.py)

- `with_jemalloc`: Füge jemalloc hinzu (Standard: True)
- `with_tests`: Füge GTest für Tests hinzu (Standard: False)

Beispiel:

```bash
conan install .. --output-folder=. --build=missing \
    -o with_jemalloc=True \
    -o with_tests=True
```

## Build-Typen

- `Release`: Optimiert für Performance (-O3)
- `Debug`: Optimiert für Debugging (-g -O0)
- `RelWithDebInfo`: Release mit Debug-Informationen
- `MinSizeRel`: Optimiert für kleinere Binärgröße

## Troubleshooting

### Conan-Cache löschen

```bash
conan remove "*" -c
```

### Build-Verzeichnis neu erstellen

```bash
rm -rf build
mkdir build && cd build
```

### Dependencies manuell bauen

```bash
conan install .. --output-folder=. --build=missing --build=protobuf --build=openssl
```

## Unterschiede zum Original-Build-System

Das ursprüngliche RethinkDB Build-System verwendet ein komplexes Makefile-Setup mit einem `configure`-Script. Dieses CMake-Setup bietet:

- **Moderne Build-Tools**: CMake ist weit verbreitet und besser in IDEs integriert
- **Automatisches Dependency-Management**: Conan lädt und baut alle Dependencies automatisch
- **Plattform-Übergreifend**: Einfacher zu portieren auf verschiedene Systeme
- **Schnellere Builds**: Bessere Parallel-Compilation und ccache-Integration

## Nächste Schritte

Das CMake-Setup ist eine Basis-Implementierung. Um vollständig funktionsfähig zu sein, muss noch:

1. Die `src/CMakeLists.txt` erstellt werden, die alle Source-Dateien definiert
2. Protobuf-Code-Generierung integriert werden
3. Version-Generierung aus Git-Tags implementiert werden
4. Tests integriert werden (falls `BUILD_TESTING=ON`)

## Vergleich: Altes vs. Neues Build-System

### Altes System (Make)

```bash
./configure
make -j8
sudo make install
```

### Neues System (CMake + Conan)

```bash
mkdir build && cd build
conan install .. --output-folder=. --build=missing
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build . -j8
sudo cmake --install .
```

Das neue System ist etwas ausführlicher, bietet aber deutlich bessere Dependency-Verwaltung und IDE-Integration.
