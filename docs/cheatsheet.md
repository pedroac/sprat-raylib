# sprat-raylib cheat sheet

Load a sheet, draw a frame, animate it. Every function links to a page with an
example.

**New here?** Read the [guide](guide.md) first. It walks through a small game.

## Quick start

```c
#include "raylib.h"
#define SPRAT_RAYLIB_IMPLEMENTATION
#include "sprat-raylib.h"

int main(void)
{
    InitWindow(800, 450, "sprat-raylib");
    SpratSheet sheet =
        LoadSpratSheet("sheet.sprat");

    while (!WindowShouldClose())
    {
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawSpratFrame(sheet, 0,
                           (Vector2){ 400, 300 }, WHITE);
        EndDrawing();
    }

    UnloadSpratSheet(sheet);
    CloseWindow();
}
```

## Sheet

| Function | What it does |
|---|---|
| [`LoadSpratSheet`](api/sheets.md#loadspratsheet) | Load a `.sprat` file and its atlas images. |
| [`LoadSpratSheetFromMemory`](api/sheets.md#loadspratsheetfrommemory) | Load a sheet from bytes already in memory. |
| [`IsSpratSheetValid`](api/sheets.md#isspratsheetvalid) | Check that the sheet and its images loaded. |
| [`UnloadSpratSheet`](api/sheets.md#unloadspratsheet) | Free the sheet and its images. |
| [`GetSpratFrameIndex`](api/sheets.md#getspratframeindex) | Find a frame by path or unique name. |
| [`GetSpratFrameMarker`](api/sheets.md#getspratframemarker) | Find a marker on a frame by name. |

## Draw frames

| Function | What it does |
|---|---|
| [`DrawSpratFrame`](api/sheets.md#drawspratframe) | Draw a frame with its pivot at a position. |
| [`DrawSpratFrameEx`](api/sheets.md#drawspratframeex) | Draw a frame with rotation and scale. |
| [`DrawSpratFrameNPatch`](api/sheets.md#drawspratframenpatch) | Stretch a frame like a nine-slice panel. |

## Sprites

| Function | What it does |
|---|---|
| [`MakeSpratSprite`](api/sprites.md#makespratsprite) | Make a sprite from a frame. |
| [`DrawSpratSprite`](api/sprites.md#drawspratsprite) | Draw a sprite. |
| [`GetSpratSpritePoint`](api/sprites.md#getspratspritepoint) | Turn a point in the frame into a screen point. |
| [`GetSpratSpriteMarker`](api/sprites.md#getspratspritemarker) | Find a marker on a sprite's frame. |

## Animate

| Function | What it does |
|---|---|
| [`GetSpratAnimation`](api/animations.md#getspratanimation) | Find an animation by name. |
| [`IsSpratAnimationValid`](api/animations.md#isspratanimationvalid) | Check that the animation was found. |
| [`MakeSpratAnimationPlayer`](api/animations.md#makespratanimationplayer) | Make a player for an animation. |
| [`UpdateSpratAnimation`](api/animations.md#updatespratanimation) | Move the animation forward by `deltaTime`. |
| [`PlaySpratAnimation`](api/animations.md#playspratanimation) | Restart the animation. |
| [`StopSpratAnimation`](api/animations.md#stopspratanimation) | Rewind and stop the animation. |
| [`PauseSpratAnimation`](api/animations.md#pausespratanimation) | Pause the animation. |
| [`ResumeSpratAnimation`](api/animations.md#resumespratanimation) | Resume a paused animation. |
| [`SetSpratAnimationSpeed`](api/animations.md#setspratanimationspeed) | Set the speed; negative plays backward. |
| [`IsSpratAnimationPlaying`](api/animations.md#isspratanimationplaying) | Check if the animation is playing. |
| [`IsSpratAnimationFinished`](api/animations.md#isspratanimationfinished) | Check if a once animation ended. |
| [`GetSpratAnimationFrame`](api/animations.md#getspratanimationframe) | Get the frame shown now. |
| [`SetSpratAnimation`](api/animations.md#setspratanimation) | Change the animation; restarts only on a change. |

## Animate a sprite

| Function | What it does |
|---|---|
| [`SetSpratSpriteAnimationFrame`](api/animations.md#setspratspriteanimationframe) | Set a sprite's frame from a player, without advancing it. |
| [`UpdateSpratSpriteAnimation`](api/animations.md#updatespratspriteanimation) | Advance a player and set a sprite's frame. |
| [`DrawSpratAnimation`](api/animations.md#drawspratanimation) | Draw the frame shown now. |
| [`DrawSpratAnimationEx`](api/animations.md#drawspratanimationex) | Draw it with rotation and scale. |
| [`DrawSpratAnimationPro`](api/animations.md#drawspratanimationpro) | Draw it with any scale, rotation and flip. |

## Markers and collisions

| Function | What it does |
|---|---|
| [`GetSpratMarkerCenter`](api/markers.md#getspratmarkercenter) | Get the screen position of a marker's center. |
| [`GetSpratMarkerOutline`](api/markers.md#getspratmarkeroutline) | Get a marker's outline as screen points. |
| [`DrawSpratMarker`](api/markers.md#drawspratmarker) | Draw a marker's outline. |
| [`CheckCollisionSpratMarkers`](api/markers.md#checkcollisionspratmarkers) | Check if two markers touch. |
| [`CheckCollisionPointSpratMarker`](api/markers.md#checkcollisionpointspratmarker) | Check if a screen point is inside a marker. |

## Puppets

| Function | What it does |
|---|---|
| [`LoadSpratPuppet`](api/puppets.md#loadspratpuppet) | Load a puppet by name. |
| [`IsSpratPuppetValid`](api/puppets.md#isspratpuppetvalid) | Check that the puppet loaded. |
| [`UnloadSpratPuppet`](api/puppets.md#unloadspratpuppet) | Free the puppet's pose. |
| [`GetSpratPuppetPartIndex`](api/puppets.md#getspratpuppetpartindex) | Find a part by name. |
| [`GetSpratPuppetPoseIndex`](api/puppets.md#getspratpuppetposeindex) | Find a pose by name. |
| [`GetSpratPuppetSequenceIndex`](api/puppets.md#getspratpuppetsequenceindex) | Find a sequence by name. |
| [`ResetSpratPuppetPose`](api/puppets.md#resetspratpuppetpose) | Go back to the rest pose. |
| [`ApplySpratPuppetPose`](api/puppets.md#applyspratpuppetpose) | Apply a pose. |
| [`ApplySpratPuppetSequence`](api/puppets.md#applyspratpuppetsequence) | Apply a sequence at a time. |
| [`GetSpratPuppetSequenceDuration`](api/puppets.md#getspratpuppetsequenceduration) | Get how long a sequence takes. |
| [`IsSpratPuppetSequenceFinished`](api/puppets.md#isspratpuppetsequencefinished) | Check if a once sequence ended. |
| [`UpdateSpratPuppet`](api/puppets.md#updatespratpuppet) | Advance the parts' animations. |
| [`GetSpratPuppetPartSprite`](api/puppets.md#getspratpuppetpartsprite) | Get a part as a sprite. |
| [`GetSpratPuppetPartTint`](api/puppets.md#getspratpuppetparttint) | Get a part's tint. |
| [`DrawSpratPuppet`](api/puppets.md#drawspratpuppet) | Draw all parts. |

## More

- [Guide](guide.md) - a short walkthrough.
- [Sheets](api/sheets.md)
- [Sprites](api/sprites.md)
- [Animations](api/animations.md)
- [Markers](api/markers.md)
- [Puppets](api/puppets.md)
