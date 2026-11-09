# Puppets

A puppet is a character made of parts, joined by markers and moved by poses and
sequences. Load one per puppet on screen.

[Back to the cheat sheet](../cheatsheet.md)

## `LoadSpratPuppet`

Loads a puppet by name in its rest pose.

```c
SpratPuppet kid = LoadSpratPuppet(sheet, "kid");
```

## `IsSpratPuppetValid`

Checks that the puppet loaded.

```c
if (!IsSpratPuppetValid(kid))
{
    // There is no puppet named "kid".
}
```

## `UnloadSpratPuppet`

Frees the puppet's pose. It does not unload the sheet.

```c
UnloadSpratPuppet(kid);
```

## `GetSpratPuppetPartIndex`

Gets a part index by name. Returns `-1` if it is not found.

```c
int hand = GetSpratPuppetPartIndex(kid, "hand");
```

## `GetSpratPuppetPoseIndex`

Gets a pose index by name. Returns `-1` if it is not found.

```c
int smile = GetSpratPuppetPoseIndex(kid, "smile");
```

## `GetSpratPuppetSequenceIndex`

Gets a sequence index by name. Returns `-1` if it is not found.

```c
int walk = GetSpratPuppetSequenceIndex(kid, "walk");
```

## `ResetSpratPuppetPose`

Resets the puppet to its rest pose. Call it before applying poses and
sequences each update.

```c
ResetSpratPuppetPose(kid);
```

## `ApplySpratPuppetPose`

Applies a pose over the current pose.

```c
ApplySpratPuppetPose(kid, smile);
```

## `ApplySpratPuppetSequence`

Applies a sequence at `time` seconds after it started, over the current pose.

```c
ApplySpratPuppetSequence(kid, walk, walkTime);
```

## `GetSpratPuppetSequenceDuration`

Gets how many seconds one pass of a sequence takes.

```c
float length = GetSpratPuppetSequenceDuration(kid, walk);
```

## `IsSpratPuppetSequenceFinished`

Checks if a once sequence has ended.

```c
if (IsSpratPuppetSequenceFinished(kid, wave, waveTime))
{
    // The wave is over.
}
```

## `UpdateSpratPuppet`

Advances the animations the parts show by `deltaTime` seconds.

```c
UpdateSpratPuppet(&kid, GetFrameTime());
```

## `GetSpratPuppetPartSprite`

Gets a part as a `SpratSprite`, so you can draw it or use its markers.

```c
int hand = GetSpratPuppetPartIndex(kid, "hand");
SpratSprite handSprite = GetSpratPuppetPartSprite(kid, hand);
```

## `GetSpratPuppetPartTint`

Gets a part's tint, combined with its pose tint and opacity.

```c
Color tint = GetSpratPuppetPartTint(kid, part, WHITE);
```

## `DrawSpratPuppet`

Draws all parts in order, each with its pose tint and opacity.

```c
DrawSpratPuppet(kid, WHITE);
```
