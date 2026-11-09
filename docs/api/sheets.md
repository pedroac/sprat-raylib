# Sheets

A sheet is one `.sprat` file plus its atlas image. Load it once, draw from it
many times, unload it once.

[Back to the cheat sheet](../cheatsheet.md)

## `LoadSpratSheet`

Loads a `.sprat` file and the atlas images it names.

```c
SpratSheet sheet = LoadSpratSheet("resources/hero.sprat");

if (!IsSpratSheetValid(sheet))
{
    TraceLog(LOG_WARNING, "Could not load the sheet");
}
```

Atlas paths are relative to the `.sprat` file's folder. Absolute paths are used
as written.

## `LoadSpratSheetFromMemory`

Same as `LoadSpratSheet()`, but the file is already in memory.

```c
int size = 0;
unsigned char *data = LoadFileData("resources/hero.sprat", &size);
SpratSheet sheet = LoadSpratSheetFromMemory(data, size, "resources");
UnloadFileData(data);
```

`atlasDir` is the folder used to find the atlas images. Pass `NULL` to use the
paths exactly as they are written in the sheet.

## `IsSpratSheetValid`

Tells you if the sheet loaded and every atlas image is on the GPU.

```c
if (!IsSpratSheetValid(sheet))
{
    // The sheet is empty, or an image is missing.
}
```

## `UnloadSpratSheet`

Frees the sheet's memory and its atlas textures. Call it once, when you are done.

```c
UnloadSpratSheet(sheet);
```

## `GetSpratFrameIndex`

Gets a frame index from a sprite path or a unique sprite name. Returns `-1` if
there is no match, or if the name is used more than once.

```c
int hero = GetSpratFrameIndex(sheet, "hero/run_01.png");  // by path
int coin = GetSpratFrameIndex(sheet, "coin");             // by unique name
```

Look names up once, after loading, and keep the index.

## `GetSpratFrameMarker`

Gets a marker from a frame by name. Returns `NULL` if the frame has no such
marker.

```c
const SpratMarker *hitbox = GetSpratFrameMarker(sheet, frame, "hitbox");

if (hitbox != NULL)
{
    // Use the marker.
}
```

## `DrawSpratFrame`

Draws a frame with its pivot at `position`.

```c
DrawSpratFrame(sheet, 0, (Vector2){ 400, 300 }, WHITE);
```

## `DrawSpratFrameEx`

Draws a frame with rotation and scale around its pivot.

```c
DrawSpratFrameEx(sheet, 0, (Vector2){ 400, 300 }, 15.0f, 2.0f, WHITE);
```

## `DrawSpratFrameNPatch`

Draws a frame stretched to fill `dest`. The nine-slice borders stay sharp.

```c
Rectangle panel = { 20, 20, 300, 110 };
DrawSpratFrameNPatch(sheet, panelFrame, panel, WHITE);
```

`dest` covers the whole original image, trimmed margins included. The pivot is
not used.
