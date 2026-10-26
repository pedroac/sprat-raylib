# sprat-raylib

Load and draw [sprat-cli](https://github.com/pedroac/sprat-cli) sprite sheets with
[raylib](https://www.raylib.com/).

A single-file C99 library in the style of raylib's own extras (`raygui.h`,
`rres-raylib.h`): plain structs, `Load`/`Unload`/`Is…Valid`/`Draw` functions, and
`TraceLog()` for problems. It reads the metadata written by
`spratconvert --transform raw` through [sprat-lib](https://github.com/pedroac/sprat-lib),
loads the atlas textures next to it, and draws sprites the way they were
authored: placed by their pivot, with trimming and packing rotation undone.

![Example](examples/sprite_sheet.png)

```sh
spratlayout ./frames --trim-transparent --rotate > layout.txt
spratpack -a atlas.png < layout.txt > /dev/null
spratconvert --transform raw -a atlas.png \
    --markers markers.txt --animations animations.txt < layout.txt > hero.sprat
```

```c
#include "raylib.h"

#define SPRAT_RAYLIB_IMPLEMENTATION
#include "sprat-raylib.h"

int main(void)
{
    InitWindow(800, 450, "sprat-raylib");

    SpratSheet sheet = LoadSpratSheet("resources/hero.sprat");
    SpratAnimationPlayer run = MakeSpratAnimationPlayer(GetSpratAnimation(sheet, "run"));

    while (!WindowShouldClose())
    {
        UpdateSpratAnimation(&run, GetFrameTime());

        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawSpratAnimation(sheet, run, (Vector2){ 400, 300 }, WHITE);
        EndDrawing();
    }

    UnloadSpratSheet(sheet);
    CloseWindow();
    return 0;
}
```

## Building

`src/sprat-raylib.h` is the whole library. Define `SPRAT_RAYLIB_IMPLEMENTATION` in
exactly one C file before including it, and link raylib (5.5 or newer) and
sprat-lib (0.5 or newer):

```sh
cc game.c -Isrc -I../sprat-lib/include ../sprat-lib/src/*.c -lraylib -lm
```

With CMake, `sprat-raylib::sprat-raylib` (the `sprat-raylib` target) compiles the
implementation for you and brings raylib along:

```cmake
add_subdirectory(sprat-raylib)          # or find_package(sprat-raylib) after an install
target_link_libraries(game PRIVATE sprat-raylib::sprat-raylib)
```

```c
#include "sprat-raylib.h"               // no SPRAT_RAYLIB_IMPLEMENTATION: the library has it
```

The CMake build uses installed raylib and sprat packages when it finds them and
downloads them otherwise. To build against a local checkout, pass
`-DFETCHCONTENT_SOURCE_DIR_SPRAT=../sprat-lib` (or `FETCHCONTENT_SOURCE_DIR_RAYLIB`).

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/examples/sprite_sheet
```

| Option | Default | Effect |
|---|---|---|
| `SPRAT_RAYLIB_BUILD_EXAMPLES` | `ON` when top level | Build `examples/sprite_sheet` |
| `SPRAT_RAYLIB_SANITIZE` | `OFF` | Build with AddressSanitizer and UBSan (GCC/Clang) |

## API

```c
// Sheet loading/unloading functions
SpratSheet LoadSpratSheet(const char *fileName);
SpratSheet LoadSpratSheetFromMemory(const unsigned char *fileData, int dataSize, const char *atlasDir);
bool IsSpratSheetValid(SpratSheet sheet);
void UnloadSpratSheet(SpratSheet sheet);

// Sheet query functions
int GetSpratFrameIndex(SpratSheet sheet, const char *pathOrName);
const SpratMarker *GetSpratFrameMarker(SpratSheet sheet, int frame, const char *name);

// Sheet drawing functions
void DrawSpratFrame(SpratSheet sheet, int frame, Vector2 position, Color tint);
void DrawSpratFrameEx(SpratSheet sheet, int frame, Vector2 position, float rotation, float scale, Color tint);
void DrawSpratFrameNPatch(SpratSheet sheet, int frame, Rectangle dest, Color tint);

// Sprite functions
SpratSprite MakeSpratSprite(SpratSheet sheet, int frame, Vector2 position);
void DrawSpratSprite(SpratSprite sprite, Color tint);
Vector2 GetSpratSpritePoint(SpratSprite sprite, Vector2 point);
const SpratMarker *GetSpratSpriteMarker(SpratSprite sprite, const char *name);

// Marker functions
Vector2 GetSpratMarkerCenter(SpratSprite sprite, const SpratMarker *marker);
int GetSpratMarkerOutline(SpratSprite sprite, const SpratMarker *marker, Vector2 *points, int maxPoints);
void DrawSpratMarker(SpratSprite sprite, const SpratMarker *marker, Color color);
bool CheckCollisionSpratMarkers(SpratSprite spriteA, const SpratMarker *markerA, SpratSprite spriteB, const SpratMarker *markerB);
bool CheckCollisionPointSpratMarker(Vector2 point, SpratSprite sprite, const SpratMarker *marker);

// Animation functions
SpratAnimation GetSpratAnimation(SpratSheet sheet, const char *name);
bool IsSpratAnimationValid(SpratAnimation animation);
SpratAnimationPlayer MakeSpratAnimationPlayer(SpratAnimation animation);
void UpdateSpratAnimation(SpratAnimationPlayer *player, float deltaTime);
void PlaySpratAnimation(SpratAnimationPlayer *player);
void StopSpratAnimation(SpratAnimationPlayer *player);
void PauseSpratAnimation(SpratAnimationPlayer *player);
void ResumeSpratAnimation(SpratAnimationPlayer *player);
void SetSpratAnimationSpeed(SpratAnimationPlayer *player, float speed);
bool IsSpratAnimationPlaying(SpratAnimationPlayer player);
bool IsSpratAnimationFinished(SpratAnimationPlayer player);
int GetSpratAnimationFrame(SpratAnimationPlayer player);
void DrawSpratAnimation(SpratSheet sheet, SpratAnimationPlayer player, Vector2 position, Color tint);
```

### The sheet

`SpratSheet` is a plain struct, like raylib's `Model` or `Font`: pass it by
value, read its arrays directly, and unload it once.

```c
typedef struct SpratSheet {
    int atlasCount;             // Number of atlas textures
    Texture2D *atlases;         // Atlas textures
    int frameCount;             // Number of frames
    SpratFrame *frames;         // Frames, in sheet order
    int animationCount;         // Number of animations
    SpratAnimation *animations; // Animations, in sheet order
    float scale;                // Scale the art was packed at: draw at 1.0f/scale for its original size
} SpratSheet;
```

- **Frames** are the sheet's sprites, in its order. A `SpratFrame` keeps the
  atlas region as stored (`source`, `rotation`), the original image's `size`,
  the trimmed `content` region inside it, the `pivot`, the nine-slice insets
  (`slice`) and the frame's `markers`. Frame geometry is in original-image
  pixels, scaled to atlas pixels, with Y pointing down.
- **Animations** list frame indexes in play order, with an `fps` and the `flip`
  they are played with. Aliases are already resolved: `run-left`, an alias of
  `run` with `flip h`, has `run`'s frames (the same array, not a copy) and `flip` set to
  `SPRAT_FLIP_HORIZONTAL`. An animation may have no frames.
- **Names**: a frame's `name` is its file's basename and can repeat across
  folders; its `path` is unique. `GetSpratFrameIndex()` takes either: it
  matches the path first, then a name that only one frame has, and logs a
  warning and returns -1 otherwise. Look names up once, after loading, and
  keep the indexes.

Frames and animations keep the order of the sprat file, so the indexes that
sprat-lib's `sprat_sheet_bind()` resolves (and its animation frame indexes)
index `sheet.frames` and `sheet.animations` as they are.

### Drawing

Every draw function places a frame by its **pivot**, the point
`spratconvert` was given with a `pivot` marker (`0, 0`, the top-left of the
original image, when there was none). Trimmed margins are left out, and
frames that were packed turned are drawn upright.

`DrawSpratFrame()` and `DrawSpratFrameEx()` are the shorthands raylib users
expect. For anything more, place the frame with a sprite.

### Sprites

A `SpratSprite` is a frame placed on the screen. It is a plain struct the
program owns and changes directly, such as `frame` from
`GetSpratAnimationFrame()` every update:

```c
typedef struct SpratSprite {
    SpratSheet sheet;           // Sheet the frame belongs to (shared, not owned: unload the sheet once)
    int frame;                  // Frame index in sheet.frames
    Vector2 position;           // Screen position of the frame's pivot
    Vector2 scale;              // Scale around the pivot, negative to mirror
    float rotation;             // Clockwise rotation around the pivot, in degrees
    int flip;                   // Flip flags around the pivot (SpratFlip)
} SpratSprite;
```

The frame is scaled, then mirrored about its pivot with
`SPRAT_FLIP_HORIZONTAL` and/or `SPRAT_FLIP_VERTICAL`, then turned clockwise
around the pivot by `rotation` degrees, the same convention as
`DrawTexturePro()`. `DrawSpratSprite()` draws it, and the marker functions
place markers with the same transform, so what is drawn and what collides
always agree. `MakeSpratSprite()` returns one unscaled, unrotated and
unflipped; a zeroed sprite has a scale of 0 and draws nothing.

`scale` is yours to choose, and is not `sheet.scale`: frame geometry is in atlas
pixels, so `1.0f` draws atlas pixels one to one. For a sheet packed at
`--scale 0.5`, draw at `1.0f/sheet.scale` to show the art at its original
size.

### Animations

An animation from the sheet is a template: `GetSpratAnimation()` returns it by
name (zeroed, with a warning, when there is none; `IsSpratAnimationValid()`
tells). Playing it takes a `SpratAnimationPlayer`, a plain struct you keep one
of per animated thing, the way raylib's `Music` is played and updated:

```c
typedef struct SpratAnimationPlayer {
    SpratAnimation animation;   // Animation played (shares the sheet's frame array)
    float time;                 // Seconds into the animation, from 0 to its duration (frameCount/fps)
    float speed;                // Playback rate: 1.0f plays at the animation's fps, negative plays in reverse
    bool playing;               // Advanced by UpdateSpratAnimation()
    bool loop;                  // Wrap around at the end, otherwise stop on the last frame
} SpratAnimationPlayer;
```

`MakeSpratAnimationPlayer()` returns one already playing, looping, at speed
`1.0f`; it allocates nothing, so copy it or drop it freely. Call
`UpdateSpratAnimation(&player, GetFrameTime())` once per frame: time only moves
while you update it, so pausing the game, slow motion or a fixed time step
apply to animations too. `PauseSpratAnimation()` and `ResumeSpratAnimation()`
keep the time, `PlaySpratAnimation()` restarts and `StopSpratAnimation()`
rewinds without playing.

A negative speed plays in reverse, and `PlaySpratAnimation()` then starts from
the last frame. Set `loop` to `false` to play once: the player stops on the
last frame it reaches (the first when reversed) and `IsSpratAnimationFinished()`
becomes true. The fields are yours to read and set, `time` included.

`GetSpratAnimationFrame()` gives the frame shown now, an index into
`sheet.frames`, for its markers or a hitbox. `DrawSpratAnimation()` draws it with
the animation's flip. To draw it with your own transform, put it in a sprite
and combine that flip with yours:

```c
enemy.sprite.frame = GetSpratAnimationFrame(enemy.walk);
enemy.sprite.flip = enemy.walk.animation.flip ^ enemy.facing;
DrawSpratSprite(enemy.sprite, WHITE);
```

### Markers

Markers (`pivot`, hitboxes, hands, spawn points...) are points, circles,
rectangles and polygons in original-image pixels. They mean something once
placed with a sprite of their frame, and each frame has its own, so look them
up again when the sprite's frame changes:

```c
// The sword is held by the hero's "hand" marker, mirrored with the hero
sword.position = GetSpratMarkerCenter(hero, GetSpratSpriteMarker(hero, "hand"));

// A coin is picked when the hero's hitbox touches the coin's
if (CheckCollisionSpratMarkers(hero, GetSpratSpriteMarker(hero, "hitbox"),
                               coin, GetSpratSpriteMarker(coin, "hitbox"))) ...

if (debug) DrawSpratMarker(hero, GetSpratSpriteMarker(hero, "hitbox"), RED);
```

- `GetSpratMarkerCenter()` gives where a marker is: a point's point, a circle's
  center, a rectangle's middle or a polygon's centroid (outside a concave
  polygon whose middle is cut away). `GetSpratSpritePoint()` places any point
  of the original image.
- `CheckCollisionSpratMarkers()` compares the placed shapes exactly: a
  rectangle turns with its sprite, a circle scaled unevenly is an ellipse, and
  polygons may be concave. Touching counts, and so does a shape inside the
  other. The sprites may come from different sheets, and a marker need not
  belong to its sprite's frame: a program-made `SpratMarker` in the frame's
  original-image pixels works too, such as an attack range.
  `CheckCollisionPointSpratMarker()` tests a screen point, such as the mouse.
- `GetSpratMarkerOutline()` fills a closed outline for `DrawLineStrip()` and,
  like `snprintf()`, returns how many points it has (pass `NULL` and 0 to
  count them). `DrawSpratMarker()` draws it, with a cross for a point.

Markers are passed by pointer so that a lookup's `NULL`, for a frame without
that marker, goes straight in: nothing collides with it,
`GetSpratMarkerCenter()` returns the sprite's position and the outline is
empty. None of them log, since they run every frame.

### Nine-slices

`DrawSpratFrameNPatch()` draws a frame stretched to fill `dest`, keeping its
slice borders unscaled, like `DrawTextureNPatch()`. `dest` covers the whole
original image, trimmed margins included, and the pivot plays no part. It
cuts the slices in original-image space, so it handles trimmed frames and
frames packed turned, which `DrawTextureNPatch()` cannot. Borders shrink
proportionally when `dest` is smaller than them, and a frame without slice is
simply stretched.

Every fill mode is drawn as `SPRAT_SLICE_MODE_STRETCH`. `slice.horizontalMode`
and `slice.verticalMode` say which mode the artist chose, for code that tiles
the middle itself.

### Loading

`LoadSpratSheet()` reads the file with `LoadFileData()`, so it honors
`SetLoadFileDataCallback()`, and loads each atlas with `LoadTexture()`, joining
its path with the sheet's folder (absolute paths are used as written).
`LoadSpratSheetFromMemory()` takes the folder to resolve atlas paths against,
or `NULL` to use them as written.

Like raylib's loaders, these never return an error: problems go to
`TraceLog()`, with sprat-lib's message naming the value at fault and where it
is:

```
WARNING: SPRAT: [resources/hero.sprat] Failed to load sheet: 4:14: sprites[0].rect.w: expected an integer, found "4x"
```

A sheet that failed to parse is zeroed, and drawing with it draws nothing. A
missing atlas leaves its texture at `id` 0, so the frames on it are skipped.
`IsSpratSheetValid()` is true only when the metadata loaded and every atlas
texture is in the GPU. You can replace an atlas: unload `sheet.atlases[i]` and
assign another texture, and `UnloadSpratSheet()` will unload that one.

### Memory

The metadata is copied into one allocation (with `RL_CALLOC`, released with
`RL_FREE`), and sprat-lib's parsed sheet is freed before the loader returns.
Every array and string of a `SpratSheet` points into that allocation and stays
valid until `UnloadSpratSheet()`. `sheet.frames` is its start, so it is not
`NULL` after a successful load even when the sheet has no frames.

## Example

[`examples/sprite_sheet.c`](examples/sprite_sheet.c) draws every frame of the
example sheet (most of them stored turned in the atlas), plays the `run`,
`run-left` and `spin` animations, resizes a nine-patch panel, and lets you run
a hero holding a sword at its `hand` marker, picking up spinning coins its
hitbox or the sword's `blade` touches. `D` shows pivots, hitboxes and the
blade.

The sheet in `examples/resources/` was packed with sprat-cli by
`make_resources.sh`, from frames drawn by `make_sources.py`.

## Limitations

- Normal and collision maps are not loaded. sprat-lib gives their paths if
  you need them.
- Atlas paths are used as written, so a sheet from an untrusted source could
  name any file. Parse such a sheet with sprat-lib first and check each
  `atlases[i].path` with `sprat_path_is_safe()` before loading it here.
- The sheet format has no loop setting yet: every player loops until you set
  `loop` to `false`.

## License

MIT. See [LICENSE](LICENSE).
