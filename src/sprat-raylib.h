/**********************************************************************************************
*
*   sprat-raylib v0.1.0 - Load and draw sprat-cli sprite sheets with raylib
*
*   DESCRIPTION:
*       Loads the metadata written by `spratconvert --transform raw` (through sprat-lib)
*       together with its atlas textures, and draws sprites the way they were authored:
*       placed by their pivot, with trimming and packing rotation undone, optionally
*       scaled, rotated, flipped, animated or stretched as nine-slices.
*
*       A SpratSheet is a plain struct, like raylib's Model or Font: load it once,
*       draw it by value, unload it once.
*
*           SpratSheet sheet = LoadSpratSheet("resources/hero.sprat");
*           int run = GetSpratAnimationIndex(sheet, "run");
*           ...
*           DrawSpratAnimation(sheet, run, (float)GetTime(), (Vector2){ 400, 300 }, WHITE);
*           ...
*           UnloadSpratSheet(sheet);
*
*       Frames keep the order of the sprites in the sheet, and animations the order of
*       the sheet's animations, so indexes from sprat_sheet_bind() work as they are.
*
*   CONFIGURATION:
*       #define SPRAT_RAYLIB_IMPLEMENTATION
*           Generates the implementation of the library into the included file.
*           If not defined, the library is in header only mode and can be included in
*           other headers or source files without problems. But only ONE file should
*           hold the implementation.
*
*   DEPENDENCIES:
*       raylib 5.5 or newer     - Textures, drawing, file loading and logging
*       sprat-lib 0.5 or newer  - Parsing and validating the .sprat file (implementation only)
*
*   COORDINATES:
*       Frame geometry (pivot, content, size, slice, markers) is in original-image pixels,
*       scaled to atlas pixels, with the origin at the top-left and Y pointing down.
*       Rotations are in degrees, clockwise on screen, like DrawTexturePro().
*
*   LICENSE: MIT
*
*   Copyright (c) 2026 Pedro Amaral Couto
*
**********************************************************************************************/

#ifndef SPRAT_RAYLIB_H
#define SPRAT_RAYLIB_H

#include "raylib.h"

#define SPRAT_RAYLIB_VERSION    "0.1.0"

// Function specifiers in case library is build/used as a shared library (Windows)
#if defined(_WIN32) && defined(SPRAT_RAYLIB_BUILD_SHARED)
    #define SPRATAPI __declspec(dllexport)
#elif defined(_WIN32) && defined(SPRAT_RAYLIB_USE_SHARED)
    #define SPRATAPI __declspec(dllimport)
#endif
#ifndef SPRATAPI
    #define SPRATAPI
#endif

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------

// Flip flags, combined with |
typedef enum {
    SPRAT_FLIP_HORIZONTAL = 1,  // Mirror around the pivot's vertical axis
    SPRAT_FLIP_VERTICAL = 2     // Mirror around the pivot's horizontal axis
} SpratFlip;

// Marker shape
typedef enum {
    SPRAT_MARKER_TYPE_POINT = 0,    // position
    SPRAT_MARKER_TYPE_CIRCLE,       // position, radius
    SPRAT_MARKER_TYPE_RECTANGLE,    // position (top-left), size
    SPRAT_MARKER_TYPE_POLYGON       // vertices (position holds the first one)
} SpratMarkerType;

// Nine-slice fill mode of the stretchable regions, per axis
typedef enum {
    SPRAT_SLICE_MODE_STRETCH = 0,
    SPRAT_SLICE_MODE_REPEAT,
    SPRAT_SLICE_MODE_MIRROR,
    SPRAT_SLICE_MODE_FIXED
} SpratSliceMode;

// Named shape attached to a frame (pivot, hitbox, spawn point...), in original-image pixels
typedef struct SpratMarker {
    const char *name;       // Marker name
    int type;               // Marker shape (SpratMarkerType)
    Vector2 position;       // Point, circle center or rectangle top-left
    float radius;           // Circle radius
    Vector2 size;           // Rectangle size
    Vector2 *vertices;      // Polygon vertices
    int vertexCount;        // Polygon vertex count (at least 1 for polygons)
} SpratMarker;

// Nine-slice insets, in original-image pixels (all 0 when the frame has no slice)
typedef struct SpratSlice {
    float left;             // Left inset
    float top;              // Top inset
    float right;            // Right inset
    float bottom;           // Bottom inset
    int horizontalMode;     // Horizontal fill mode (SpratSliceMode)
    int verticalMode;       // Vertical fill mode (SpratSliceMode)
} SpratSlice;

