GoogleTest is vendored as a git submodule:

```
git submodule add https://github.com/google/googletest.git third_party/googletest
git submodule update --init --recursive
```

Pinned tag: v1.15.2 (or later). `tests/unit/CMakeLists.txt` also FetchContent-falls-back if the submodule is missing.
