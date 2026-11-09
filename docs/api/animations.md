# Animations

An animation is a list of frames played at an fps. A player keeps the time.

[Back to the cheat sheet](../cheatsheet.md)

## `GetSpratAnimation`

Gets an animation from a sheet by name. Returns a zeroed animation if it is not
found.

```c
SpratAnimation run = GetSpratAnimation(sheet, "run");
```

## `IsSpratAnimationValid`

Checks that an animation was found. It may still have zero frames.

```c
if (!IsSpratAnimationValid(run))
{
    // There is no animation named "run".
}
```

## `MakeSpratAnimationPlayer`

Makes a player that starts at the beginning, at speed `1.0f`.

```c
SpratAnimationPlayer player = MakeSpratAnimationPlayer(run);
```

## `UpdateSpratAnimation`

Moves the animation forward by `deltaTime` seconds. Call it once per frame.

```c
UpdateSpratAnimation(&player, GetFrameTime());
```

## `PlaySpratAnimation`

Restarts the animation. If the speed is negative, it starts at the end.

```c
PlaySpratAnimation(&player);
```

## `StopSpratAnimation`

Rewinds the animation and stops it.

```c
StopSpratAnimation(&player);
```

## `PauseSpratAnimation`

Pauses the animation and keeps its time.

```c
PauseSpratAnimation(&player);
```

## `ResumeSpratAnimation`

Resumes a paused animation.

```c
ResumeSpratAnimation(&player);
```

## `SetSpratAnimationSpeed`

Sets the playback speed. `1.0f` is the animation's fps; negative plays backward.

```c
SetSpratAnimationSpeed(&player, 2.0f);   // double speed
SetSpratAnimationSpeed(&player, -1.0f);  // reverse
```

## `IsSpratAnimationPlaying`

Returns `true` if the animation is playing.

```c
if (IsSpratAnimationPlaying(player))
{
    // ...
}
```

## `IsSpratAnimationFinished`

Returns `true` when a once animation reached its end.

```c
if (IsSpratAnimationFinished(player))
{
    // The attack is over.
}
```

## `GetSpratAnimationFrame`

Gets the frame index shown right now. Returns `-1` if there is no frame.

```c
int frame = GetSpratAnimationFrame(player);
```

## `SetSpratAnimation`

Changes the animation played. It restarts only when the animation changes, so
you can call it every frame. When it changes, it keeps the current speed and
starts playing.

```c
SetSpratAnimation(&player, moving ? walk : idle);
```

## `SetSpratSpriteAnimationFrame`

Sets a sprite's frame and flip from a player without advancing the player.

```c
SetSpratSpriteAnimationFrame(&sprite, player);
```

Use it when several sprites share one player.

## `UpdateSpratSpriteAnimation`

Advances the player, then sets the sprite's frame and flip.

```c
UpdateSpratSpriteAnimation(&sprite, &player, GetFrameTime());
```

## `DrawSpratAnimation`

Draws the frame shown now, with the animation's flip.

```c
DrawSpratAnimation(sheet, player, (Vector2){ 400, 300 }, WHITE);
```

## `DrawSpratAnimationEx`

Draws the frame shown now with rotation and uniform scale.

```c
DrawSpratAnimationEx(sheet, player, (Vector2){ 400, 300 }, 15.0f, 2.0f, WHITE);
```

## `DrawSpratAnimationPro`

Draws the frame shown now with any scale, rotation and flip. The flip is
combined with the animation's flip.

```c
DrawSpratAnimationPro(sheet, player, (Vector2){ 400, 300 },
                      (Vector2){ 2.0f, 3.0f }, 15.0f,
                      SPRAT_FLIP_HORIZONTAL, WHITE);
```
