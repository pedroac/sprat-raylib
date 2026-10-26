/**********************************************************************************************
*
*   sprat-raylib v0.3.0 - Load and draw sprat-cli sprite sheets with raylib
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
*           SpratAnimationPlayer run = MakeSpratAnimationPlayer(GetSpratAnimation(sheet, "run"));
*           ...
*           UpdateSpratAnimation(&run, GetFrameTime());
*           DrawSpratAnimation(sheet, run, (Vector2){ 400, 300 }, WHITE);
*           ...
*           UnloadSpratSheet(sheet);
*
*       A SpratSprite is a frame placed on the screen: drawn there, and its markers placed
*       there, so what is drawn and what collides always agree.
*
*           SpratSprite hero = MakeSpratSprite(sheet, GetSpratAnimationFrame(run), position);
*           const SpratMarker *hitbox = GetSpratSpriteMarker(hero, "hitbox");
*           if (CheckCollisionSpratMarkers(hero, hitbox, coin, coinHitbox)) ...
*           DrawSpratSprite(hero, WHITE);
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

#define SPRAT_RAYLIB_VERSION    "0.3.0"

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
// NOTE: A marker means something once placed with a sprite of its frame: see SpratSprite
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

// Playback of an animation: keep one per animated thing and update it every frame
typedef struct SpratAnimationPlayer {
    SpratAnimation animation;   // Animation played (shares the sheet's frame array)
    float time;                 // Seconds into the animation, from 0 to its duration (frameCount/fps)
    float speed;                // Playback rate: 1.0f plays at the animation's fps, negative plays in reverse
    bool playing;               // Advanced by UpdateSpratAnimation()
    bool loop;                  // Wrap around at the end, otherwise stop on the last frame
} SpratAnimationPlayer;

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

// Frame placed on the screen: scaled, then flipped, then rotated clockwise around its pivot
// NOTE: A plain struct owned by the program: change its fields directly, such as frame from
// GetSpratAnimationFrame() every update. A scale of 1.0f draws atlas pixels one to one
typedef struct SpratSprite {
    SpratSheet sheet;           // Sheet the frame belongs to (shared, not owned: unload the sheet once)
    int frame;                  // Frame index in sheet.frames
    Vector2 position;           // Screen position of the frame's pivot
    Vector2 scale;              // Scale around the pivot, negative to mirror
    float rotation;             // Clockwise rotation around the pivot, in degrees
    int flip;                   // Flip flags around the pivot (SpratFlip)
} SpratSprite;

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
SPRATAPI const SpratMarker *GetSpratFrameMarker(SpratSheet sheet, int frame, const char *name); // Get frame marker by name, NULL if not found

// Sheet drawing functions
SPRATAPI void DrawSpratFrame(SpratSheet sheet, int frame, Vector2 position, Color tint); // Draw a frame with its pivot at position
SPRATAPI void DrawSpratFrameEx(SpratSheet sheet, int frame, Vector2 position, float rotation, float scale, Color tint); // Draw a frame with rotation and scale around its pivot
SPRATAPI void DrawSpratFrameNPatch(SpratSheet sheet, int frame, Rectangle dest, Color tint); // Draw a frame stretched to dest, keeping its nine-slice borders unscaled

// Sprite functions
SPRATAPI SpratSprite MakeSpratSprite(SpratSheet sheet, int frame, Vector2 position); // Make a sprite of a frame with its pivot at position, unscaled, unrotated and unflipped
SPRATAPI void DrawSpratSprite(SpratSprite sprite, Color tint);                 // Draw a sprite: its frame scaled, flipped and rotated around the pivot at its position
SPRATAPI Vector2 GetSpratSpritePoint(SpratSprite sprite, Vector2 point);       // Get screen position of an original-image point of the sprite's frame
SPRATAPI const SpratMarker *GetSpratSpriteMarker(SpratSprite sprite, const char *name); // Get marker of the sprite's frame by name, NULL if not found

// Marker functions (markers in the original-image pixels of the sprite's frame, placed as the sprite is drawn)
SPRATAPI Vector2 GetSpratMarkerCenter(SpratSprite sprite, const SpratMarker *marker); // Get screen position of a marker's point, or of its shape's center
SPRATAPI int GetSpratMarkerOutline(SpratSprite sprite, const SpratMarker *marker, Vector2 *points, int maxPoints); // Get a marker's closed outline in screen coordinates, returns its point count
SPRATAPI void DrawSpratMarker(SpratSprite sprite, const SpratMarker *marker, Color color); // Draw a marker's outline (a cross for a point)
SPRATAPI bool CheckCollisionSpratMarkers(SpratSprite spriteA, const SpratMarker *markerA, SpratSprite spriteB, const SpratMarker *markerB); // Check collision between markers of two sprites
SPRATAPI bool CheckCollisionPointSpratMarker(Vector2 point, SpratSprite sprite, const SpratMarker *marker); // Check if a screen point is inside a sprite's marker

// Animation functions
SPRATAPI SpratAnimation GetSpratAnimation(SpratSheet sheet, const char *name); // Get animation by name, zeroed if not found
SPRATAPI bool IsSpratAnimationValid(SpratAnimation animation);                 // Check if an animation is valid (found in a sheet; it may have no frames)
SPRATAPI SpratAnimationPlayer MakeSpratAnimationPlayer(SpratAnimation animation); // Make a player for an animation: playing, looping, at speed 1.0f
SPRATAPI void UpdateSpratAnimation(SpratAnimationPlayer *player, float deltaTime); // Advance a playing animation by deltaTime seconds, scaled by its speed
SPRATAPI void PlaySpratAnimation(SpratAnimationPlayer *player);                // Play an animation from its start (its end when the speed is negative)
SPRATAPI void StopSpratAnimation(SpratAnimationPlayer *player);                // Stop an animation and rewind it to where PlaySpratAnimation() starts
SPRATAPI void PauseSpratAnimation(SpratAnimationPlayer *player);               // Pause an animation, keeping its time
SPRATAPI void ResumeSpratAnimation(SpratAnimationPlayer *player);              // Resume a paused animation
SPRATAPI void SetSpratAnimationSpeed(SpratAnimationPlayer *player, float speed); // Set playback rate (1.0f is the animation's fps, negative plays in reverse)
SPRATAPI bool IsSpratAnimationPlaying(SpratAnimationPlayer player);            // Check if an animation is playing
SPRATAPI bool IsSpratAnimationFinished(SpratAnimationPlayer player);           // Check if a non-looping animation reached its end in the direction it plays
SPRATAPI int GetSpratAnimationFrame(SpratAnimationPlayer player);              // Get frame index shown now (an index into sheet.frames), -1 if none
SPRATAPI void DrawSpratAnimation(SpratSheet sheet, SpratAnimationPlayer player, Vector2 position, Color tint); // Draw the frame shown now, with the animation's flip (use a SpratSprite to scale or rotate it)

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
#include <math.h>           // Required for: sin(), cos(), acos(), atan2(), sqrt(), floor(), ceil(), fmod(), isfinite()
#include <stdint.h>         // Required for: SIZE_MAX, uintptr_t
#include <stdlib.h>         // Required for: calloc(), free() [Used in RL_CALLOC(), RL_FREE()]
#include <string.h>         // Required for: strlen(), strcmp(), memcpy()

#ifndef SPRAT_MAX_PATH_LENGTH
    #define SPRAT_MAX_PATH_LENGTH   4096    // Maximum length of an atlas path, joined with its folder
#endif

#define SPRAT_PI_D                  3.14159265358979323846
#define SPRAT_OUTLINE_TOLERANCE     0.25    // Distance a circle outline may stray from the curve, in screen pixels
#define SPRAT_MAX_CIRCLE_SIDES      256     // Maximum sides of a circle outline
#define SPRAT_UNIT_TOLERANCE        1e-9    // Distance past touching, in an ellipse's unit space, that still counts as touching

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

// Point in double precision, for marker geometry
typedef struct SpratPoint {
    double x;
    double y;
} SpratPoint;

// Placement of original-image points: position + m*(point - pivot)
// NOTE: m scales, then flips, then rotates clockwise: the same transform the frame is drawn with
typedef struct SpratPlacement {
    double m[4];
    SpratPoint pivot;
    SpratPoint position;
} SpratPlacement;

// Marker placed on the screen: a polygon (a point or a segment with one or two vertices),
// or an ellipse, the image of the unit disk by center + l
typedef struct SpratShape {
    bool circle;                // Placed from a circle marker: center and l are set, even when flattened
    bool ellipse;
    SpratPoint center;
    double l[4];
    int vertexCount;
    SpratPoint points[4];       // Vertices of a point, a rectangle or a flattened ellipse
    const Vector2 *vertices;    // Or vertices of a polygon marker, placed by placement
    SpratPlacement placement;
} SpratShape;

// Where shapes are compared: the screen, or the unit space of an ellipse, where it is the unit disk
// NOTE: A screen point p is at inverse*(p - origin)
typedef struct SpratSpace {
    SpratPoint origin;
    double inverse[4];
} SpratSpace;

//----------------------------------------------------------------------------------
// Module Internal Functions Declaration
//----------------------------------------------------------------------------------
static SpratSheet SpratLoadSheetData(const unsigned char *fileData, int dataSize, const char *atlasDir, const char *fileName);
static SpratSheet SpratConvertSheet(const sprat_sheet *raw);
static bool SpratJoinPath(char *buffer, int bufferSize, const char *dir, const char *path);
static bool SpratIsFrameDrawable(SpratSheet sheet, int frame);
static float SpratGetAnimationDuration(SpratAnimation animation);
static SpratPlacement SpratGetPlacement(Vector2 pivot, Vector2 position, Vector2 scale, float rotation, int flip);
static SpratPoint SpratPlacePoint(const SpratPlacement *placement, double x, double y);
static bool SpratGetSpritePlacement(SpratSprite sprite, SpratPlacement *placement);
static bool SpratGetMarkerShape(SpratSprite sprite, const SpratMarker *marker, SpratShape *shape);
static SpratPoint SpratGetScreenVertex(const SpratShape *shape, int i);
static bool SpratShapesOverlap(const SpratShape *a, const SpratShape *b);
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

// Draw a frame with its pivot at position
void DrawSpratFrame(SpratSheet sheet, int frame, Vector2 position, Color tint)
{
    DrawSpratSprite(MakeSpratSprite(sheet, frame, position), tint);
}

// Draw a frame with rotation and scale around its pivot
void DrawSpratFrameEx(SpratSheet sheet, int frame, Vector2 position, float rotation, float scale, Color tint)
{
    SpratSprite sprite = MakeSpratSprite(sheet, frame, position);

    sprite.rotation = rotation;
    sprite.scale = (Vector2){ scale, scale };

    DrawSpratSprite(sprite, tint);
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

// Make a sprite of a frame with its pivot at position, unscaled, unrotated and unflipped
SpratSprite MakeSpratSprite(SpratSheet sheet, int frame, Vector2 position)
{
    SpratSprite sprite = { 0 };

    sprite.sheet = sheet;
    sprite.frame = frame;
    sprite.position = position;
    sprite.scale = (Vector2){ 1.0f, 1.0f };

    return sprite;
}

// Draw a sprite: its frame scaled, flipped and rotated around the pivot at its position
// NOTE: Trimmed margins are left out, and frames packed turned are drawn upright
void DrawSpratSprite(SpratSprite sprite, Color tint)
{
    if (!SpratIsFrameDrawable(sprite.sheet, sprite.frame)) return;

    const SpratFrame *f = &sprite.sheet.frames[sprite.frame];
    SpratDrawCall call = SpratGetFrameDrawCall(f, sprite.position, sprite.scale, sprite.rotation, sprite.flip);

    DrawTexturePro(sprite.sheet.atlases[f->atlas], call.source, call.dest, call.origin, call.rotation, tint);
}

// Get screen position of an original-image point of the sprite's frame
// NOTE: Placed as DrawSpratSprite() places the frame; for an out-of-range frame, returns the sprite's position
Vector2 GetSpratSpritePoint(SpratSprite sprite, Vector2 point)
{
    SpratPlacement placement = { 0 };

    if (!SpratGetSpritePlacement(sprite, &placement)) return sprite.position;

    SpratPoint p = SpratPlacePoint(&placement, point.x, point.y);

    return (Vector2){ (float)p.x, (float)p.y };
}

// Get marker of the sprite's frame by name, NULL if not found
// NOTE: Each frame has its own markers: look the marker up again when the sprite's frame changes
const SpratMarker *GetSpratSpriteMarker(SpratSprite sprite, const char *name)
{
    return GetSpratFrameMarker(sprite.sheet, sprite.frame, name);
}

// Get a polygon's center in original-image pixels: its centroid, or the middle of its ends when it has no area
// NOTE: Sums are taken around the first vertex, which keeps them small
static SpratPoint SpratGetPolygonCenter(const Vector2 *vertices, int vertexCount)
{
    double x0 = vertices[0].x, y0 = vertices[0].y;
    double minX = x0, maxX = x0, minY = y0, maxY = y0;
    double area = 0.0, cx = 0.0, cy = 0.0;
    SpratPoint center = { 0 };

    for (int i = 0; i < vertexCount; i++)
    {
        double ax = vertices[i].x - x0, ay = vertices[i].y - y0;
        double bx = vertices[(i + 1)%vertexCount].x - x0, by = vertices[(i + 1)%vertexCount].y - y0;
        double cross = ax*by - bx*ay;

        area += cross;
        cx += (ax + bx)*cross;
        cy += (ay + by)*cross;
        minX = fmin(minX, vertices[i].x);
        maxX = fmax(maxX, vertices[i].x);
        minY = fmin(minY, vertices[i].y);
        maxY = fmax(maxY, vertices[i].y);
    }

    if (area != 0.0)
    {
        center.x = x0 + cx/(3.0*area);
        center.y = y0 + cy/(3.0*area);
    }
    else
    {
        // A point, a segment, or vertices on a line: the middle of its ends
        center.x = (minX + maxX)/2.0;
        center.y = (minY + maxY)/2.0;
    }

    return center;
}

// Get screen position of a marker's point, or of its shape's center
// NOTE: The center of a circle, the middle of a rectangle, a polygon's centroid (outside a concave
// polygon whose middle is cut away). Returns the sprite's position for a NULL or invalid marker
Vector2 GetSpratMarkerCenter(SpratSprite sprite, const SpratMarker *marker)
{
    SpratShape shape = { 0 };
    SpratPoint center = { 0 };

    if (!SpratGetMarkerShape(sprite, marker, &shape)) return sprite.position;

    if (shape.circle) center = shape.center;
    else if (shape.vertices != NULL)
    {
        // Taken unplaced, so a frame scaled to 0 keeps it on the pivot
        SpratPoint centroid = SpratGetPolygonCenter(shape.vertices, shape.vertexCount);
        center = SpratPlacePoint(&shape.placement, centroid.x, centroid.y);
    }
    else
    {
        // A point, or the middle of a rectangle's corners
        for (int i = 0; i < shape.vertexCount; i++)
        {
            center.x += shape.points[i].x;
            center.y += shape.points[i].y;
        }

        center.x /= shape.vertexCount;
        center.y /= shape.vertexCount;
    }

    return (Vector2){ (float)center.x, (float)center.y };
}

// Get the number of sides that keep a circle of this screen radius within the outline tolerance
// NOTE: The middle of each side is radius*(1 - cos(PI/sides)) inside the curve
static int SpratGetCircleSides(double radius)
{
    if (!(radius > SPRAT_OUTLINE_TOLERANCE)) return 8;

    double sides = ceil(SPRAT_PI_D/acos(1.0 - SPRAT_OUTLINE_TOLERANCE/radius));

    if (sides < 8.0) sides = 8.0;
    else if (sides > SPRAT_MAX_CIRCLE_SIDES) sides = SPRAT_MAX_CIRCLE_SIDES;

    return (int)sides;
}

// Get a placed shape's closed outline, returns its point count (filling only maxPoints)
// NOTE: A circle is center + l*(cos, sin), with as many sides as its longest axis needs; a polygon,
// a rectangle or a point is its vertices, closed by the first again unless it is a single point
static int SpratGetShapeOutline(const SpratShape *shape, Vector2 *points, int maxPoints)
{
    int count = 0;

    if (shape->circle)
    {
        const double *l = shape->l;
        int sides = SpratGetCircleSides(fmax(hypot(l[0], l[2]), hypot(l[1], l[3])));

        for (int i = 0; i <= sides; i++, count++)
        {
            double angle = (i < sides)? (2.0*SPRAT_PI_D*i)/sides : 0.0;
            double cosRes = cos(angle), sinRes = sin(angle);

            if (count < maxPoints) points[count] = (Vector2){ (float)(shape->center.x + l[0]*cosRes + l[1]*sinRes), (float)(shape->center.y + l[2]*cosRes + l[3]*sinRes) };
        }
    }
    else
    {
        int closing = (shape->vertexCount > 1)? 1 : 0;

        for (int i = 0; i < shape->vertexCount + closing; i++, count++)
        {
            if (count < maxPoints)
            {
                SpratPoint p = SpratGetScreenVertex(shape, i%shape->vertexCount);
                points[count] = (Vector2){ (float)p.x, (float)p.y };
            }
        }
    }

    return count;
}

// Get a marker's closed outline in screen coordinates, returns its point count
// NOTE: The outline ends on the point it starts from, so DrawLineStrip() draws the whole shape:
// a rectangle's 4 corners from its top-left one, a polygon's vertices, or points around a circle
// (as many as keep it within a quarter of a pixel of the curve, up to 256), each followed by the
// first again. A point marker's outline, and a one-vertex polygon's, is that point.
// Like snprintf(), it returns the full count but fills only maxPoints: pass NULL and 0 to count.
// Returns 0 for a NULL or invalid marker, or an out-of-range frame
int GetSpratMarkerOutline(SpratSprite sprite, const SpratMarker *marker, Vector2 *points, int maxPoints)
{
    SpratShape shape = { 0 };

    if (!SpratGetMarkerShape(sprite, marker, &shape)) return 0;

    return SpratGetShapeOutline(&shape, points, (points != NULL)? maxPoints : 0);
}

// Draw a marker's outline (a cross for a point)
// NOTE: Use it to see hitboxes; it turns, mirrors and stretches with the sprite
void DrawSpratMarker(SpratSprite sprite, const SpratMarker *marker, Color color)
{
    SpratShape shape = { 0 };

    if (!SpratGetMarkerShape(sprite, marker, &shape)) return;

    if (shape.circle)
    {
        Vector2 points[SPRAT_MAX_CIRCLE_SIDES + 1];
        int count = SpratGetShapeOutline(&shape, points, SPRAT_MAX_CIRCLE_SIDES + 1);

        DrawLineStrip(points, count, color);
    }
    else if (shape.vertexCount == 1)
    {
        SpratPoint vertex = SpratGetScreenVertex(&shape, 0);
        Vector2 p = { (float)vertex.x, (float)vertex.y };

        DrawLineV((Vector2){ p.x - 3.0f, p.y }, (Vector2){ p.x + 4.0f, p.y }, color);
        DrawLineV((Vector2){ p.x, p.y - 3.0f }, (Vector2){ p.x, p.y + 4.0f }, color);
    }
    else
    {
        // Edge by edge, so a polygon of any size needs no buffer
        SpratPoint previous = SpratGetScreenVertex(&shape, shape.vertexCount - 1);

        for (int i = 0; i < shape.vertexCount; i++)
        {
            SpratPoint current = SpratGetScreenVertex(&shape, i);
            DrawLineV((Vector2){ (float)previous.x, (float)previous.y }, (Vector2){ (float)current.x, (float)current.y }, color);
            previous = current;
        }
    }
}

// Check collision between markers of two sprites
// NOTE: Shapes are compared exactly, as each sprite places them: a rectangle turns with its frame,
// and a circle scaled unevenly is an ellipse. Shapes that touch collide, and so does a shape inside
// the other; polygons may be concave. A marker need not belong to its sprite's frame: any marker in
// the frame's original-image pixels works. Returns false for a NULL or invalid marker
bool CheckCollisionSpratMarkers(SpratSprite spriteA, const SpratMarker *markerA, SpratSprite spriteB, const SpratMarker *markerB)
{
    SpratShape a = { 0 };
    SpratShape b = { 0 };

    if (!SpratGetMarkerShape(spriteA, markerA, &a) || !SpratGetMarkerShape(spriteB, markerB, &b)) return false;

    return SpratShapesOverlap(&a, &b);
}

// Check if a screen point is inside a sprite's marker
// NOTE: Points on the marker's edge are inside, so a point marker contains only its own point
bool CheckCollisionPointSpratMarker(Vector2 point, SpratSprite sprite, const SpratMarker *marker)
{
    SpratShape a = { 0 };
    SpratShape b = { 0 };

    if (!SpratGetMarkerShape(sprite, marker, &b)) return false;

    a.vertexCount = 1;
    a.points[0] = (SpratPoint){ point.x, point.y };

    return SpratShapesOverlap(&a, &b);
}

// Get animation by name, zeroed if not found
// NOTE: The animation shares the sheet's frame array, so it is valid until UnloadSpratSheet()
SpratAnimation GetSpratAnimation(SpratSheet sheet, const char *name)
{
    SpratAnimation animation = { 0 };

    if (name == NULL) return animation;

    for (int i = 0; i < sheet.animationCount; i++)
    {
        if (strcmp(sheet.animations[i].name, name) == 0) return sheet.animations[i];
    }

    TraceLog(LOG_WARNING, "SPRAT: Failed to find animation: %s", name);

    return animation;
}

// Check if an animation is valid (found in a sheet; it may have no frames)
bool IsSpratAnimationValid(SpratAnimation animation)
{
    return (animation.name != NULL) && (animation.fps > 0) && (animation.frameCount >= 0) &&
           ((animation.frameCount == 0) || (animation.frames != NULL));
}

// Make a player for an animation: playing, looping, at speed 1.0f
// NOTE: A player allocates nothing: copy it, reset it or drop it freely
SpratAnimationPlayer MakeSpratAnimationPlayer(SpratAnimation animation)
{
    SpratAnimationPlayer player = { 0 };

    player.animation = animation;
    player.speed = 1.0f;
    player.playing = true;
    player.loop = true;

    return player;
}

// Advance a playing animation by deltaTime seconds, scaled by its speed
// NOTE: A looping animation's time wraps around, so it stays precise however long it plays.
// A non-looping one stops at its end, or at its start when playing in reverse
void UpdateSpratAnimation(SpratAnimationPlayer *player, float deltaTime)
{
    if ((player == NULL) || !player->playing) return;

    double duration = SpratGetAnimationDuration(player->animation);
    if (duration <= 0.0) return;

    double time = (double)player->time + (double)deltaTime*player->speed;
    if (!isfinite(time)) return;     // NaN or infinite time, step or speed: keep the last good time

    if (player->loop)
    {
        time = fmod(time, duration);
        if (time < 0.0) time += duration;
    }
    else
    {
        if (time > duration) time = duration;
        else if (time < 0.0) time = 0.0;

        if (((player->speed > 0.0f) && (time >= duration)) || ((player->speed < 0.0f) && (time <= 0.0))) player->playing = false;
    }

    player->time = (float)time;
}

// Play an animation from its start (its end when the speed is negative)
void PlaySpratAnimation(SpratAnimationPlayer *player)
{
    if (player == NULL) return;

    player->time = (player->speed < 0.0f)? SpratGetAnimationDuration(player->animation) : 0.0f;
    player->playing = true;
}

// Stop an animation and rewind it to where PlaySpratAnimation() starts
void StopSpratAnimation(SpratAnimationPlayer *player)
{
    if (player == NULL) return;

    PlaySpratAnimation(player);
    player->playing = false;
}

// Pause an animation, keeping its time
void PauseSpratAnimation(SpratAnimationPlayer *player)
{
    if (player != NULL) player->playing = false;
}

// Resume a paused animation
void ResumeSpratAnimation(SpratAnimationPlayer *player)
{
    if (player != NULL) player->playing = true;
}

// Set playback rate (1.0f is the animation's fps, negative plays in reverse)
void SetSpratAnimationSpeed(SpratAnimationPlayer *player, float speed)
{
    if ((player != NULL) && isfinite(speed)) player->speed = speed;
}

// Check if an animation is playing
bool IsSpratAnimationPlaying(SpratAnimationPlayer player)
{
    return player.playing;
}

// Check if a non-looping animation reached its end in the direction it plays
// NOTE: Looping animations never finish
bool IsSpratAnimationFinished(SpratAnimationPlayer player)
{
    float duration = SpratGetAnimationDuration(player.animation);

    if (player.loop || (duration <= 0.0f)) return false;

    return (player.speed < 0.0f)? (player.time <= 0.0f) : (player.time >= duration);
}

// Get frame index shown now (an index into sheet.frames), -1 if none
// NOTE: Frame k covers the times [k/fps, (k + 1)/fps), or (k/fps, (k + 1)/fps] in reverse,
// so playing from either end shows the frame at that end first
int GetSpratAnimationFrame(SpratAnimationPlayer player)
{
    const SpratAnimation *anim = &player.animation;

    if ((anim->frameCount <= 0) || (anim->frames == NULL) || (anim->fps <= 0)) return -1;

    double position = (double)player.time*anim->fps;
    double step = (player.speed < 0.0f)? ceil(position) - 1.0 : floor(position);
    if (!isfinite(step)) return -1;     // NaN or infinite time: (int)step would be undefined

    if (player.loop)
    {
        // fmod() keeps the step exact and in range for any time, negative included
        step = fmod(step, (double)anim->frameCount);
        if (step < 0.0) step += anim->frameCount;
    }
    else if (step < 0.0) step = 0.0;
    else if (step > anim->frameCount - 1) step = anim->frameCount - 1;

    return anim->frames[(int)step];
}

// Draw the frame shown now, with the animation's flip
void DrawSpratAnimation(SpratSheet sheet, SpratAnimationPlayer player, Vector2 position, Color tint)
{
    SpratSprite sprite = MakeSpratSprite(sheet, GetSpratAnimationFrame(player), position);

    sprite.flip = player.animation.flip;

    DrawSpratSprite(sprite, tint);
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

// Get animation duration in seconds, 0 for an animation without frames
static float SpratGetAnimationDuration(SpratAnimation animation)
{
    if ((animation.frameCount <= 0) || (animation.fps <= 0)) return 0.0f;

    return (float)((double)animation.frameCount/animation.fps);
}

// Get the cosine and sine of a clockwise angle in degrees
// NOTE: Quarter turns are exact, without trigonometry, so a turned point keeps whole coordinates
static void SpratGetCosSin(double angle, double *cosRes, double *sinRes)
{
    double turn = fmod(angle, 360.0);
    if (turn < 0.0) turn += 360.0;

    if (turn == 0.0) { *cosRes = 1.0; *sinRes = 0.0; }
    else if (turn == 90.0) { *cosRes = 0.0; *sinRes = 1.0; }
    else if (turn == 180.0) { *cosRes = -1.0; *sinRes = 0.0; }
    else if (turn == 270.0) { *cosRes = 0.0; *sinRes = -1.0; }
    else
    {
        *cosRes = cos(angle*(SPRAT_PI_D/180.0));
        *sinRes = sin(angle*(SPRAT_PI_D/180.0));
    }
}

// Get the placement of original-image points: scaled, then flipped around the pivot, then
// rotated clockwise around it, with the pivot moved to position
static SpratPlacement SpratGetPlacement(Vector2 pivot, Vector2 position, Vector2 scale, float rotation, int flip)
{
    SpratPlacement placement = { 0 };
    double scaleX = (flip & SPRAT_FLIP_HORIZONTAL)? -scale.x : scale.x;
    double scaleY = (flip & SPRAT_FLIP_VERTICAL)? -scale.y : scale.y;
    double cosRes = 1.0, sinRes = 0.0;

    SpratGetCosSin(rotation, &cosRes, &sinRes);

    placement.m[0] = scaleX*cosRes;
    placement.m[1] = -scaleY*sinRes;
    placement.m[2] = scaleX*sinRes;
    placement.m[3] = scaleY*cosRes;
    placement.pivot = (SpratPoint){ pivot.x, pivot.y };
    placement.position = (SpratPoint){ position.x, position.y };

    return placement;
}

// Place an original-image point on the screen
static SpratPoint SpratPlacePoint(const SpratPlacement *placement, double x, double y)
{
    double dx = x - placement->pivot.x;
    double dy = y - placement->pivot.y;

    return (SpratPoint){ placement->position.x + placement->m[0]*dx + placement->m[1]*dy,
                         placement->position.y + placement->m[2]*dx + placement->m[3]*dy };
}

// Get the placement of a sprite's frame, false for an out-of-range frame
static bool SpratGetSpritePlacement(SpratSprite sprite, SpratPlacement *placement)
{
    if ((sprite.frame < 0) || (sprite.frame >= sprite.sheet.frameCount)) return false;

    *placement = SpratGetPlacement(sprite.sheet.frames[sprite.frame].pivot, sprite.position, sprite.scale, sprite.rotation, sprite.flip);

    return true;
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
// NOTE: The content's center is placed as markers are, so what is drawn and what collides agree
static SpratDrawCall SpratGetFrameDrawCall(const SpratFrame *frame, Vector2 position, Vector2 scale, float rotation, int flip)
{
    SpratPlacement placement = SpratGetPlacement(frame->pivot, position, scale, rotation, flip);
    SpratPoint center = SpratPlacePoint(&placement, frame->content.x + frame->content.width/2.0, frame->content.y + frame->content.height/2.0);

    // A negative scale mirrors around the pivot, as a flip does
    if (scale.x < 0.0f) { scale.x = -scale.x; flip ^= SPRAT_FLIP_HORIZONTAL; }
    if (scale.y < 0.0f) { scale.y = -scale.y; flip ^= SPRAT_FLIP_VERTICAL; }

    Vector2 size = { frame->content.width*scale.x, frame->content.height*scale.y };

    return SpratGetRegionDrawCall(frame, frame->source, (Vector2){ (float)center.x, (float)center.y }, size, rotation, flip);
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

// Map a point by a 2x2 matrix
static SpratPoint SpratMultiply(const double m[4], double x, double y)
{
    return (SpratPoint){ m[0]*x + m[1]*y, m[2]*x + m[3]*y };
}

// Get the determinant of a 2x2 matrix
static double SpratDeterminant(const double m[4])
{
    return m[0]*m[3] - m[1]*m[2];
}

// Invert a 2x2 matrix of a nonzero determinant
static void SpratInvert(const double m[4], double inverse[4])
{
    double det = SpratDeterminant(m);

    inverse[0] = m[3]/det;
    inverse[1] = -m[1]/det;
    inverse[2] = -m[2]/det;
    inverse[3] = m[0]/det;
}

// Set a shape to the ellipse center + l, or to what a radius or a scale of 0 flattens it to
// NOTE: l's columns are where the frame's x and y axes go, scaled by the radius: a scale of 0
// zeroes one, leaving the segment center +- the other, and a radius of 0 both, leaving a point
static void SpratSetEllipse(SpratShape *shape, SpratPoint center, const double l[4])
{
    int column = ((l[0] != 0.0) || (l[2] != 0.0))? 0 : 1;

    shape->center = center;
    memcpy(shape->l, l, sizeof(shape->l));

    if (SpratDeterminant(l) != 0.0) shape->ellipse = true;
    else if ((l[column] == 0.0) && (l[column + 2] == 0.0))
    {
        shape->vertexCount = 1;
        shape->points[0] = center;
    }
    else
    {
        shape->vertexCount = 2;
        shape->points[0] = (SpratPoint){ center.x - l[column], center.y - l[column + 2] };
        shape->points[1] = (SpratPoint){ center.x + l[column], center.y + l[column + 2] };
    }
}

// Place a sprite's marker on the screen, false for a NULL or invalid marker or an out-of-range frame
static bool SpratGetMarkerShape(SpratSprite sprite, const SpratMarker *marker, SpratShape *shape)
{
    SpratPlacement p = { 0 };

    if ((marker == NULL) || !SpratGetSpritePlacement(sprite, &p)) return false;

    double x0 = marker->position.x;
    double y0 = marker->position.y;

    switch (marker->type)
    {
        case SPRAT_MARKER_TYPE_POINT:
        {
            shape->vertexCount = 1;
            shape->points[0] = SpratPlacePoint(&p, x0, y0);
        } break;
        case SPRAT_MARKER_TYPE_RECTANGLE:
        {
            double x1 = x0 + marker->size.x, y1 = y0 + marker->size.y;

            shape->vertexCount = 4;
            shape->points[0] = SpratPlacePoint(&p, x0, y0);
            shape->points[1] = SpratPlacePoint(&p, x1, y0);
            shape->points[2] = SpratPlacePoint(&p, x1, y1);
            shape->points[3] = SpratPlacePoint(&p, x0, y1);
        } break;
        case SPRAT_MARKER_TYPE_CIRCLE:
        {
            double l[4] = { 0 };
            for (int i = 0; i < 4; i++) l[i] = p.m[i]*marker->radius;

            shape->circle = true;
            SpratSetEllipse(shape, SpratPlacePoint(&p, x0, y0), l);
        } break;
        case SPRAT_MARKER_TYPE_POLYGON:
        {
            // INT_MAX vertices would overflow the count of a closed outline
            if ((marker->vertexCount < 1) || (marker->vertexCount == INT_MAX) || (marker->vertices == NULL)) return false;

            shape->vertexCount = marker->vertexCount;
            shape->vertices = marker->vertices;
            shape->placement = p;
        } break;
        default: return false;
    }

    return true;
}

// Get vertex i of a polygon shape, on the screen
static SpratPoint SpratGetScreenVertex(const SpratShape *shape, int i)
{
    if (shape->vertices == NULL) return shape->points[i];

    return SpratPlacePoint(&shape->placement, shape->vertices[i].x, shape->vertices[i].y);
}

// Get vertex i of a polygon shape, in a space
static SpratPoint SpratGetVertex(const SpratShape *shape, int i, const SpratSpace *space)
{
    SpratPoint p = SpratGetScreenVertex(shape, i);

    return SpratMultiply(space->inverse, p.x - space->origin.x, p.y - space->origin.y);
}

// Get the edge count of a polygon shape: a point is one edge to itself, a segment one back and forth
// NOTE: Edge i runs from vertex i - 1 (the last for i = 0) to vertex i
static int SpratGetEdgeCount(const SpratShape *shape)
{
    return (shape->vertexCount < 3)? 1 : shape->vertexCount;
}

// Get the cross product of o->a and o->b
static double SpratCross(SpratPoint o, SpratPoint a, SpratPoint b)
{
    return (a.x - o.x)*(b.y - o.y) - (a.y - o.y)*(b.x - o.x);
}

// Check if p, known to be on the line through a and b, lies between them
static bool SpratIsOnSegment(SpratPoint a, SpratPoint b, SpratPoint p)
{
    return (fmin(a.x, b.x) <= p.x) && (p.x <= fmax(a.x, b.x)) && (fmin(a.y, b.y) <= p.y) && (p.y <= fmax(a.y, b.y));
}

// Check if segments p0-p1 and q0-q1 share a point, ends included; either may be a single point
static bool SpratSegmentsIntersect(SpratPoint p0, SpratPoint p1, SpratPoint q0, SpratPoint q1)
{
    double d0 = SpratCross(q0, q1, p0);
    double d1 = SpratCross(q0, q1, p1);
    double d2 = SpratCross(p0, p1, q0);
    double d3 = SpratCross(p0, p1, q1);

    if ((((d0 > 0.0) && (d1 < 0.0)) || ((d0 < 0.0) && (d1 > 0.0))) &&
        (((d2 > 0.0) && (d3 < 0.0)) || ((d2 < 0.0) && (d3 > 0.0)))) return true;

    return ((d0 == 0.0) && SpratIsOnSegment(q0, q1, p0)) || ((d1 == 0.0) && SpratIsOnSegment(q0, q1, p1)) ||
           ((d2 == 0.0) && SpratIsOnSegment(p0, p1, q0)) || ((d3 == 0.0) && SpratIsOnSegment(p0, p1, q1));
}

// Check if p is inside a polygon shape of 3 vertices or more, by the even-odd rule
// NOTE: Points on its edges are left to the edge tests
static bool SpratIsInsidePolygon(SpratPoint p, const SpratShape *shape, const SpratSpace *space)
{
    SpratPoint previous = SpratGetVertex(shape, shape->vertexCount - 1, space);
    bool inside = false;

    for (int i = 0; i < shape->vertexCount; i++)
    {
        SpratPoint current = SpratGetVertex(shape, i, space);

        if (((current.y > p.y) != (previous.y > p.y)) &&
            (p.x < (previous.x - current.x)*(p.y - current.y)/(previous.y - current.y) + current.x)) inside = !inside;

        previous = current;
    }

    return inside;
}

// Check if two polygons overlap: edges of both cross or touch, or else one is inside the other
static bool SpratPolygonsOverlap(const SpratShape *a, const SpratShape *b)
{
    const SpratSpace screen = { { 0.0, 0.0 }, { 1.0, 0.0, 0.0, 1.0 } };

    SpratPoint a0 = SpratGetScreenVertex(a, a->vertexCount - 1);

    for (int i = 0; i < SpratGetEdgeCount(a); i++)
    {
        SpratPoint a1 = SpratGetScreenVertex(a, i);
        SpratPoint b0 = SpratGetScreenVertex(b, b->vertexCount - 1);

        for (int j = 0; j < SpratGetEdgeCount(b); j++)
        {
            SpratPoint b1 = SpratGetScreenVertex(b, j);

            if (SpratSegmentsIntersect(a0, a1, b0, b1)) return true;

            b0 = b1;
        }

        a0 = a1;
    }

    return ((b->vertexCount >= 3) && SpratIsInsidePolygon(SpratGetVertex(a, 0, &screen), b, &screen)) ||
           ((a->vertexCount >= 3) && SpratIsInsidePolygon(SpratGetVertex(b, 0, &screen), a, &screen));
}

// Get the unit space of an ellipse, where it is the unit disk
static SpratSpace SpratGetUnitSpace(const SpratShape *ellipse)
{
    SpratSpace space = { 0 };

    space.origin = ellipse->center;
    SpratInvert(ellipse->l, space.inverse);

    return space;
}

// Get the squared distance from the origin to segment a-b
static double SpratGetSquaredDistanceToOrigin(SpratPoint a, SpratPoint b)
{
    double dx = b.x - a.x, dy = b.y - a.y;
    double length = dx*dx + dy*dy;
    double t = 0.0;

    if (length > 0.0) t = fmin(fmax(-(a.x*dx + a.y*dy)/length, 0.0), 1.0);

    a.x += t*dx;
    a.y += t*dy;

    return a.x*a.x + a.y*a.y;
}

// Check if an ellipse and a polygon overlap: in the ellipse's unit space, an edge comes within 1
// of the origin, or the origin is inside the polygon
static bool SpratEllipseOverlapsPolygon(const SpratShape *ellipse, const SpratShape *polygon)
{
    SpratSpace space = SpratGetUnitSpace(ellipse);
    SpratPoint origin = { 0.0, 0.0 };

    SpratPoint p0 = SpratGetVertex(polygon, polygon->vertexCount - 1, &space);

    for (int i = 0; i < SpratGetEdgeCount(polygon); i++)
    {
        SpratPoint p1 = SpratGetVertex(polygon, i, &space);

        if (SpratGetSquaredDistanceToOrigin(p0, p1) <= 1.0 + SPRAT_UNIT_TOLERANCE) return true;

        p0 = p1;
    }

    return (polygon->vertexCount >= 3) && SpratIsInsidePolygon(origin, polygon, &space);
}

// Check if two ellipses overlap: in the unit space of a, the ellipse c + l*u (|u| <= 1) that b
// becomes holds the origin or comes within 1 of it
// NOTE: Its nearest point is on its edge, where (l^T l + mu) u = -l^T c for the mu >= 0 that
// gives |u| = 1, found by bisection
static bool SpratEllipsesOverlap(const SpratShape *a, const SpratShape *b)
{
    SpratSpace space = SpratGetUnitSpace(a);
    SpratPoint c = SpratMultiply(space.inverse, b->center.x - a->center.x, b->center.y - a->center.y);
    double l[4] = {
        space.inverse[0]*b->l[0] + space.inverse[1]*b->l[2],
        space.inverse[0]*b->l[1] + space.inverse[1]*b->l[3],
        space.inverse[2]*b->l[0] + space.inverse[3]*b->l[2],
        space.inverse[2]*b->l[1] + space.inverse[3]*b->l[3]
    };
    double inverse[4] = { 0 };

    // The origin inside b
    SpratInvert(l, inverse);
    SpratPoint u = SpratMultiply(inverse, -c.x, -c.y);
    if (u.x*u.x + u.y*u.y <= 1.0 + SPRAT_UNIT_TOLERANCE) return true;

    // l^T l, its eigenvalues and eigenvectors, and l^T c along them
    double p00 = l[0]*l[0] + l[2]*l[2];
    double p01 = l[0]*l[1] + l[2]*l[3];
    double p11 = l[1]*l[1] + l[3]*l[3];
    double mean = (p00 + p11)/2.0;
    double halfDifference = (p00 - p11)/2.0;
    double root = sqrt(halfDifference*halfDifference + p01*p01);
    double lambda0 = mean + root;
    double lambda1 = fmax(mean - root, 0.0);

    // b's longest semi-axis, sqrt(lambda0), doesn't reach the unit circle
    if (sqrt(c.x*c.x + c.y*c.y) - sqrt(lambda0) > 1.0 + SPRAT_UNIT_TOLERANCE) return false;

    double theta = atan2(2.0*p01, p00 - p11)/2.0;
    double e0x = cos(theta), e0y = sin(theta);
    double e1x = -e0y, e1y = e0x;
    double gx = l[0]*c.x + l[2]*c.y;
    double gy = l[1]*c.x + l[3]*c.y;
    double g0 = e0x*gx + e0y*gy;
    double g1 = e1x*gx + e1y*gy;

    // |u(mu)| falls from over 1 at mu = 0, where u is the point found above, to at most 1 at |g|
    double low = 0.0;
    double high = sqrt(gx*gx + gy*gy);

    for (int i = 0; (i < 200) && (low < high); i++)
    {
        double middle = (low + high)/2.0;
        double u0 = g0/(lambda0 + middle);
        double u1 = (g1 == 0.0)? 0.0 : g1/(lambda1 + middle);

        if ((middle == low) || (middle == high)) break;

        if (u0*u0 + u1*u1 > 1.0) low = middle;
        else high = middle;
    }

    double mu = high;

    // The nearest point on b's edge, kept on it by normalizing u
    double v0 = g0/(lambda0 + mu);
    double v1 = (g1 == 0.0)? 0.0 : g1/(lambda1 + mu);
    double ux = -v0*e0x - v1*e1x;
    double uy = -v0*e0y - v1*e1y;
    double length = sqrt(ux*ux + uy*uy);

    if (length > 0.0)
    {
        ux /= length;
        uy /= length;
    }

    SpratPoint nearest = SpratMultiply(l, ux, uy);
    nearest.x += c.x;
    nearest.y += c.y;

    return nearest.x*nearest.x + nearest.y*nearest.y <= 1.0 + SPRAT_UNIT_TOLERANCE;
}

// Check if two placed shapes overlap
static bool SpratShapesOverlap(const SpratShape *a, const SpratShape *b)
{
    if (a->ellipse && b->ellipse) return SpratEllipsesOverlap(a, b);
    if (a->ellipse) return SpratEllipseOverlapsPolygon(a, b);
    if (b->ellipse) return SpratEllipseOverlapsPolygon(b, a);

    return SpratPolygonsOverlap(a, b);
}

#endif // SPRAT_RAYLIB_IMPLEMENTATION
