# Markers

Markers are named shapes on a frame: pivots, hitboxes, hands, spawn points.
They are in the frame's original-image pixels.

[Back to the cheat sheet](../cheatsheet.md)

## `GetSpratMarkerCenter`

Gets the screen position of a marker's center.

```c
Vector2 hand = GetSpratMarkerCenter(hero, GetSpratSpriteMarker(hero, "hand"));
```

For a point, it is the point. For a circle, its center. For a rectangle, its
middle. For a polygon, its balance point (centroid).

## `GetSpratMarkerOutline`

Fills a closed outline in screen coordinates. Returns the number of points.
Pass `NULL` and `0` to just count them.

```c
Vector2 points[64];
int count = GetSpratMarkerOutline(hero, hitbox, points, 64);
DrawLineStrip(points, count, RED);
```

## `DrawSpratMarker`

Draws a marker's outline. A point is drawn as a cross.

```c
DrawSpratMarker(hero, hitbox, RED);
```

## `CheckCollisionSpratMarkers`

Checks if two markers touch or overlap. Works with points, circles, rectangles
and polygons, including shapes with a dent (concave polygons).

```c
if (CheckCollisionSpratMarkers(hero, heroHitbox, coin, coinHitbox))
{
    // Pick up the coin.
}
```

## `CheckCollisionPointSpratMarker`

Checks if a screen point, such as the mouse, is inside a marker.

```c
if (CheckCollisionPointSpratMarker(GetMousePosition(), hero, hitbox))
{
    // The mouse is on the hitbox.
}
```
