# sprat-raylib

Draw [sprat-cli](https://github.com/pedroac/sprat-cli) sprite sheets with
[raylib](https://www.raylib.com/).

A sprite sheet is one `.sprat` file plus an atlas image (one image that holds
all the frames). sprat-raylib loads them, then draws frames, sprites,
animations and puppets. It is a single header, like `raygui.h`.

![Example](examples/sprite_sheet.png)

## Quick start

```c
#include "raylib.h"
#define SPRAT_RAYLIB_IMPLEMENTATION
#include "sprat-raylib.h"

int main(void)
{
    InitWindow(800, 450, "sprat-raylib");

    SpratSheet sheet = LoadSpratSheet("resources/sheet.sprat");
    SpratAnimationPlayer run =
        MakeSpratAnimationPlayer(GetSpratAnimation(sheet, "run"));

    while (!WindowShouldClose())
    {
        UpdateSpratAnimation(&run, GetFrameTime());

        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawSpratAnimation(sheet, run,
                               (Vector2){ 400, 300 }, WHITE);
        EndDrawing();
    }

    UnloadSpratSheet(sheet);
    CloseWindow();
    return 0;
}
```

## Install

Copy `src/sprat-raylib.h` into your project. In one `.c` file, define
`SPRAT_RAYLIB_IMPLEMENTATION` before including it. Link raylib 5.5+ and
sprat-lib 0.15+.

```sh
cc game.c -Isrc -I../sprat-lib/include ../sprat-lib/src/*.c -lraylib -lm
```

With CMake:

```cmake
add_subdirectory(sprat-raylib)
target_link_libraries(game PRIVATE sprat-raylib::sprat-raylib)
```

## Learn

- [Guide](docs/guide.md) - a short walkthrough with examples.
- [Cheat sheet](docs/cheatsheet.md) - every function, grouped by what you want
  to do.
- [API pages](docs/api/sheets.md) - one page per area, with examples.

## Example

[`examples/sprite_sheet.c`](examples/sprite_sheet.c) draws every frame, plays
animations, resizes a nine-slice panel, and lets you run a hero holding a sword
and pick up coins.

## License

MIT. See [LICENSE](LICENSE).
