# sprat-raylib guide

This guide gets you from a `.sprat` file to moving sprites. It is short on
purpose. For every function, see the [cheat sheet](cheatsheet.md).

## What you need

- raylib 5.5 or newer.
- sprat-lib 0.15 or newer.
- A sheet made by sprat-cli: one `.sprat` file and its atlas image (one image
  that holds all the frames).

## Build

`src/sprat-raylib.h` is the whole library. In one `.c` file:

```c
#define SPRAT_RAYLIB_IMPLEMENTATION
#include "sprat-raylib.h"
```

Link raylib and sprat-lib:

```sh
cc game.c -Isrc -I../sprat-lib/include ../sprat-lib/src/*.c -lraylib -lm
```

With CMake, link `sprat-raylib::sprat-raylib`. It brings raylib along.

## Load a sheet

```c
SpratSheet sheet = LoadSpratSheet("resources/sheet.sprat");

if (!IsSpratSheetValid(sheet))
{
    TraceLog(LOG_WARNING, "Could not load resources/sheet.sprat");
}

// ...

UnloadSpratSheet(sheet);
```

`LoadSpratSheet()` reads the `.sprat` file and loads the atlas images it names.
The image paths are relative to the `.sprat` file's folder.

## Draw a frame

```c
DrawSpratFrame(sheet, 0, (Vector2){ 400, 300 }, WHITE);
```

The frame is drawn by its **pivot**: the point the artist chose to place it by.
`DrawSpratFrameEx()` also rotates and scales it.

## Animate

```c
SpratAnimationPlayer run = MakeSpratAnimationPlayer(GetSpratAnimation(sheet, "run"));

while (!WindowShouldClose())
{
    UpdateSpratAnimation(&run, GetFrameTime());

    BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawSpratAnimation(sheet, run, (Vector2){ 400, 300 }, WHITE);
    EndDrawing();
}
```

An animation is a list of frames. A **player** keeps the time. Keep one player
per animated thing. `SetSpratAnimation()` changes the animation and restarts it
only when it changes, so you can call it every frame.

## Use a sprite

A sprite is a frame placed on the screen. Change its fields directly.

```c
SpratSprite hero = MakeSpratSprite(sheet, heroFrame, (Vector2){ 400, 300 });
hero.scale = (Vector2){ 3.0f, 3.0f };
hero.rotation = 10.0f;

DrawSpratSprite(hero, WHITE);
```

`UpdateSpratSpriteAnimation()` keeps a sprite and a player together:

```c
SetSpratAnimation(&player, moving ? walk : idle);
UpdateSpratSpriteAnimation(&hero, &player, GetFrameTime());
```

## Markers and collisions

Markers are named shapes on a frame: pivots, hitboxes, hands, spawn points.

```c
const SpratMarker *hitbox = GetSpratSpriteMarker(hero, "hitbox");

if (CheckCollisionSpratMarkers(hero, hitbox, coin, coinHitbox))
{
    // Pick up the coin.
}
```

`DrawSpratMarker()` draws a marker so you can see it while debugging.

## Nine-slices

A nine-slice frame stretches without blurring its corners.

```c
Rectangle panel = { 20, 20, 300, 110 };
DrawSpratFrameNPatch(sheet, panelFrame, panel, WHITE);
```

## Puppets

A puppet is a character made of parts. The parts are joined by markers and
moved by poses and sequences.

```c
SpratPuppet kid = LoadSpratPuppet(sheet, "kid");
int walk = GetSpratPuppetSequenceIndex(kid, "walk");

// ...

UpdateSpratPuppet(&kid, GetFrameTime());
ResetSpratPuppetPose(kid);
ApplySpratPuppetSequence(kid, walk, walkTime);
DrawSpratPuppet(kid, WHITE);

// ...

UnloadSpratPuppet(kid);
```

## Good to know

- `SpratSheet` is a plain struct. Pass it by value. Unload it once.
- `sheet.scale` is the scale the art was packed at. Draw at `1.0f/sheet.scale`
  for its original size.
- `GetSpratFrameIndex()` takes a path, or a name that only one frame uses.
- Drawing functions skip bad frame indexes and unloaded images. They do not log.
- Names and markers point into the sheet. Do not use them after
  `UnloadSpratSheet()`.
- A sheet is one allocation. Each puppet adds one more. Unload the puppets,
  then the sheet.

## Limits

- Atlas paths are used as written. For untrusted sheets, check them with
  sprat-lib's `sprat_path_is_safe()` first.
- Every nine-slice fill mode is drawn as stretch.

## More

- [Cheat sheet](cheatsheet.md)
- [Sheets](api/sheets.md)
- [Sprites](api/sprites.md)
- [Animations](api/animations.md)
- [Markers](api/markers.md)
- [Puppets](api/puppets.md)