// One sprite of the sheet, as packed in its atlas
typedef struct SpratFrame {
    const char *name;       // Sprite name: file basename without extension (may repeat)
    const char *path;       // Sprite source path (unique in the sheet)
    int atlas;              // Atlas texture index
    Rectangle source;       // Region in the atlas texture, as stored (rotated when rotation != 0)
    int rotation;           // Clockwise degrees applied when packed: 0, 90 or 270
    Vector2 size;           // Original image size, trimmed margins included
    Rectangle content;      // Trimmed image region inside the original image, upright
    Vector2 pivot;          // Pivot inside the original image (0, 0 when none was set)
    SpratSlice slice;       // Nine-slice insets
    SpratMarker *markers;   // Frame markers
    int markerCount;        // Frame marker count
} SpratFrame;

// Sequence of frames played at a fixed rate
typedef struct SpratAnimation {
    const char *name;       // Animation name
    int fps;                // Frames per second (1..100)
    int *frames;            // Frame indexes, in play order
    int frameCount;         // Frame index count (may be 0)
    int flip;               // Flip flags the animation is played with (SpratFlip)
} SpratAnimation;

// Sprite sheet: atlas textures, frames and animations
typedef struct SpratSheet {
    int atlasCount;             // Number of atlas textures
    Texture2D *atlases;         // Atlas textures
    int frameCount;             // Number of frames
    SpratFrame *frames;         // Frames, in sheet order (also the start of the sheet's memory: don't reassign it)
    int animationCount;         // Number of animations
    SpratAnimation *animations; // Animations, in sheet order
    float scale;                // Scale the art was packed at: draw at 1.0f/scale for its original size
} SpratSheet;

#if defined(__cplusplus)
extern "C" {            // Prevents name mangling of functions
#endif

//------------------------------------------------------------------------------------
// Sprite Sheet Functions Declaration
//------------------------------------------------------------------------------------

// Sheet loading/unloading functions
SPRATAPI SpratSheet LoadSpratSheet(const char *fileName);                      // Load sheet from file, atlases are loaded from the sheet's folder
SPRATAPI SpratSheet LoadSpratSheetFromMemory(const unsigned char *fileData, int dataSize, const char *atlasDir); // Load sheet from memory, atlases are loaded from atlasDir (NULL: paths as written)
SPRATAPI bool IsSpratSheetValid(SpratSheet sheet);                             // Check if a sheet is valid (metadata loaded and every atlas texture in GPU)
SPRATAPI void UnloadSpratSheet(SpratSheet sheet);                              // Unload sheet metadata and atlas textures

// Sheet query functions
SPRATAPI int GetSpratFrameIndex(SpratSheet sheet, const char *pathOrName);     // Get frame index by sprite path, or by name when it is unique, -1 if not found
SPRATAPI int GetSpratAnimationIndex(SpratSheet sheet, const char *name);       // Get animation index by name, -1 if not found
SPRATAPI const SpratMarker *GetSpratFrameMarker(SpratSheet sheet, int frame, const char *name); // Get frame marker by name, NULL if not found
SPRATAPI int GetSpratAnimationFrame(SpratSheet sheet, int animation, float time); // Get frame index shown after time seconds of looped playback, -1 if none
SPRATAPI Vector2 GetSpratFramePoint(SpratSheet sheet, int frame, Vector2 point, Vector2 position, Vector2 scale, float rotation, int flip); // Get screen position of an original-image point, drawn as DrawSpratFramePro() would

// Sheet drawing functions
SPRATAPI void DrawSpratFrame(SpratSheet sheet, int frame, Vector2 position, Color tint); // Draw a frame with its pivot at position
SPRATAPI void DrawSpratFrameEx(SpratSheet sheet, int frame, Vector2 position, float rotation, float scale, Color tint); // Draw a frame with rotation and scale around its pivot
SPRATAPI void DrawSpratFramePro(SpratSheet sheet, int frame, Vector2 position, Vector2 scale, float rotation, int flip, Color tint); // Draw a frame with scale, rotation and flip around its pivot
SPRATAPI void DrawSpratFrameNPatch(SpratSheet sheet, int frame, Rectangle dest, Color tint); // Draw a frame stretched to dest, keeping its nine-slice borders unscaled
SPRATAPI void DrawSpratAnimation(SpratSheet sheet, int animation, float time, Vector2 position, Color tint); // Draw the animation frame shown after time seconds, with the animation's flip

#if defined(__cplusplus)
}
#endif

#endif // SPRAT_RAYLIB_H

/***********************************************************************************
*
*   SPRAT_RAYLIB IMPLEMENTATION
*
************************************************************************************/

#if defined(SPRAT_RAYLIB_IMPLEMENTATION)

#include "sprat/sprat.h"    // Required for: sprat_sheet_load_memory(), sprat_sheet_free()...

#include <limits.h>         // Required for: INT_MAX
#include <math.h>           // Required for: sinf(), cosf(), floor(), fmod(), isfinite()
#include <stdint.h>         // Required for: SIZE_MAX, uintptr_t
#include <stdlib.h>         // Required for: calloc(), free() [Used in RL_CALLOC(), RL_FREE()]
#include <string.h>         // Required for: strlen(), strcmp(), memcpy()

