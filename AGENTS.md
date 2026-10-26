# AGENTS.md

Guidance for AI agents (and humans) working on **sprat-raylib**.

## Purpose

sprat-raylib loads sprite sheets made by [sprat-cli](https://github.com/pedroac/sprat-cli)
into raylib and draws them. Parsing and validation belong to
[sprat-lib](https://github.com/pedroac/sprat-lib); this library converts its model into
raylib types, loads the atlas textures, and does the drawing math.

## Priorities

1. **Follow raylib's design.** Single-file header with `SPRAT_RAYLIB_IMPLEMENTATION`,
   like `raygui.h`. Plain structs passed by value, `int`/`float` and raylib types
   (`Vector2`, `Rectangle`, `Texture2D`) in the API, `PascalCase` functions named
   `Load*`/`Unload*`/`Is*Valid`/`Get*`/`Draw*`/`Draw*Ex`/`Draw*Pro`. No error
   codes: problems go to `TraceLog()` with a `SPRAT:` prefix, and loaders return a
   zeroed struct. raylib's code style: Allman braces, `camelCase` locals, `type *ptr`,
   `//` comments, and a one-line description after each declaration.
2. **Draw exactly what was authored.** Frames are placed by their pivot with trims
   and packing rotation undone. Every change to the drawing math needs the pixel
   tests in `tests/` to pass.
3. **Keep the sheet's order.** `frames[i]` is the sheet's `sprites[i]` and
   `animations[i]` its `animations[i]`, so sprat-lib's binding indexes work as they are.

## Code layout

- `src/sprat-raylib.h`: the library. The public part needs only `raylib.h`;
  `sprat/sprat.h` is included by the implementation alone.
- `src/sprat-raylib.c`: compiles the implementation for the CMake target.
- `tests/test_sprat_raylib.c`: runs without a window. It includes the implementation,
  so it can call internal functions: `SpratConvertSheet()` for loading, and
  `SpratGetFrameDrawCall()` / `SpratGetNPatchDrawCalls()`, which compute the
  `DrawTexturePro()` arguments. The draw tests rasterize those arguments the way
  raylib maps its quad and compare every pixel with the original image.
- `examples/sprite_sheet.c`: the demo, in raylib's example layout.
  `examples/resources/` holds a sheet packed by sprat-cli (`make_resources.sh`)
  from frames drawn by `make_sources.py`.

## Coding practices

- C99, warning-free with `-Wall -Wextra -Wpedantic -Wshadow -Wconversion`.
- Allocate with `RL_CALLOC`/`RL_FREE`. A sheet's metadata is one allocation that
  starts at `frames`; don't add per-record allocations.
- Drawing functions do nothing for an out-of-range index or an unloaded atlas;
  they don't log, since they run every frame.

## Build and test

```sh
cmake -S . -B build -DBUILD_TESTING=ON -DSPRAT_RAYLIB_SANITIZE=ON \
  -DFETCHCONTENT_SOURCE_DIR_SPRAT=../sprat-lib
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Add or update tests for every behavior change or bug fix. When the drawing changes,
also run `build/examples/sprite_sheet` and look at it.

## Versioning and commits

- Update `VERSION` (`vMAJOR.MINOR.PATCH`) and `SPRAT_RAYLIB_VERSION` in the header
  in the same commit as the change.
- Commit messages: `feat: ...`, `fix: ...`, `docs: ...`, `test: ...`, `refactor: ...`.
- Never commit build output.
