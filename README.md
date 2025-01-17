# TotalGB

PR's, issues and any help is very much welcome!

## Building SDL2

you will need to install the following packages.

- sdl2

for linux, use your package manager. on windows, use vcpkg.

```cmake
cmake -B build -DPLATFORM_SDL2=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Credits

- <https://gbdev.github.io/pandocs/>