#ifndef SPRAT_MAX_PATH_LENGTH
    #define SPRAT_MAX_PATH_LENGTH   4096    // Maximum length of an atlas path, joined with its folder
#endif

//----------------------------------------------------------------------------------
// Types and Structures Definition (internal)
//----------------------------------------------------------------------------------

// Arguments for one DrawTexturePro() call
typedef struct SpratDrawCall {
    Rectangle source;
    Rectangle dest;
    Vector2 origin;
    float rotation;
} SpratDrawCall;

//----------------------------------------------------------------------------------
// Module Internal Functions Declaration
//----------------------------------------------------------------------------------
static SpratSheet SpratLoadSheetData(const unsigned char *fileData, int dataSize, const char *atlasDir, const char *fileName);
static SpratSheet SpratConvertSheet(const sprat_sheet *raw);
static bool SpratJoinPath(char *buffer, int bufferSize, const char *dir, const char *path);
static bool SpratIsFrameDrawable(SpratSheet sheet, int frame);
static Vector2 SpratTransformPoint(Vector2 pivot, Vector2 point, Vector2 position, Vector2 scale, float rotation, int flip);
static Rectangle SpratGetStoredRegion(const SpratFrame *frame, float u0, float v0, float u1, float v1);
static SpratDrawCall SpratGetRegionDrawCall(const SpratFrame *frame, Rectangle source, Vector2 center, Vector2 size, float rotation, int flip);
static SpratDrawCall SpratGetFrameDrawCall(const SpratFrame *frame, Vector2 position, Vector2 scale, float rotation, int flip);
static int SpratGetNPatchDrawCalls(const SpratFrame *frame, Rectangle dest, SpratDrawCall calls[9]);

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------

// Load sheet from file, atlases are loaded from the sheet's folder
SpratSheet LoadSpratSheet(const char *fileName)
{
    SpratSheet sheet = { 0 };

    if (fileName == NULL)
    {
        TraceLog(LOG_WARNING, "SPRAT: Failed to load sheet: no file name");
        return sheet;
    }

    int dataSize = 0;
    unsigned char *fileData = LoadFileData(fileName, &dataSize);

    if (fileData != NULL)
    {
        // GetDirectoryPath() returns a static string, so keep a copy
        char atlasDir[SPRAT_MAX_PATH_LENGTH] = { 0 };
        const char *dir = GetDirectoryPath(fileName);
        size_t dirLength = strlen(dir);

        if (dirLength < sizeof(atlasDir))
        {
            memcpy(atlasDir, dir, dirLength + 1);
            sheet = SpratLoadSheetData(fileData, dataSize, atlasDir, fileName);
        }
        else TraceLog(LOG_WARNING, "SPRAT: [%s] Failed to load sheet: path too long", fileName);

        UnloadFileData(fileData);
    }

    return sheet;
}

// Load sheet from memory, atlases are loaded from atlasDir (NULL: paths as written)
SpratSheet LoadSpratSheetFromMemory(const unsigned char *fileData, int dataSize, const char *atlasDir)
{
    SpratSheet sheet = { 0 };

    if ((fileData == NULL) || (dataSize <= 0)) TraceLog(LOG_WARNING, "SPRAT: Failed to load sheet: no data");
    else sheet = SpratLoadSheetData(fileData, dataSize, atlasDir, "memory");

    return sheet;
}

// Check if a sheet is valid (metadata loaded and every atlas texture in GPU)
bool IsSpratSheetValid(SpratSheet sheet)
{
    if (sheet.frames == NULL) return false;     // frames holds the sheet's memory, even when frameCount is 0

    for (int i = 0; i < sheet.atlasCount; i++)
    {
        if (!IsTextureValid(sheet.atlases[i])) return false;
    }

    return true;
}

// Unload sheet metadata and atlas textures
void UnloadSpratSheet(SpratSheet sheet)
{
    for (int i = 0; i < sheet.atlasCount; i++)
    {
        if (sheet.atlases[i].id > 0) UnloadTexture(sheet.atlases[i]);
    }

    // Every array of the sheet lives in one allocation, starting at frames
    RL_FREE(sheet.frames);

    if (sheet.frames != NULL) TraceLog(LOG_INFO, "SPRAT: Unloaded sheet data from RAM and VRAM");
}

// Get frame index by sprite path, or by name when it is unique, -1 if not found
// NOTE: Names are file basenames and repeat across folders, while paths are unique
int GetSpratFrameIndex(SpratSheet sheet, const char *pathOrName)
{
    if (pathOrName == NULL) return -1;

    int index = -1;
    int nameMatches = 0;

    for (int i = 0; i < sheet.frameCount; i++)
    {
        if (strcmp(sheet.frames[i].path, pathOrName) == 0) return i;

        if (strcmp(sheet.frames[i].name, pathOrName) == 0)
        {
            if (nameMatches == 0) index = i;
            nameMatches++;
        }
    }

    if (nameMatches == 0) TraceLog(LOG_WARNING, "SPRAT: Failed to find frame: %s", pathOrName);
    else if (nameMatches > 1)
    {
        TraceLog(LOG_WARNING, "SPRAT: Frame name is ambiguous, %i frames are named %s: use the sprite path instead", nameMatches, pathOrName);
        index = -1;
    }

    return index;
}

