# Breakable Toy

Initial Vulkan work based on [Little Vulkan Engine](https://github.com/blurrypiano/littleVulkanEngine) from
[Brendan Galea's YouTube series](https://www.youtube.com/playlist?list=PL8327DO66nu9qYVKLDmdLW_84-yE4auCR).

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
cmake --build . --target toy
.\bin\toy.exe
```

To build (debug) and run (Linux/macOS):

```command_line
mkdir build-debug
cd build-debug
cmake -GNinja -DCMAKE_BUILD_TYPE=Debug ../breakable_toy
cmake --build . --target toy
./bin/toy
```
