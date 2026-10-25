/* Tests sprat-raylib without a window: converting sheets, the queries, and the
 * geometry of every draw. Draws are checked by rasterizing the DrawTexturePro()
 * arguments the library computes, the way raylib maps its quad, and comparing
 * every screen pixel with the original image placed at its pivot. */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The size of the last sheet allocation, to check what a sheet costs. */
static size_t lastCallocSize = 0;

static void *recording_calloc(size_t count, size_t size)
{
    lastCallocSize = count*size;
    return calloc(count, size);
}

#define RL_CALLOC(n, sz) recording_calloc(n, sz)

#define SPRAT_RAYLIB_IMPLEMENTATION
#include "sprat-raylib.h"

static int failures = 0;

#define COUNT(array) (sizeof(array)/sizeof((array)[0]))

/* Where screen pixels are sampled: just past their center, so a sample never
 * lands exactly on a source pixel's edge, where the library's float math and
 * the double-precision references below could round to different sides (with
 * dest width 47 for a 12 pixel image, center 23.5 maps exactly to 6). */
#define SAMPLE 0.501

#define CHECK(cond) \
    do { if (!(cond)) { fprintf(stderr, "%s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #cond); failures++; } } while (0)
#define CHECK_INT(actual, expected) \
    do { long long a_ = (long long)(actual), e_ = (long long)(expected); \
         if (a_ != e_) { fprintf(stderr, "%s:%d: %s is %lld, expected %lld\n", __FILE__, __LINE__, #actual, a_, e_); failures++; } } while (0)
#define CHECK_FLOAT(actual, expected) \
    do { double a_ = (double)(actual), e_ = (double)(expected); \
         if (fabs(a_ - e_) > 1e-4) { fprintf(stderr, "%s:%d: %s is %g, expected %g\n", __FILE__, __LINE__, #actual, a_, e_); failures++; } } while (0)
#define CHECK_STR(actual, expected) \
    do { const char *a_ = (actual), *e_ = (expected); \
         if (a_ == NULL || strcmp(a_, e_) != 0) { fprintf(stderr, "%s:%d: %s is \"%s\", expected \"%s\"\n", __FILE__, __LINE__, #actual, a_ ? a_ : "(null)", e_); failures++; } } while (0)

static SpratSheet convert_text(const char *text)
{
    sprat_sheet *raw = NULL;
    sprat_error error;
    SpratSheet sheet = { 0 };

    if (sprat_sheet_load_memory(text, strlen(text), &raw, &error) != SPRAT_OK)
    {
        fprintf(stderr, "load failed: %s\n", error.message);
        failures++;
        return sheet;
    }
    sheet = SpratConvertSheet(raw);
    sprat_sheet_free(raw);
    return sheet;
}

/* ---- Converting ----------------------------------------------------------- */

static void test_convert_example(void)
{
    sprat_sheet *raw = NULL;
    sprat_error error;

    if (sprat_sheet_load_file(SPRAT_RAYLIB_EXAMPLE_SHEET, &raw, &error) != SPRAT_OK)
    {
        fprintf(stderr, "%s: %s\n", SPRAT_RAYLIB_EXAMPLE_SHEET, error.message);
        failures++;
        return;
    }
    SpratSheet sheet = SpratConvertSheet(raw);
    sprat_sheet_free(raw);   // the copy keeps no pointer into it

    CHECK(sheet.frames != NULL);
    CHECK_INT(sheet.atlasCount, 1);
    CHECK_INT(sheet.frameCount, 16);
    CHECK_INT(sheet.animationCount, 3);
    CHECK_FLOAT(sheet.scale, 1.0);
    CHECK_INT(sheet.atlases[0].id, 0);   // textures are loaded separately

    // sprite 0 56 12 ... rotation 90, trim 0, pivot 6,50
    SpratFrame sword = sheet.frames[0];
    CHECK_STR(sword.name, "sword");
    CHECK_STR(sword.path, "items/sword.png");
    CHECK_INT(sword.atlas, 0);
    CHECK_FLOAT(sword.source.width, 56);
    CHECK_FLOAT(sword.source.height, 12);
    CHECK_INT(sword.rotation, 90);
    CHECK_FLOAT(sword.size.x, 12);
    CHECK_FLOAT(sword.size.y, 56);
    CHECK_FLOAT(sword.content.width, 12);
    CHECK_FLOAT(sword.content.height, 56);
    CHECK_FLOAT(sword.pivot.x, 6);
    CHECK_FLOAT(sword.pivot.y, 50);

    // run_01: 41x21 stored turned, trim 16,4 11,3 of a 48x48 image
    int run01 = GetSpratFrameIndex(sheet, "hero/run_01.png");
    CHECK_INT(run01, 5);
    SpratFrame hero = sheet.frames[run01];
    CHECK_FLOAT(hero.content.x, 16);
    CHECK_FLOAT(hero.content.y, 4);
    CHECK_FLOAT(hero.content.width, 21);
    CHECK_FLOAT(hero.content.height, 41);
    CHECK_FLOAT(hero.size.x, 48);
    CHECK_FLOAT(hero.size.y, 48);
    CHECK_INT(hero.markerCount, 3);
    CHECK_STR(hero.markers[2].name, "hitbox");
    CHECK_INT(hero.markers[2].type, SPRAT_MARKER_TYPE_RECTANGLE);
    CHECK_FLOAT(hero.markers[2].position.x, 16);
    CHECK_FLOAT(hero.markers[2].size.y, 40);
    CHECK_INT(hero.markers[2].vertexCount, 0);
    CHECK(hero.markers[2].vertices == NULL);

    SpratFrame panel = sheet.frames[GetSpratFrameIndex(sheet, "panel")];
    CHECK_FLOAT(panel.slice.left, 8);
    CHECK_FLOAT(panel.slice.bottom, 8);
    CHECK_INT(panel.slice.horizontalMode, SPRAT_SLICE_MODE_STRETCH);
    CHECK_INT(panel.markerCount, 0);
    CHECK(panel.markers == NULL);

    SpratAnimation run = sheet.animations[GetSpratAnimationIndex(sheet, "run")];
    SpratAnimation left = sheet.animations[GetSpratAnimationIndex(sheet, "run-left")];
    CHECK_INT(run.fps, 10);
    CHECK_INT(run.frameCount, 6);
    CHECK_INT(run.frames[0], run01);
    CHECK_INT(run.flip, 0);
    CHECK_INT(left.frameCount, 6);
    CHECK_INT(left.frames[5], run.frames[5]);
    CHECK_INT(left.flip, SPRAT_FLIP_HORIZONTAL);

    UnloadSpratSheet(sheet);
}

static void test_convert_polygon_and_empty(void)
{
    SpratSheet sheet = convert_text(
        "sprat-raw 1\n"
        "sheet 2 0 0 0 1 1 1 1 2 0 11 0: 0:\n"
        "atlas 16 16 0: 0: 0:\n"
        "sprite 0 0 0 8 8 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1:s 1:s 0:\n"
        "marker 3 3 4 0 0 0 2 3 4 1 2 4:poly\n"
        "anim 10 0 -1 0 5:empty\n");

    CHECK_FLOAT(sheet.scale, 2.0);
    CHECK_INT(sheet.frames[0].markers[0].type, SPRAT_MARKER_TYPE_POLYGON);
    CHECK_INT(sheet.frames[0].markers[0].vertexCount, 2);
    CHECK_FLOAT(sheet.frames[0].markers[0].vertices[0].x, 3);
    CHECK_FLOAT(sheet.frames[0].markers[0].vertices[1].y, 2);
    CHECK_STR(sheet.animations[0].name, "empty");
    CHECK_INT(sheet.animations[0].frameCount, 0);
    CHECK(sheet.animations[0].frames == NULL);
    CHECK_INT(GetSpratAnimationFrame(sheet, 0, 1.0f), -1);
    UnloadSpratSheet(sheet);

    // No sprites at all: frames still holds the sheet's memory
    sheet = convert_text("sprat-raw 1\nsheet 1 0 0 0 1 0 0 0 0 0 0 0: 0:\natlas 4 4 0: 0: 0:\n");
    CHECK(sheet.frames != NULL);
    CHECK_INT(sheet.frameCount, 0);
    CHECK(sheet.animations == NULL);
    CHECK(!IsSpratSheetValid(sheet));   // its atlas texture is not loaded
    UnloadSpratSheet(sheet);
}

/* Aliases share their source's frame indexes, so they can't multiply the size
 * of the copy: on a 32-bit size_t, that size once wrapped around and the copy
 * overflowed the allocation. */
static void test_alias_frames_are_shared(void)
{
    // run-back (0) aliases run (2) through run-alias (1), declared before both
    SpratSheet sheet = convert_text(
        "sprat-raw 1\n"
        "sheet 1 0 0 0 1 2 3 0 0 3 24 0: 0:\n"
        "atlas 16 16 0: 0: 0:\n"
        "sprite 0 0 0 4 4 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1:a 1:a 0:\n"
        "sprite 0 4 0 4 4 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1:b 1:b 0:\n"
        "anim 5 1 1 0 8:run-back\n"
        "anim 5 0 2 0 9:run-alias\n"
        "anim 5 0 -1 3 1 0 1 3:run\n");

    CHECK_INT(sheet.animations[2].frameCount, 3);
    CHECK(sheet.animations[0].frames == sheet.animations[2].frames);
    CHECK(sheet.animations[1].frames == sheet.animations[2].frames);
    CHECK_INT(sheet.animations[0].frameCount, 3);
    CHECK_INT(sheet.animations[0].frames[0], 1);
    CHECK_INT(sheet.animations[0].frames[1], 0);
    CHECK_INT(sheet.animations[0].flip, SPRAT_FLIP_HORIZONTAL);
    UnloadSpratSheet(sheet);

    // 1000 frames aliased 1000 times: a million indexes if each alias had its copy
    enum { FRAMES = 1000, ALIASES = 1000 };
    size_t capacity = 256 + FRAMES*2 + ALIASES*32;
    char *text = (char *)malloc(capacity);
    size_t length = (size_t)snprintf(text, capacity,
        "sprat-raw 1\nsheet 1 0 0 0 1 1 %d 0 0 %d %d 0: 0:\natlas 4 4 0: 0: 0:\n"
        "sprite 0 0 0 4 4 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1:s 1:s 0:\nanim 10 0 -1 %d",
        ALIASES + 1, FRAMES, 2 + 1 + ALIASES*4, FRAMES);
    for (int i = 0; i < FRAMES; i++) length += (size_t)snprintf(text + length, capacity - length, " 0");
    length += (size_t)snprintf(text + length, capacity - length, " 1:r\n");
    for (int i = 0; i < ALIASES; i++) length += (size_t)snprintf(text + length, capacity - length, "anim 10 0 0 0 4:a%03d\n", i);

    sheet = convert_text(text);
    CHECK_INT(sheet.animationCount, ALIASES + 1);
    CHECK_INT(sheet.animations[ALIASES].frameCount, FRAMES);
    CHECK(sheet.animations[ALIASES].frames == sheet.animations[0].frames);
    CHECK(lastCallocSize < 256*1024);
    UnloadSpratSheet(sheet);
    free(text);
}

static void test_add_size_overflow(void)
{
    size_t total = 0;

    CHECK(SpratAddSize(&total, 3, 4, true));
    CHECK_INT(total, 16);
    CHECK(SpratAddSize(&total, 5, 1, false));
    CHECK_INT(total, 21);

    // A product or a sum past SIZE_MAX fails, leaving the total as it was
    CHECK(!SpratAddSize(&total, SIZE_MAX/4 + 1, 4, false));
    CHECK(!SpratAddSize(&total, SIZE_MAX - 10, 1, false));
    CHECK(!SpratAddSize(&total, SIZE_MAX/16, 16, true));    // fits, but not once aligned
    CHECK_INT(total, 21);
}

static void test_join_path(void)
{
    char buffer[16];

    CHECK(SpratJoinPath(buffer, sizeof(buffer), "res", "a.png"));
    CHECK_STR(buffer, "res/a.png");
    CHECK(SpratJoinPath(buffer, sizeof(buffer), "res/", "a.png"));
    CHECK_STR(buffer, "res/a.png");
    CHECK(SpratJoinPath(buffer, sizeof(buffer), ".", "a.png"));
    CHECK_STR(buffer, "a.png");
    CHECK(SpratJoinPath(buffer, sizeof(buffer), NULL, "a.png"));
    CHECK_STR(buffer, "a.png");
    CHECK(SpratJoinPath(buffer, sizeof(buffer), "res", "/abs.png"));
    CHECK_STR(buffer, "/abs.png");
    CHECK(SpratJoinPath(buffer, sizeof(buffer), "res", "C:\\a.png"));
    CHECK_STR(buffer, "C:\\a.png");
    CHECK(SpratJoinPath(buffer, sizeof(buffer), "res", "0123456789"));
    CHECK(!SpratJoinPath(buffer, sizeof(buffer), "res", "0123456789ab"));   // 16 bytes with the NUL
}

/* ---- Queries -------------------------------------------------------------- */

static void test_lookups(void)
{
    SpratSheet sheet = convert_text(
        "sprat-raw 1\n"
        "sheet 1 0 0 0 1 3 2 1 0 3 53 0: 0:\n"
        "atlas 16 16 0: 0: 0:\n"
        "sprite 0 0 0 4 4 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 4:idle 10:a/idle.png 0:\n"
        "marker 1 2 3 4 0 0 0 3:hit\n"
        "sprite 0 4 0 4 4 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 4:idle 10:b/idle.png 0:\n"
        "sprite 0 8 0 4 4 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 4:jump 10:b/jump.png 0:\n"
        "anim 4 0 -1 3 2 0 1 4:walk\n"
        "anim 4 3 0 0 4:back\n");

    CHECK_INT(GetSpratFrameIndex(sheet, "a/idle.png"), 0);
    CHECK_INT(GetSpratFrameIndex(sheet, "b/idle.png"), 1);
    CHECK_INT(GetSpratFrameIndex(sheet, "jump"), 2);        // a unique name
    CHECK_INT(GetSpratFrameIndex(sheet, "idle"), -1);       // ambiguous: two folders
    CHECK_INT(GetSpratFrameIndex(sheet, "missing"), -1);
    CHECK_INT(GetSpratFrameIndex(sheet, NULL), -1);

    CHECK_INT(GetSpratAnimationIndex(sheet, "back"), 1);
    CHECK_INT(GetSpratAnimationIndex(sheet, "run"), -1);
    CHECK_INT(sheet.animations[1].flip, SPRAT_FLIP_HORIZONTAL | SPRAT_FLIP_VERTICAL);

    const SpratMarker *hit = GetSpratFrameMarker(sheet, 0, "hit");
    CHECK(hit != NULL);
    if (hit != NULL)
    {
        CHECK_INT(hit->type, SPRAT_MARKER_TYPE_CIRCLE);
        CHECK_FLOAT(hit->radius, 4);
    }
    CHECK(GetSpratFrameMarker(sheet, 0, "pivot") == NULL);
    CHECK(GetSpratFrameMarker(sheet, 3, "hit") == NULL);
    CHECK(GetSpratFrameMarker(sheet, -1, "hit") == NULL);

    // walk plays frames 2, 0, 1 at 4 fps, looping
    CHECK_INT(GetSpratAnimationFrame(sheet, 0, 0.0f), 2);
    CHECK_INT(GetSpratAnimationFrame(sheet, 0, 0.24f), 2);
    CHECK_INT(GetSpratAnimationFrame(sheet, 0, 0.25f), 0);
    CHECK_INT(GetSpratAnimationFrame(sheet, 0, 0.5f), 1);
    CHECK_INT(GetSpratAnimationFrame(sheet, 0, 0.75f), 2);
    CHECK_INT(GetSpratAnimationFrame(sheet, 0, -0.1f), 1);  // loops backwards too
    CHECK_INT(GetSpratAnimationFrame(sheet, 0, 3600.0f*24.0f*365.0f), 2);   // 4*31536000 steps, a multiple of 3
    CHECK_INT(GetSpratAnimationFrame(sheet, 0, NAN), -1);
    CHECK_INT(GetSpratAnimationFrame(sheet, 0, INFINITY), -1);
    CHECK_INT(GetSpratAnimationFrame(sheet, 0, -INFINITY), -1);
    CHECK_INT(GetSpratAnimationFrame(sheet, 1, 0.25f), 0);
    CHECK_INT(GetSpratAnimationFrame(sheet, 2, 0.0f), -1);
    CHECK_INT(GetSpratAnimationFrame(sheet, -1, 0.0f), -1);

    UnloadSpratSheet(sheet);

    SpratSheet empty = { 0 };
    CHECK(!IsSpratSheetValid(empty));
    CHECK_INT(GetSpratFrameIndex(empty, "a"), -1);
    UnloadSpratSheet(empty);   // a no-op
}

/* ---- Drawing -------------------------------------------------------------- */

#define ATLAS_SIZE 64
#define SCREEN 160

typedef struct image_case {
    int w, h;
    int left, top, right, bottom;   /* trim */
    int pivotX, pivotY;
} image_case;

static int atlas[ATLAS_SIZE][ATLAS_SIZE];

/* A distinct, nonzero value for every pixel of the original image. */
static int original_pixel(int x, int y)
{
    return y*64 + x + 1;
}

/* Packs the trimmed image at (3, 5) of the atlas, turned like spratpack:
 *   CW (90):   atlas(col, row) <- content(row, h - 1 - col)
 *   CCW (270): atlas(col, row) <- content(w - 1 - row, col)
 * and returns the frame sprat-raylib would load for it. */
static SpratFrame pack(const image_case *img, int rotation, SpratSlice slice)
{
    int cw = img->w - img->left - img->right;
    int ch = img->h - img->top - img->bottom;
    int rw = (rotation == 0)? cw : ch;
    int rh = (rotation == 0)? ch : cw;

    memset(atlas, 0, sizeof(atlas));
    for (int row = 0; row < rh; row++)
    {
        for (int col = 0; col < rw; col++)
        {
            int px = (rotation == 90)? row : (rotation == 270)? cw - 1 - row : col;
            int py = (rotation == 90)? ch - 1 - col : (rotation == 270)? col : row;
            atlas[5 + row][3 + col] = original_pixel(img->left + px, img->top + py);
        }
    }

    SpratFrame frame = { 0 };
    frame.source = (Rectangle){ 3, 5, (float)rw, (float)rh };
    frame.rotation = rotation;
    frame.size = (Vector2){ (float)img->w, (float)img->h };
    frame.content = (Rectangle){ (float)img->left, (float)img->top, (float)cw, (float)ch };
    frame.pivot = (Vector2){ (float)img->pivotX, (float)img->pivotY };
    frame.slice = slice;
    return frame;
}

/* A DrawTexturePro() call, with the sine and cosine that undo its rotation. */
typedef struct quad {
    SpratDrawCall call;
    double cosR;
    double sinR;
} quad;

static quad make_quad(SpratDrawCall call)
{
    quad q = { call, cos(-call.rotation*DEG2RAD), sin(-call.rotation*DEG2RAD) };
    return q;
}

/* The atlas value DrawTexturePro() shows at screen pixel (px, py), or 0 outside
 * its quad. raylib places the box's top-left at dest.xy - origin, turns it
 * clockwise around dest.xy, and maps the source onto it, mirrored when
 * source.width or source.height is negative. */
static int engine_pixel(const quad *q, int px, int py)
{
    const SpratDrawCall *c = &q->call;
    double x = px + SAMPLE - c->dest.x;
    double y = py + SAMPLE - c->dest.y;
    double u = x*q->cosR - y*q->sinR + c->origin.x;
    double v = x*q->sinR + y*q->cosR + c->origin.y;
    double w = c->dest.width, h = c->dest.height;

    if (u < 0 || v < 0 || u >= w || v >= h) return 0;

    double sw = fabs(c->source.width), sh = fabs(c->source.height);
    if (c->source.width < 0) u = w - u;
    if (c->source.height < 0) v = h - v;

    int sx = (int)floor(c->source.x + u/w*sw);
    int sy = (int)floor(c->source.y + v/h*sh);
    if (sx >= c->source.x + sw) sx = (int)(c->source.x + sw) - 1;
    if (sy >= c->source.y + sh) sy = (int)(c->source.y + sh) - 1;
    return atlas[sy][sx];
}

/* The original pixel that belongs at screen pixel (px, py) when the image is
 * scaled, flipped about its pivot and turned clockwise around it by the angle
 * whose cosine and sine undo it, with the pivot at (wx, wy). 0 where the trim
 * removed it. */
static int expected_pixel(const image_case *img, double wx, double wy, Vector2 scale, double cosR, double sinR,
                          int flip, int px, int py)
{
    double dx = px + SAMPLE - wx;
    double dy = py + SAMPLE - wy;
    double x = (dx*cosR - dy*sinR)/scale.x;
    double y = (dx*sinR + dy*cosR)/scale.y;

    if (flip & SPRAT_FLIP_HORIZONTAL) x = -x;
    if (flip & SPRAT_FLIP_VERTICAL) y = -y;
    x += img->pivotX;
    y += img->pivotY;
    if (x < img->left || x >= img->w - img->right || y < img->top || y >= img->h - img->bottom) return 0;
    return original_pixel((int)floor(x), (int)floor(y));
}

static void test_draw_matches_original(void)
{
    static const image_case images[] = {
        { 9, 6, 1, 2, 3, 1, 6, 2 },     /* odd content size: centered on half pixels */
        { 9, 6, 1, 2, 3, 1, 0, 0 },     /* no pivot */
        { 10, 6, 1, 2, 3, 0, 5, 3 },    /* sprat-lib's README example */
        { 7, 11, 0, 0, 0, 0, 3, 10 },   /* untrimmed, pivot at the feet */
    };
    static const int rotations[] = { 0, 90, 270 };
    static const Vector2 scales[] = { { 1, 1 }, { 3, 3 }, { 2, 3 } };
    static const float turns[] = { 0, 90, 180, 270 };

    for (size_t i = 0; i < COUNT(images); i++)
    for (size_t r = 0; r < COUNT(rotations); r++)
    for (int flip = 0; flip < 4; flip++)
    for (size_t s = 0; s < COUNT(scales); s++)
    for (size_t t = 0; t < COUNT(turns); t++)
    {
        const image_case *img = &images[i];
        SpratSlice noSlice = { 0 };
        SpratFrame frame = pack(img, rotations[r], noSlice);
        Vector2 position = { 80, 70 };
        quad q = make_quad(SpratGetFrameDrawCall(&frame, position, scales[s], turns[t], flip));
        double cosTurn = cos(-turns[t]*DEG2RAD), sinTurn = sin(-turns[t]*DEG2RAD);
        int mismatches = 0, drawn = 0;

        for (int py = 0; py < SCREEN; py++)
        {
            for (int px = 0; px < SCREEN; px++)
            {
                int got = engine_pixel(&q, px, py);
                int want = expected_pixel(img, position.x, position.y, scales[s], cosTurn, sinTurn, flip, px, py);
                if (got != want) mismatches++;
                if (got != 0) drawn++;
            }
        }

        int cw = img->w - img->left - img->right, ch = img->h - img->top - img->bottom;
        int area = (int)(cw*ch*scales[s].x*scales[s].y);
        if (mismatches != 0 || drawn != area)
        {
            fprintf(stderr, "image %zu rotation %d flip %d scale %gx%g turn %g: %d pixels differ, %d of %d drawn\n",
                    i, rotations[r], flip, scales[s].x, scales[s].y, turns[t], mismatches, drawn, area);
            failures++;
        }
    }
}

static void test_frame_point_matches_draw(void)
{
    static const image_case img = { 10, 6, 1, 2, 3, 0, 5, 3 };
    SpratSlice noSlice = { 0 };
    SpratSheet sheet = { 0 };
    SpratFrame frame = pack(&img, 90, noSlice);

    sheet.frames = &frame;
    sheet.frameCount = 1;

    // The pivot lands on the position, whatever the transform
    Vector2 p = GetSpratFramePoint(sheet, 0, frame.pivot, (Vector2){ 40, 30 }, (Vector2){ 2, 3 }, 33.0f, SPRAT_FLIP_HORIZONTAL);
    CHECK_FLOAT(p.x, 40);
    CHECK_FLOAT(p.y, 30);

    // A point inside content pixel (6, 4) lands on the screen pixel that shows it
    p = GetSpratFramePoint(sheet, 0, (Vector2){ 6.5f, 4.5f }, (Vector2){ 80, 70 }, (Vector2){ 2, 2 }, 90.0f, SPRAT_FLIP_HORIZONTAL);
    quad q = make_quad(SpratGetFrameDrawCall(&frame, (Vector2){ 80, 70 }, (Vector2){ 2, 2 }, 90.0f, SPRAT_FLIP_HORIZONTAL));
    CHECK_INT(engine_pixel(&q, (int)floor(p.x), (int)floor(p.y)), original_pixel(6, 4));

    CHECK_FLOAT(GetSpratFramePoint(sheet, 1, frame.pivot, (Vector2){ 9, 8 }, (Vector2){ 1, 1 }, 0.0f, 0).x, 9);
}

/* The original pixel a nine-slice puts at screen pixel (px, py) of dest. */
static int expected_npatch_pixel(const image_case *img, SpratSlice slice, Rectangle dest, int px, int py)
{
    double source[2][4] = {
        { 0, slice.left, img->w - slice.right, img->w },
        { 0, slice.top, img->h - slice.bottom, img->h },
    };
    double start[2] = { dest.x, dest.y };
    double length[2] = { dest.width, dest.height };
    double coord[2] = { px + SAMPLE, py + SAMPLE };
    double out[2];

    for (int axis = 0; axis < 2; axis++)
    {
        double before = source[axis][1], after = source[axis][3] - source[axis][2];
        if (before + after > length[axis])
        {
            double k = length[axis]/(before + after);
            before *= k;
            after *= k;
        }
        double edges[4] = { start[axis], start[axis] + before, start[axis] + length[axis] - after, start[axis] + length[axis] };
        double c = coord[axis];
        if (c < edges[0] || c >= edges[3]) return 0;
        int cell = (c < edges[1])? 0 : (c < edges[2])? 1 : 2;
        out[axis] = source[axis][cell] + (c - edges[cell])/(edges[cell + 1] - edges[cell])*(source[axis][cell + 1] - source[axis][cell]);
    }

    double x = out[0], y = out[1];
    if (x < img->left || x >= img->w - img->right || y < img->top || y >= img->h - img->bottom) return 0;
    return original_pixel((int)floor(x), (int)floor(y));
}

static void test_npatch_matches_original(void)
{
    static const image_case images[] = {
        { 12, 10, 0, 0, 0, 0, 0, 0 },   /* untrimmed */
        { 12, 10, 1, 3, 4, 2, 0, 0 },   /* trimmed into the borders */
        { 12, 10, 5, 4, 5, 4, 0, 0 },   /* trimmed past the borders: only middle content left */
    };
    static const SpratSlice slices[] = {
        { 3, 2, 4, 3, SPRAT_SLICE_MODE_STRETCH, SPRAT_SLICE_MODE_STRETCH },
        { 0, 0, 0, 0, SPRAT_SLICE_MODE_STRETCH, SPRAT_SLICE_MODE_STRETCH },   /* no slice: plain stretch */
    };
    static const Rectangle dests[] = {
        { 10, 20, 12, 10 },     /* natural size */
        { 10, 20, 47, 35 },     /* middle 5x5 grows to 40x30 */
        { 10, 20, 112, 95 },    /* 105 = 7*15 wide, 90 = 18*5 high: exact for both slices */
        { 10, 20, 7, 4 },       /* smaller than the borders: they shrink */
    };
    static const int rotations[] = { 0, 90, 270 };

    for (size_t i = 0; i < COUNT(images); i++)
    for (size_t sl = 0; sl < COUNT(slices); sl++)
    for (size_t d = 0; d < COUNT(dests); d++)
    for (size_t r = 0; r < COUNT(rotations); r++)
    {
        const image_case *img = &images[i];
        SpratFrame frame = pack(img, rotations[r], slices[sl]);
        SpratDrawCall calls[9];
        quad quads[9];
        int count = SpratGetNPatchDrawCalls(&frame, dests[d], calls);
        int mismatches = 0, overlaps = 0;

        for (int c = 0; c < count; c++) quads[c] = make_quad(calls[c]);

        for (int py = 0; py < SCREEN; py++)
        {
            for (int px = 0; px < SCREEN; px++)
            {
                int got = 0;
                for (int c = 0; c < count; c++)
                {
                    int value = engine_pixel(&quads[c], px, py);
                    if (value != 0 && got != 0) overlaps++;
                    if (value != 0) got = value;
                }
                if (got != expected_npatch_pixel(img, slices[sl], dests[d], px, py)) mismatches++;
            }
        }

        if (mismatches != 0 || overlaps != 0)
        {
            fprintf(stderr, "npatch image %zu slice %zu dest %zu rotation %d: %d pixels differ, %d overlap\n",
                    i, sl, d, rotations[r], mismatches, overlaps);
            failures++;
        }
    }

    // Nothing to draw into an empty rectangle
    SpratFrame frame = pack(&images[0], 0, slices[0]);
    SpratDrawCall calls[9];
    CHECK_INT(SpratGetNPatchDrawCalls(&frame, (Rectangle){ 0, 0, 0, 10 }, calls), 0);
}

int main(void)
{
    SetTraceLogLevel(LOG_NONE);

    test_convert_example();
    test_convert_polygon_and_empty();
    test_alias_frames_are_shared();
    test_add_size_overflow();
    test_join_path();
    test_lookups();
    test_draw_matches_original();
    test_frame_point_matches_draw();
    test_npatch_matches_original();

    if (failures != 0) fprintf(stderr, "%d failures\n", failures);
    else printf("all tests passed\n");
    return (failures == 0)? 0 : 1;
}