// Get animation index by name, -1 if not found
int GetSpratAnimationIndex(SpratSheet sheet, const char *name)
{
    if (name == NULL) return -1;

    for (int i = 0; i < sheet.animationCount; i++)
    {
        if (strcmp(sheet.animations[i].name, name) == 0) return i;
    }

    TraceLog(LOG_WARNING, "SPRAT: Failed to find animation: %s", name);

    return -1;
}

// Get frame marker by name, NULL if not found
const SpratMarker *GetSpratFrameMarker(SpratSheet sheet, int frame, const char *name)
{
    if ((frame < 0) || (frame >= sheet.frameCount) || (name == NULL)) return NULL;

    const SpratFrame *f = &sheet.frames[frame];

    for (int i = 0; i < f->markerCount; i++)
    {
        if (strcmp(f->markers[i].name, name) == 0) return &f->markers[i];
    }

    return NULL;
}

// Get frame index shown after time seconds of looped playback, -1 if none
int GetSpratAnimationFrame(SpratSheet sheet, int animation, float time)
{
    if ((animation < 0) || (animation >= sheet.animationCount)) return -1;

    const SpratAnimation *anim = &sheet.animations[animation];

    if (anim->frameCount <= 0) return -1;

    // fmod() keeps the step exact and in range for any time, negative included
    double step = fmod(floor((double)time*anim->fps), (double)anim->frameCount);
    if (!isfinite(step)) return -1;     // NaN or infinite time: (int)step would be undefined
    if (step < 0.0) step += anim->frameCount;

    return anim->frames[(int)step];
}

// Get screen position of an original-image point, drawn as DrawSpratFramePro() would
// NOTE: Use it to place markers, hitboxes or attachments on a drawn frame
Vector2 GetSpratFramePoint(SpratSheet sheet, int frame, Vector2 point, Vector2 position, Vector2 scale, float rotation, int flip)
{
    if ((frame < 0) || (frame >= sheet.frameCount)) return position;

    return SpratTransformPoint(sheet.frames[frame].pivot, point, position, scale, rotation, flip);
}

// Draw a frame with its pivot at position
void DrawSpratFrame(SpratSheet sheet, int frame, Vector2 position, Color tint)
{
    DrawSpratFramePro(sheet, frame, position, (Vector2){ 1.0f, 1.0f }, 0.0f, 0, tint);
}

// Draw a frame with rotation and scale around its pivot
void DrawSpratFrameEx(SpratSheet sheet, int frame, Vector2 position, float rotation, float scale, Color tint)
{
    DrawSpratFramePro(sheet, frame, position, (Vector2){ scale, scale }, rotation, 0, tint);
}

// Draw a frame with scale, rotation and flip around its pivot
void DrawSpratFramePro(SpratSheet sheet, int frame, Vector2 position, Vector2 scale, float rotation, int flip, Color tint)
{
    if (!SpratIsFrameDrawable(sheet, frame)) return;

    const SpratFrame *f = &sheet.frames[frame];
    SpratDrawCall call = SpratGetFrameDrawCall(f, position, scale, rotation, flip);

    DrawTexturePro(sheet.atlases[f->atlas], call.source, call.dest, call.origin, call.rotation, tint);
}

// Draw a frame stretched to dest, keeping its nine-slice borders unscaled
// NOTE: dest covers the whole original image, trimmed margins included, and the pivot is ignored.
// Borders shrink proportionally when dest is smaller than them, as with DrawTextureNPatch().
// Every fill mode is drawn as SPRAT_SLICE_MODE_STRETCH; a frame without slice is simply stretched
void DrawSpratFrameNPatch(SpratSheet sheet, int frame, Rectangle dest, Color tint)
{
    if (!SpratIsFrameDrawable(sheet, frame)) return;

    const SpratFrame *f = &sheet.frames[frame];
    SpratDrawCall calls[9];
    int count = SpratGetNPatchDrawCalls(f, dest, calls);

    for (int i = 0; i < count; i++)
    {
        DrawTexturePro(sheet.atlases[f->atlas], calls[i].source, calls[i].dest, calls[i].origin, calls[i].rotation, tint);
    }
}

// Draw the animation frame shown after time seconds, with the animation's flip
void DrawSpratAnimation(SpratSheet sheet, int animation, float time, Vector2 position, Color tint)
{
    int frame = GetSpratAnimationFrame(sheet, animation, time);

    if (frame >= 0) DrawSpratFramePro(sheet, frame, position, (Vector2){ 1.0f, 1.0f }, 0.0f, sheet.animations[animation].flip, tint);
}

