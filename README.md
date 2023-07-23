# pacemaker

### Requirements
##### Build
- pre-commit
- clang-format

##### Runtime
- JACK Audio Server

### Build
Setup pre-commit hooks:
```sh
$ pre-commit install
```

Configure CMake:
```sh
$ cmake --list-presets
$ cmake --preset linux-clang-debug
```

Compile:
```sh
$ cd build
$ cmake --build . --config debug
```
