# Sprites

A sprite is a frame placed on the screen. Change its fields directly.

[Back to the cheat sheet](../cheatsheet.md)

## `MakeSpratSprite`

Makes a sprite with its pivot at `position`, unscaled, unrotated and unflipped.

```c
SpratSprite hero = MakeSpratSprite(sheet, heroFrame, (Vector2){ 400, 300 });
hero.scale = (Vector2){ 3.0f, 3.0f };
```

## `DrawSpratSprite`

Draws a sprite: scaled, flipped and rotated around its pivot.

```c
DrawSpratSprite(hero, WHITE);
```

## `GetSpratSpritePoint`

Gets the screen position of a point in the frame's original image.

```c
Vector2 hand = GetSpratSpritePoint(hero, (Vector2){ 24, 30 });
```

## `GetSpratSpriteMarker`

Gets a marker of the sprite's current frame by name. Returns `NULL` if the frame
has no such marker.

```c
const SpratMarker *hitbox = GetSpratSpriteMarker(hero, "hitbox");
```

Look it up again when the sprite's frame changes.