//----------------------------------------------------------------------------------
// Module Internal Functions Definition
//----------------------------------------------------------------------------------

// Parse sheet data and load its atlas textures
static SpratSheet SpratLoadSheetData(const unsigned char *fileData, int dataSize, const char *atlasDir, const char *fileName)
{
    SpratSheet sheet = { 0 };
    sprat_sheet *raw = NULL;
    sprat_error error = { 0 };

    if (sprat_sheet_load_memory((const char *)fileData, (size_t)dataSize, &raw, &error) != SPRAT_OK)
    {
        if (error.line > 0) TraceLog(LOG_WARNING, "SPRAT: [%s] Failed to load sheet: %i:%i: %s", fileName, (int)error.line, (int)error.column, error.message);
        else TraceLog(LOG_WARNING, "SPRAT: [%s] Failed to load sheet: %s", fileName, error.message);
        return sheet;
    }

    sheet = SpratConvertSheet(raw);

    if (sheet.frames == NULL) TraceLog(LOG_WARNING, "SPRAT: [%s] Failed to load sheet: out of memory", fileName);
    else
    {
        for (int i = 0; i < sheet.atlasCount; i++)
        {
            char atlasPath[SPRAT_MAX_PATH_LENGTH] = { 0 };

            // Atlas paths are usually relative to the sheet's folder
            if (SpratJoinPath(atlasPath, (int)sizeof(atlasPath), atlasDir, raw->atlases[i].path)) sheet.atlases[i] = LoadTexture(atlasPath);
            else TraceLog(LOG_WARNING, "SPRAT: [%s] Failed to load atlas %i: path too long", fileName, i);
        }

        TraceLog(LOG_INFO, "SPRAT: [%s] Sheet loaded successfully (%i atlases, %i frames, %i animations)", fileName, sheet.atlasCount, sheet.frameCount, sheet.animationCount);
    }

    sprat_sheet_free(raw);

    return sheet;
}

// Round a section size up so the next section stays aligned for any member
static size_t SpratAlign(size_t size)
{
    return (size + 15) & ~(size_t)15;
}

// Add count items of itemSize bytes to a size, as one aligned section when align is true
// NOTE: Returns false instead of wrapping around, which on a 32-bit size_t would undersize the block
static bool SpratAddSize(size_t *total, size_t count, size_t itemSize, bool align)
{
    if ((itemSize > 0) && (count > (SIZE_MAX - 15)/itemSize)) return false;

    size_t size = count*itemSize;
    if (align) size = SpratAlign(size);

    if (size > SIZE_MAX - *total) return false;

    *total += size;
    return true;
}

// Copy a string into the sheet's string pool
static const char *SpratCopyString(char **pool, const char *text)
{
    size_t length = strlen(text) + 1;
    char *copy = *pool;

    memcpy(copy, text, length);
    *pool += length;

    return copy;
}

