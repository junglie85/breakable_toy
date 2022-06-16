# Breakable Toy

## Development

To clone:

```command_line
git clone --recurse-submodules -j8 https://github.com/junglie85/breakable_toy.git
```

To build (debug) and run (Windows):

```command_line
mkdir build-debug
cd build-debug
cmake -GNinja -DCMAKE_BUILD_TYPE=Debug ..\breakable_toy
cmake --build .
.\src\toy.exe
```

To build (debug) and run (Linux/macOS):

```command_line
mkdir build-debug
cd build-debug
cmake -GNinja -DCMAKE_BUILD_TYPE=Debug ../breakable_toy
cmake --build .
./src/toy
```