// Copy a loaded sprat_sheet into a SpratSheet, without textures
// NOTE: Every array goes in a single allocation that starts at frames, which is never NULL on success.
// Copies hold no pointer into raw, so it can be freed right after
static SpratSheet SpratConvertSheet(const sprat_sheet *raw)
{
    SpratSheet sheet = { 0 };

    // Counts: the loader bounds them by the input size, but a SpratSheet counts with int.
    // Every sum is checked, so a hostile file can't wrap a size around and undersize the block
    size_t markerTotal = 0;
    size_t vertexTotal = 0;
    size_t stringTotal = 0;
    bool fits = true;

    for (size_t i = 0; fits && (i < raw->sprite_count); i++)
    {
        const sprat_sprite *sprite = &raw->sprites[i];

        fits = SpratAddSize(&markerTotal, sprite->marker_count, 1, false) &&
               SpratAddSize(&stringTotal, strlen(sprite->name) + strlen(sprite->path) + 2, 1, false);

        for (size_t m = 0; fits && (m < sprite->marker_count); m++)
        {
            fits = SpratAddSize(&vertexTotal, sprite->markers[m].vertex_count, 1, false) &&
                   SpratAddSize(&stringTotal, strlen(sprite->markers[m].name) + 1, 1, false);
        }
    }

    // Frame indexes of regular animations are slices of one array of the sprat sheet, and an alias
    // shares its source's slice: copy that array once, so aliases can't multiply its size
    const size_t *frameBase = NULL;
    size_t indexTotal = 0;

    for (size_t i = 0; fits && (i < raw->animation_count); i++)
    {
        const sprat_animation *src = &raw->animations[i];

        fits = SpratAddSize(&stringTotal, strlen(src->name) + 1, 1, false);

        if ((src->alias_of == NULL) && (src->frame_count > 0))
        {
            fits = fits && SpratAddSize(&indexTotal, src->frame_count, 1, false);
            if ((frameBase == NULL) || ((uintptr_t)src->frames < (uintptr_t)frameBase)) frameBase = src->frames;
        }
    }

    if (!fits || (raw->atlas_count > INT_MAX) || (raw->sprite_count > INT_MAX) || (raw->animation_count > INT_MAX) ||
        (markerTotal > INT_MAX) || (vertexTotal > INT_MAX) || (indexTotal > INT_MAX)) return sheet;

    size_t framesSize = 0, animationsSize = 0, markersSize = 0, atlasesSize = 0, verticesSize = 0, indexesSize = 0;
    size_t totalSize = 1;   // At least one byte, so frames is never NULL

    if (!SpratAddSize(&framesSize, raw->sprite_count, sizeof(SpratFrame), true) ||
        !SpratAddSize(&animationsSize, raw->animation_count, sizeof(SpratAnimation), true) ||
        !SpratAddSize(&markersSize, markerTotal, sizeof(SpratMarker), true) ||
        !SpratAddSize(&atlasesSize, raw->atlas_count, sizeof(Texture2D), true) ||
        !SpratAddSize(&verticesSize, vertexTotal, sizeof(Vector2), true) ||
        !SpratAddSize(&indexesSize, indexTotal, sizeof(int), true) ||
        !SpratAddSize(&totalSize, framesSize, 1, false) ||
        !SpratAddSize(&totalSize, animationsSize, 1, false) ||
        !SpratAddSize(&totalSize, markersSize, 1, false) ||
        !SpratAddSize(&totalSize, atlasesSize, 1, false) ||
        !SpratAddSize(&totalSize, verticesSize, 1, false) ||
        !SpratAddSize(&totalSize, indexesSize, 1, false) ||
        !SpratAddSize(&totalSize, stringTotal, 1, false)) return sheet;

    unsigned char *block = (unsigned char *)RL_CALLOC(totalSize, 1);
    if (block == NULL) return sheet;

    unsigned char *section = block;
    SpratFrame *frames = (SpratFrame *)section;         section += framesSize;
    SpratAnimation *animations = (SpratAnimation *)section; section += animationsSize;
    SpratMarker *markers = (SpratMarker *)section;      section += markersSize;
    Texture2D *atlases = (Texture2D *)section;          section += atlasesSize;
    Vector2 *vertices = (Vector2 *)section;             section += verticesSize;
    int *indexes = (int *)section;                      section += indexesSize;
    char *strings = (char *)section;

    for (size_t i = 0; i < raw->sprite_count; i++)
    {
        const sprat_sprite *sprite = &raw->sprites[i];
        SpratFrame *frame = &frames[i];
        sprat_size content = sprat_sprite_content_size(sprite);
        sprat_size size = sprat_sprite_source_size(sprite);

        frame->name = SpratCopyString(&strings, sprite->name);
        frame->path = SpratCopyString(&strings, sprite->path);
        frame->atlas = (int)sprite->atlas_index;
        frame->source = (Rectangle){ (float)sprite->rect.x, (float)sprite->rect.y, (float)sprite->rect.w, (float)sprite->rect.h };
        frame->rotation = sprite->rotation;
        frame->size = (Vector2){ (float)size.w, (float)size.h };
        frame->content = (Rectangle){ (float)sprite->trim.left, (float)sprite->trim.top, (float)content.w, (float)content.h };
        frame->pivot = (Vector2){ (float)sprite->pivot.x, (float)sprite->pivot.y };

        if (sprite->has_slice)
        {
            frame->slice.left = (float)sprite->slice.left;
            frame->slice.top = (float)sprite->slice.top;
            frame->slice.right = (float)sprite->slice.right;
            frame->slice.bottom = (float)sprite->slice.bottom;
            frame->slice.horizontalMode = (int)sprite->slice.h_mode;
            frame->slice.verticalMode = (int)sprite->slice.v_mode;
        }

        frame->markers = (sprite->marker_count > 0)? markers : NULL;
        frame->markerCount = (int)sprite->marker_count;

        for (size_t m = 0; m < sprite->marker_count; m++)
        {
            const sprat_marker *src = &sprite->markers[m];
            SpratMarker *marker = markers++;

            marker->name = SpratCopyString(&strings, src->name);
            marker->type = (int)src->type;
            marker->position = (Vector2){ (float)src->x, (float)src->y };
            marker->radius = (float)src->radius;
            marker->size = (Vector2){ (float)src->w, (float)src->h };
            marker->vertices = (src->vertex_count > 0)? vertices : NULL;
            marker->vertexCount = (int)src->vertex_count;

            for (size_t v = 0; v < src->vertex_count; v++)
            {
                *vertices++ = (Vector2){ (float)src->vertices[v].x, (float)src->vertices[v].y };
            }
        }
    }

    for (size_t i = 0; i < raw->animation_count; i++)
    {
        const sprat_animation *src = &raw->animations[i];
        SpratAnimation *anim = &animations[i];

        anim->name = SpratCopyString(&strings, src->name);
        anim->fps = src->fps;
        anim->frameCount = (int)src->frame_count;
        anim->flip = (int)src->flip;     // SPRAT_FLIP_H/V have the values of SPRAT_FLIP_HORIZONTAL/VERTICAL

        if (src->frame_count == 0) continue;

        // Same slice of the copy as of the sprat sheet's array, checked to lie inside the copy
        size_t offset = ((uintptr_t)src->frames - (uintptr_t)frameBase)/sizeof(size_t);

        if (((uintptr_t)src->frames < (uintptr_t)frameBase) || (offset > indexTotal) || (src->frame_count > indexTotal - offset))
        {
            RL_FREE(block);
            return sheet;
        }

        anim->frames = indexes + offset;

        if (src->alias_of == NULL)
        {
            for (size_t f = 0; f < src->frame_count; f++) anim->frames[f] = (int)src->frames[f];
        }
    }

    sheet.atlasCount = (int)raw->atlas_count;
    sheet.atlases = (raw->atlas_count > 0)? atlases : NULL;
    sheet.frameCount = (int)raw->sprite_count;
    sheet.frames = frames;
    sheet.animationCount = (int)raw->animation_count;
    sheet.animations = (raw->animation_count > 0)? animations : NULL;
    sheet.scale = (float)raw->scale;

    return sheet;
}

// Join an atlas path with the folder it is relative to
// NOTE: Absolute paths, and any path when dir is NULL, empty or ".", are kept as written
static bool SpratJoinPath(char *buffer, int bufferSize, const char *dir, const char *path)
{
    bool absolute = (path[0] == '/') || (path[0] == '\\') ||
                    (((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')) && (path[1] == ':'));
    size_t pathLength = strlen(path);

    if (absolute || (dir == NULL) || (dir[0] == '\0') || (strcmp(dir, ".") == 0))
    {
        if (pathLength >= (size_t)bufferSize) return false;
        memcpy(buffer, path, pathLength + 1);
        return true;
    }

    size_t dirLength = strlen(dir);
    bool separator = (dir[dirLength - 1] == '/') || (dir[dirLength - 1] == '\\');
    size_t length = dirLength + (separator? 0 : 1) + pathLength;

    if (length >= (size_t)bufferSize) return false;

    memcpy(buffer, dir, dirLength);
    if (!separator) buffer[dirLength++] = '/';
    memcpy(buffer + dirLength, path, pathLength + 1);

    return true;
}

// Check frame index and its atlas texture
static bool SpratIsFrameDrawable(SpratSheet sheet, int frame)
{
    if ((frame < 0) || (frame >= sheet.frameCount)) return false;

    int atlas = sheet.frames[frame].atlas;

    return (atlas >= 0) && (atlas < sheet.atlasCount) && (sheet.atlases[atlas].id > 0);
}

// Transform an original-image point: scaled, then flipped around the pivot, then rotated
// clockwise around it, with the pivot moved to position
static Vector2 SpratTransformPoint(Vector2 pivot, Vector2 point, Vector2 position, Vector2 scale, float rotation, int flip)
{
    float x = (point.x - pivot.x)*scale.x;
    float y = (point.y - pivot.y)*scale.y;

    if (flip & SPRAT_FLIP_HORIZONTAL) x = -x;
    if (flip & SPRAT_FLIP_VERTICAL) y = -y;

    if (rotation == 0.0f) return (Vector2){ position.x + x, position.y + y };   // The common case, without trigonometry

    float cosRes = cosf(rotation*DEG2RAD);
    float sinRes = sinf(rotation*DEG2RAD);

    return (Vector2){ position.x + x*cosRes - y*sinRes, position.y + x*sinRes + y*cosRes };
}

// Get the atlas region that stores the upright content region (u0, v0)-(u1, v1)
// NOTE: Packed clockwise (90), content (u, v) is stored at (height - v, u);
// packed counter-clockwise (270), at (v, width - u)
static Rectangle SpratGetStoredRegion(const SpratFrame *frame, float u0, float v0, float u1, float v1)
{
    if (frame->rotation == 90) return (Rectangle){ frame->source.x + frame->content.height - v1, frame->source.y + u0, v1 - v0, u1 - u0 };
    if (frame->rotation == 270) return (Rectangle){ frame->source.x + v0, frame->source.y + frame->content.width - u1, v1 - v0, u1 - u0 };

    return (Rectangle){ frame->source.x + u0, frame->source.y + v0, u1 - u0, v1 - v0 };
}

// Get the DrawTexturePro() arguments that draw a stored region upright, size wide and high,
// centered on center, then flipped and rotated clockwise around its center
// NOTE: A region is stored turned by frame.rotation, so its box is turned back by the opposite,
// and raylib flips the stored image before turning it, so a turned region swaps its flip axes
static SpratDrawCall SpratGetRegionDrawCall(const SpratFrame *frame, Rectangle source, Vector2 center, Vector2 size, float rotation, int flip)
{
    bool turned = (frame->rotation != 0);
    float width = turned? size.y : size.x;      // Stored box: a turned region's width is its upright height
    float height = turned? size.x : size.y;
    int sourceFlip = !turned? flip : ((flip & SPRAT_FLIP_HORIZONTAL)? SPRAT_FLIP_VERTICAL : 0) | ((flip & SPRAT_FLIP_VERTICAL)? SPRAT_FLIP_HORIZONTAL : 0);

    if (sourceFlip & SPRAT_FLIP_HORIZONTAL) source.width = -source.width;
    if (sourceFlip & SPRAT_FLIP_VERTICAL) source.height = -source.height;

    SpratDrawCall call = {
        source,
        (Rectangle){ center.x, center.y, width, height },
        (Vector2){ width/2.0f, height/2.0f },
        rotation - (float)frame->rotation
    };

    return call;
}

// Get the DrawTexturePro() arguments that draw a frame with its pivot at position
static SpratDrawCall SpratGetFrameDrawCall(const SpratFrame *frame, Vector2 position, Vector2 scale, float rotation, int flip)
{
    Vector2 contentCenter = { frame->content.x + frame->content.width/2.0f, frame->content.y + frame->content.height/2.0f };
    Vector2 center = SpratTransformPoint(frame->pivot, contentCenter, position, scale, rotation, flip);
    Vector2 size = { frame->content.width*scale.x, frame->content.height*scale.y };

    return SpratGetRegionDrawCall(frame, frame->source, center, size, rotation, flip);
}

// Get the edges of the three slices along one axis of dest
static void SpratGetSliceEdges(float start, float length, float before, float after, float edges[4])
{
    // Borders shrink proportionally when they don't fit, as in DrawTextureNPatch()
    if (before + after > length)
    {
        float ratio = length/(before + after);
        before *= ratio;
        after *= ratio;
    }

    edges[0] = start;
    edges[1] = start + before;
    edges[2] = start + length - after;
    edges[3] = start + length;
}

// Get the DrawTexturePro() arguments that draw a frame as a nine-slice into dest
// NOTE: Slices are cut in original-image space, then each is clipped to the trimmed content
// and mapped to the atlas region, which may be stored turned
static int SpratGetNPatchDrawCalls(const SpratFrame *frame, Rectangle dest, SpratDrawCall calls[9])
{
    int count = 0;

    if ((frame->size.x <= 0.0f) || (frame->size.y <= 0.0f) || (dest.width <= 0.0f) || (dest.height <= 0.0f)) return 0;

    SpratSlice slice = frame->slice;
    float sourceX[4] = { 0.0f, slice.left, frame->size.x - slice.right, frame->size.x };
    float sourceY[4] = { 0.0f, slice.top, frame->size.y - slice.bottom, frame->size.y };
    float destX[4] = { 0 };
    float destY[4] = { 0 };

    SpratGetSliceEdges(dest.x, dest.width, slice.left, slice.right, destX);
    SpratGetSliceEdges(dest.y, dest.height, slice.top, slice.bottom, destY);

    float contentRight = frame->content.x + frame->content.width;
    float contentBottom = frame->content.y + frame->content.height;

    for (int row = 0; row < 3; row++)
    {
        for (int col = 0; col < 3; col++)
        {
            float x0 = sourceX[col], x1 = sourceX[col + 1];
            float y0 = sourceY[row], y1 = sourceY[row + 1];

            if ((x1 <= x0) || (y1 <= y0)) continue;
            if ((destX[col + 1] <= destX[col]) || (destY[row + 1] <= destY[row])) continue;

            // Clip the slice to the content left after trimming
            float clipX0 = (x0 > frame->content.x)? x0 : frame->content.x;
            float clipX1 = (x1 < contentRight)? x1 : contentRight;
            float clipY0 = (y0 > frame->content.y)? y0 : frame->content.y;
            float clipY1 = (y1 < contentBottom)? y1 : contentBottom;

            if ((clipX1 <= clipX0) || (clipY1 <= clipY0)) continue;

            float scaleX = (destX[col + 1] - destX[col])/(x1 - x0);
            float scaleY = (destY[row + 1] - destY[row])/(y1 - y0);
            Rectangle part = {
                destX[col] + (clipX0 - x0)*scaleX,
                destY[row] + (clipY0 - y0)*scaleY,
                (clipX1 - clipX0)*scaleX,
                (clipY1 - clipY0)*scaleY
            };

            // Region in upright content coordinates
            Rectangle source = SpratGetStoredRegion(frame, clipX0 - frame->content.x, clipY0 - frame->content.y,
                                                    clipX1 - frame->content.x, clipY1 - frame->content.y);
            Vector2 center = { part.x + part.width/2.0f, part.y + part.height/2.0f };

            calls[count++] = SpratGetRegionDrawCall(frame, source, center, (Vector2){ part.width, part.height }, 0.0f, 0);
        }
    }

    return count;
}

#endif // SPRAT_RAYLIB_IMPLEMENTATION
