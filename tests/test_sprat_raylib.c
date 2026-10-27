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
    CHECK_INT(sheet.animationCount, 5);
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

    SpratAnimation run = GetSpratAnimation(sheet, "run");
    SpratAnimation left = GetSpratAnimation(sheet, "run-left");
    CHECK(IsSpratAnimationValid(run));
    CHECK(run.frames == sheet.animations[0].frames);
    CHECK_INT(run.fps, 10);
    CHECK_INT(run.frameCount, 6);
    CHECK_INT(run.frames[0], run01);
    CHECK_INT(run.flip, 0);
    CHECK_INT(left.frameCount, 6);
    CHECK_INT(left.frames[5], run.frames[5]);
    CHECK_INT(left.flip, SPRAT_FLIP_HORIZONTAL);
    CHECK_INT(run.loop, 0);

    SpratAnimation idle = GetSpratAnimation(sheet, "idle");     // the run's first frame
    CHECK_INT(idle.frameCount, 1);
    CHECK_INT(idle.frames[0], run01);

    SpratAnimation pingpong = GetSpratAnimation(sheet, "spin-pingpong");
    CHECK_INT(pingpong.loop, SPRAT_LOOP_MODE_PINGPONG);
    CHECK_INT(pingpong.frameCount, 5);
    CHECK_INT(pingpong.frames[4], GetSpratFrameIndex(sheet, "coin/spin_05.png"));

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
    CHECK(IsSpratAnimationValid(sheet.animations[0]));      // found, though empty
    SpratAnimationPlayer empty = MakeSpratAnimationPlayer(sheet.animations[0]);
    UpdateSpratAnimation(&empty, 1.0f);
    CHECK_FLOAT(empty.time, 0);
    CHECK_INT(GetSpratAnimationFrame(empty), -1);
    CHECK(!IsSpratAnimationFinished(empty));
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

/* Version 2 sheets give each animation loop flags; an alias plays the way its
 * source does, whatever flags it carries itself. */
static void test_convert_loops(void)
{
    SpratSheet sheet = convert_text(
        "sprat-raw 2\n"
        "sheet 1 0 0 0 1 1 3 0 0 3 13 0: 0:\n"
        "atlas 16 16 0: 0: 0:\n"
        "sprite 0 0 0 4 4 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1:s 1:s 0:\n"
        "anim 4 0 3 -1 2 0 0 3:opp\n"
        "anim 4 1 0 0 0 4:back\n"
        "anim 4 0 1 -1 1 0 4:once\n");

    CHECK_INT(sheet.animations[0].loop, SPRAT_LOOP_MODE_ONCE | SPRAT_LOOP_MODE_PINGPONG);
    CHECK_INT(sheet.animations[1].loop, SPRAT_LOOP_MODE_ONCE | SPRAT_LOOP_MODE_PINGPONG);
    CHECK_INT(sheet.animations[1].flip, SPRAT_FLIP_HORIZONTAL);
    CHECK_INT(sheet.animations[2].loop, SPRAT_LOOP_MODE_ONCE);
    UnloadSpratSheet(sheet);
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

    CHECK_STR(GetSpratAnimation(sheet, "back").name, "back");
    CHECK(!IsSpratAnimationValid(GetSpratAnimation(sheet, "run")));
    CHECK(!IsSpratAnimationValid(GetSpratAnimation(sheet, NULL)));
    CHECK_INT(sheet.animations[1].flip, SPRAT_FLIP_HORIZONTAL | SPRAT_FLIP_VERTICAL);
    CHECK_INT(sheet.animations[0].loop, 0);     // version 1 sheets have no loop flags

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
    SpratAnimationPlayer walk = MakeSpratAnimationPlayer(sheet.animations[0]);
    float times[] = { 0.0f, 0.24f, 0.25f, 0.5f, 0.75f, -0.1f, 3600.0f*24.0f*365.0f, NAN, INFINITY, -INFINITY };
    int expected[] = { 2, 2, 0, 1, 2, 1, 2, -1, -1, -1 };     // a year is 4*31536000 steps, a multiple of 3
    for (size_t i = 0; i < COUNT(times); i++)
    {
        walk.time = times[i];
        CHECK_INT(GetSpratAnimationFrame(walk), expected[i]);
    }
    SpratAnimationPlayer back = MakeSpratAnimationPlayer(sheet.animations[1]);
    back.time = 0.25f;
    CHECK_INT(GetSpratAnimationFrame(back), 0);
    CHECK_INT(GetSpratAnimationFrame(MakeSpratAnimationPlayer(GetSpratAnimation(sheet, "missing"))), -1);

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

/* The first frameCount of frames 7, 8, 9 at 4 fps, with loop flags: each frame
 * lasts 0.25 s, exact in binary, so all three repeating last 0.75 s. */
static SpratAnimationPlayer make_test_player(int loop, int frameCount)
{
    static int frames[] = { 7, 8, 9 };
    SpratAnimation animation = { "walk", 4, frames, frameCount, 0, loop };

    return MakeSpratAnimationPlayer(animation);
}

/* Checks the frames a player shows, updating it by one frame (0.25 s) after each. */
#define CHECK_FRAMES(player, expected) check_frames(__LINE__, (player), (expected), COUNT(expected))

static void check_frames(int line, SpratAnimationPlayer *player, const int *expected, size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        int frame = GetSpratAnimationFrame(*player);
        if (frame != expected[i])
        {
            fprintf(stderr, "%s:%d: frame %zu is %d, expected %d\n", __FILE__, line, i, frame, expected[i]);
            failures++;
        }
        UpdateSpratAnimation(player, 0.25f);
    }
}

static void test_animation_player(void)
{
    SpratAnimationPlayer player = make_test_player(0, 3);
    CHECK(IsSpratAnimationPlaying(player));
    CHECK_INT(player.animation.loop, 0);
    CHECK_FLOAT(player.speed, 1);
    CHECK_FLOAT(player.time, 0);
    CHECK_INT(GetSpratAnimationFrame(player), 7);

    // Looping forward: each frame from its start, and back to the first
    UpdateSpratAnimation(&player, 0.25f);
    CHECK_INT(GetSpratAnimationFrame(player), 8);
    UpdateSpratAnimation(&player, 0.25f);
    CHECK_INT(GetSpratAnimationFrame(player), 9);
    UpdateSpratAnimation(&player, 0.25f);
    CHECK_FLOAT(player.time, 0);
    CHECK_INT(GetSpratAnimationFrame(player), 7);
    UpdateSpratAnimation(&player, 7.5f + 0.25f);    // many loops at once: time stays wrapped
    CHECK_FLOAT(player.time, 0.25f);
    CHECK(!IsSpratAnimationFinished(player));

    // Speed scales the step
    SetSpratAnimationSpeed(&player, 2.0f);
    UpdateSpratAnimation(&player, 0.125f);
    CHECK_INT(GetSpratAnimationFrame(player), 9);
    SetSpratAnimationSpeed(&player, NAN);           // ignored
    CHECK_FLOAT(player.speed, 2);

    // Paused, it keeps its time and frame
    PauseSpratAnimation(&player);
    CHECK(!IsSpratAnimationPlaying(player));
    UpdateSpratAnimation(&player, 0.25f);
    CHECK_FLOAT(player.time, 0.5f);
    ResumeSpratAnimation(&player);
    UpdateSpratAnimation(&player, 0.125f);
    CHECK_FLOAT(player.time, 0.0f);

    // Bad steps keep the last good time
    UpdateSpratAnimation(&player, NAN);
    UpdateSpratAnimation(&player, INFINITY);
    CHECK_FLOAT(player.time, 0.0f);
    UpdateSpratAnimation(NULL, 1.0f);

    // Reverse: starts on the last frame, each frame from its end, wrapping around
    player = make_test_player(0, 3);
    SetSpratAnimationSpeed(&player, -1.0f);
    PlaySpratAnimation(&player);
    CHECK_FLOAT(player.time, 0.75f);
    CHECK_INT(GetSpratAnimationFrame(player), 9);
    UpdateSpratAnimation(&player, 0.25f);
    CHECK_INT(GetSpratAnimationFrame(player), 8);
    UpdateSpratAnimation(&player, 0.25f);
    CHECK_INT(GetSpratAnimationFrame(player), 7);
    UpdateSpratAnimation(&player, 0.125f);
    CHECK_INT(GetSpratAnimationFrame(player), 7);
    UpdateSpratAnimation(&player, 0.125f);
    CHECK_FLOAT(player.time, 0);
    CHECK_INT(GetSpratAnimationFrame(player), 9);  // time 0 ends frame 7, so it is the wrap to 9
    UpdateSpratAnimation(&player, 0.125f);
    CHECK_FLOAT(player.time, 0.625f);
    CHECK_INT(GetSpratAnimationFrame(player), 9);

    // Once: stops on the last frame and reports it finished
    player = make_test_player(0, 3);
    player.animation.loop = SPRAT_LOOP_MODE_ONCE;
    UpdateSpratAnimation(&player, 0.5f);
    CHECK(IsSpratAnimationPlaying(player));
    CHECK(!IsSpratAnimationFinished(player));
    UpdateSpratAnimation(&player, 1.0f);
    CHECK(!IsSpratAnimationPlaying(player));
    CHECK(IsSpratAnimationFinished(player));
    CHECK_FLOAT(player.time, 0.75f);
    CHECK_INT(GetSpratAnimationFrame(player), 9);
    PlaySpratAnimation(&player);
    UpdateSpratAnimation(&player, 0.0f);            // a zero step doesn't stop a fresh one
    CHECK(IsSpratAnimationPlaying(player));
    CHECK_INT(GetSpratAnimationFrame(player), 7);

    // Once in reverse: stops on the first frame
    SetSpratAnimationSpeed(&player, -1.0f);
    PlaySpratAnimation(&player);
    UpdateSpratAnimation(&player, 2.0f);
    CHECK(!IsSpratAnimationPlaying(player));
    CHECK(IsSpratAnimationFinished(player));
    CHECK_FLOAT(player.time, 0);
    CHECK_INT(GetSpratAnimationFrame(player), 7);

    // Stop rewinds to where Play starts, without playing
    UpdateSpratAnimation(&player, 0.0f);
    StopSpratAnimation(&player);
    CHECK(!IsSpratAnimationPlaying(player));
    CHECK_FLOAT(player.time, 0.75f);
    SetSpratAnimationSpeed(&player, 1.0f);
    StopSpratAnimation(&player);
    CHECK_FLOAT(player.time, 0);

    // Times set by hand: any value maps to a frame, or to none
    player = make_test_player(0, 3);
    player.time = -0.1f;
    CHECK_INT(GetSpratAnimationFrame(player), 9);
    player.time = 100.0f;                           // step 400, past 133 loops
    CHECK_INT(GetSpratAnimationFrame(player), 8);
    player.time = NAN;
    CHECK_INT(GetSpratAnimationFrame(player), -1);
    player.animation.loop = SPRAT_LOOP_MODE_ONCE;
    player.time = 100.0f;
    CHECK_INT(GetSpratAnimationFrame(player), 9);
    player.time = -100.0f;
    CHECK_INT(GetSpratAnimationFrame(player), 7);

    // A zeroed animation (not found) plays nothing
    SpratAnimation none = { 0 };
    player = MakeSpratAnimationPlayer(none);
    CHECK(!IsSpratAnimationValid(none));
    UpdateSpratAnimation(&player, 1.0f);
    PlaySpratAnimation(&player);
    CHECK_INT(GetSpratAnimationFrame(player), -1);
    CHECK(!IsSpratAnimationFinished(player));
}

/* Setting the animation played restarts only on a change, keeping the speed. */
static void test_set_animation(void)
{
    SpratAnimationPlayer player = make_test_player(0, 3);
    SpratAnimation walk = player.animation;
    SpratAnimation idle = walk;
    idle.name = "idle";
    idle.frameCount = 1;

    SetSpratAnimationSpeed(&player, 2.0f);
    UpdateSpratAnimation(&player, 0.125f);
    CHECK_INT(GetSpratAnimationFrame(player), 8);

    SetSpratAnimation(&player, walk);               // the one played: nothing changes
    CHECK_FLOAT(player.time, 0.25);
    SpratAnimation once = walk;
    once.loop = SPRAT_LOOP_MODE_ONCE;
    once.flip = SPRAT_FLIP_VERTICAL;
    SetSpratAnimation(&player, once);               // a copy played another way still is
    CHECK_FLOAT(player.time, 0.25);
    CHECK_INT(player.animation.loop, 0);

    SetSpratAnimation(&player, idle);               // renamed: replaced, from its start
    CHECK_STR(player.animation.name, "idle");
    CHECK_FLOAT(player.time, 0);
    CHECK_FLOAT(player.speed, 2);
    CHECK(IsSpratAnimationPlaying(player));
    CHECK_INT(GetSpratAnimationFrame(player), 7);

    SpratAnimation shorter = walk;
    shorter.frameCount = 2;
    SetSpratAnimation(&player, walk);
    UpdateSpratAnimation(&player, 0.125f);
    SetSpratAnimation(&player, shorter);            // other frame count: replaced
    CHECK_INT(player.animation.frameCount, 2);
    CHECK_FLOAT(player.time, 0);

    StopSpratAnimation(&player);
    SetSpratAnimation(&player, shorter);            // stopped stays stopped
    CHECK(!IsSpratAnimationPlaying(player));
    SetSpratAnimation(&player, walk);               // a new one plays
    CHECK(IsSpratAnimationPlaying(player));

    SetSpratAnimationSpeed(&player, -1.0f);
    SetSpratAnimation(&player, idle);               // reversed: from its end
    CHECK_FLOAT(player.time, 0.25);
    CHECK_INT(GetSpratAnimationFrame(player), 7);

    SetSpratAnimation(NULL, walk);                  // ignored
}

/* A sprite follows its player's frame and flip; the drawn animation sprite combines
 * the given transform with them. */
static void test_animation_sprite(void)
{
    SpratSheet sheet = { 0 };
    SpratAnimationPlayer player = make_test_player(0, 3);
    player.animation.flip = SPRAT_FLIP_HORIZONTAL;

    SpratSprite sprite = MakeSpratSprite(sheet, 0, (Vector2){ 5, 6 });
    sprite.scale = (Vector2){ -2, 3 };
    sprite.rotation = 45.0f;
    sprite.flip = SPRAT_FLIP_VERTICAL;
    SetSpratSpriteAnimationFrame(&sprite, player);
    CHECK_INT(sprite.frame, 7);
    CHECK_INT(sprite.flip, SPRAT_FLIP_HORIZONTAL);  // replaced, not combined
    CHECK_FLOAT(sprite.position.x, 5);              // the rest is the program's
    CHECK_FLOAT(sprite.scale.x, -2);
    CHECK_FLOAT(sprite.rotation, 45);

    UpdateSpratAnimation(&player, 0.5f);
    SetSpratSpriteAnimationFrame(&sprite, player);
    CHECK_INT(sprite.frame, 9);
    SetSpratSpriteAnimationFrame(&sprite, player);  // idempotent
    CHECK_INT(sprite.frame, 9);
    CHECK_INT(sprite.flip, SPRAT_FLIP_HORIZONTAL);

    SetSpratSpriteAnimationFrame(NULL, player);     // ignored
    SetSpratSpriteAnimationFrame(&sprite, make_test_player(0, 0));
    CHECK_INT(sprite.frame, -1);                    // nothing shown: drawn as nothing

    SpratSprite drawn = SpratMakeAnimationSprite(sheet, player, (Vector2){ 1, 2 }, (Vector2){ 3, 4 }, 30.0f,
                                                 SPRAT_FLIP_HORIZONTAL | SPRAT_FLIP_VERTICAL);
    CHECK_INT(drawn.frame, 9);
    CHECK_INT(drawn.flip, SPRAT_FLIP_VERTICAL);     // the animation's horizontal flip undone
    CHECK_FLOAT(drawn.position.x, 1);
    CHECK_FLOAT(drawn.position.y, 2);
    CHECK_FLOAT(drawn.scale.x, 3);
    CHECK_FLOAT(drawn.scale.y, 4);
    CHECK_FLOAT(drawn.rotation, 30);

    drawn = SpratMakeAnimationSprite(sheet, player, (Vector2){ 1, 2 }, (Vector2){ 1, 1 }, 0.0f, 0);
    CHECK_INT(drawn.flip, SPRAT_FLIP_HORIZONTAL);   // DrawSpratAnimation(): the animation's flip

    UpdateSpratSpriteAnimation(&sprite, &player, 0.25f);    // advanced, then set: wraps to 7
    CHECK_INT(sprite.frame, 7);
    CHECK_FLOAT(player.time, 0);
    sprite.flip = 0;
    StopSpratAnimation(&player);
    UpdateSpratAnimation(&player, 0.25f);
    UpdateSpratSpriteAnimation(&sprite, &player, 0.0f);     // only sets the sprite
    CHECK_INT(sprite.frame, 7);
    CHECK_INT(sprite.flip, SPRAT_FLIP_HORIZONTAL);
    ResumeSpratAnimation(&player);
    UpdateSpratSpriteAnimation(NULL, &player, 0.25f);       // no sprite: the player still advances
    CHECK_FLOAT(player.time, 0.25);
    UpdateSpratSpriteAnimation(&sprite, NULL, 0.25f);       // ignored
    CHECK_INT(sprite.frame, 7);
}

static void test_animation_pingpong(void)
{
    // Repeating: 7 8 9 8, then 7 again; one pass lasts 1 s
    SpratAnimationPlayer player = make_test_player(SPRAT_LOOP_MODE_PINGPONG, 3);
    const int forward[] = { 7, 8, 9, 8, 7, 8, 9, 8, 7 };
    CHECK_FRAMES(&player, forward);
    CHECK(IsSpratAnimationPlaying(player));
    CHECK(!IsSpratAnimationFinished(player));
    CHECK_FLOAT(player.time, 0.25f);

    // In reverse: the same pass backwards, from its last step
    SetSpratAnimationSpeed(&player, -1.0f);
    PlaySpratAnimation(&player);
    CHECK_FLOAT(player.time, 1.0f);
    const int backward[] = { 8, 9, 8, 7, 8, 9 };
    CHECK_FRAMES(&player, backward);

    // Once: 7 8 9 8 7, stopping on the first frame after 1.25 s
    player = make_test_player(SPRAT_LOOP_MODE_ONCE | SPRAT_LOOP_MODE_PINGPONG, 3);
    const int once[] = { 7, 8, 9, 8, 7 };
    CHECK_FRAMES(&player, once);
    CHECK(!IsSpratAnimationPlaying(player));
    CHECK(IsSpratAnimationFinished(player));
    CHECK_FLOAT(player.time, 1.25f);
    CHECK_INT(GetSpratAnimationFrame(player), 7);
    player.time = 100.0f;
    CHECK_INT(GetSpratAnimationFrame(player), 7);

    // Once in reverse: the same frames, stopping on the first frame at time 0
    SetSpratAnimationSpeed(&player, -1.0f);
    PlaySpratAnimation(&player);
    CHECK_FRAMES(&player, once);    // a palindrome
    CHECK(IsSpratAnimationFinished(player));
    CHECK_FLOAT(player.time, 0);
    CHECK_INT(GetSpratAnimationFrame(player), 7);

    // Two frames: 7 8 repeating, or 7 8 7 once; one frame has no way back
    player = make_test_player(SPRAT_LOOP_MODE_PINGPONG, 2);
    player.time = 0.5f;
    CHECK_INT(GetSpratAnimationFrame(player), 7);
    CHECK_FLOAT(SpratGetAnimationDuration(player.animation), 0.5f);
    player = make_test_player(SPRAT_LOOP_MODE_ONCE | SPRAT_LOOP_MODE_PINGPONG, 2);
    CHECK_FLOAT(SpratGetAnimationDuration(player.animation), 0.75f);
    player.time = 0.5f;
    CHECK_INT(GetSpratAnimationFrame(player), 7);
    player.time = 0.25f;
    CHECK_INT(GetSpratAnimationFrame(player), 8);
    player = make_test_player(SPRAT_LOOP_MODE_PINGPONG, 1);
    CHECK_FLOAT(SpratGetAnimationDuration(player.animation), 0.25f);
    player.time = 0.6f;
    CHECK_INT(GetSpratAnimationFrame(player), 7);

    // The pass of the largest frame count doesn't overflow
    SpratAnimation huge = { "huge", 1, NULL, INT_MAX, 0, SPRAT_LOOP_MODE_ONCE | SPRAT_LOOP_MODE_PINGPONG };
    CHECK(SpratGetAnimationStepCount(huge) == 2.0*INT_MAX - 1.0);
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
    static const Vector2 scales[] = { { 1, 1 }, { 3, 3 }, { 2, 3 }, { -2, 1 }, { 1, -3 }, { -1, -2 } };
    static const float turns[] = { 0, 90, 180, 270, -90 };

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
        int area = (int)(cw*ch*fabs(scales[s].x*scales[s].y));
        if (mismatches != 0 || drawn != area)
        {
            fprintf(stderr, "image %zu rotation %d flip %d scale %gx%g turn %g: %d pixels differ, %d of %d drawn\n",
                    i, rotations[r], flip, scales[s].x, scales[s].y, turns[t], mismatches, drawn, area);
            failures++;
        }
    }
}

static void test_sprite_point_matches_draw(void)
{
    static const image_case img = { 10, 6, 1, 2, 3, 0, 5, 3 };
    static const Vector2 scales[] = { { 2, 2 }, { -2, 2 }, { 2, -3 }, { -3, -2 } };
    static const float turns[] = { 0, 90, 33, -150 };
    SpratSlice noSlice = { 0 };
    SpratSheet sheet = { 0 };
    SpratFrame frame = pack(&img, 90, noSlice);

    sheet.frames = &frame;
    sheet.frameCount = 1;

    SpratSprite sprite = MakeSpratSprite(sheet, 0, (Vector2){ 40, 30 });
    CHECK_FLOAT(sprite.scale.x, 1);
    CHECK_FLOAT(sprite.scale.y, 1);
    CHECK_FLOAT(sprite.rotation, 0);
    CHECK_INT(sprite.flip, 0);

    // The pivot lands on the position, whatever the transform
    sprite.scale = (Vector2){ 2, 3 };
    sprite.rotation = 33.0f;
    sprite.flip = SPRAT_FLIP_HORIZONTAL;
    Vector2 p = GetSpratSpritePoint(sprite, frame.pivot);
    CHECK_FLOAT(p.x, 40);
    CHECK_FLOAT(p.y, 30);

    // A point inside content pixel (6, 4) lands on the screen pixel that shows it
    for (size_t s = 0; s < COUNT(scales); s++)
    for (size_t t = 0; t < COUNT(turns); t++)
    for (int flip = 0; flip < 4; flip++)
    {
        sprite.position = (Vector2){ 80, 70 };
        sprite.scale = scales[s];
        sprite.rotation = turns[t];
        sprite.flip = flip;
        p = GetSpratSpritePoint(sprite, (Vector2){ 6.5f, 4.5f });
        quad q = make_quad(SpratGetFrameDrawCall(&frame, sprite.position, sprite.scale, sprite.rotation, sprite.flip));
        if (engine_pixel(&q, (int)floor(p.x), (int)floor(p.y)) != original_pixel(6, 4))
        {
            fprintf(stderr, "sprite point scale %gx%g turn %g flip %d: lands on another pixel\n", scales[s].x, scales[s].y, turns[t], flip);
            failures++;
        }
    }

    // Out of range: the sprite's position
    sprite.frame = 1;
    CHECK_FLOAT(GetSpratSpritePoint(sprite, frame.pivot).x, 80);
}

/* ---- Markers -------------------------------------------------------------- */

/* Two frames to place markers with: frame 0's pivot is its top-left, frame 1's is (3, -2). */
static SpratFrame markerFrames[2];
static SpratSheet markerSheet;

static void make_marker_sheet(void)
{
    memset(markerFrames, 0, sizeof(markerFrames));
    markerFrames[1].pivot = (Vector2){ 3, -2 };
    memset(&markerSheet, 0, sizeof(markerSheet));
    markerSheet.frameCount = 2;
    markerSheet.frames = markerFrames;
    markerSheet.scale = 1.0f;
}

/* A small generator, so the random cases are the same everywhere. */
static unsigned int randomState = 1;

static void seed_random(unsigned int seed)
{
    randomState = seed;
}

static float random_float(void)
{
    randomState = randomState*1664525u + 1013904223u;
    return (float)(randomState >> 8)/16777216.0f;
}

static int random_int(int n)
{
    return (int)(random_float()*(float)n);
}

static SpratMarker point_marker(float x, float y)
{
    SpratMarker m = { 0 };
    m.name = "point";
    m.type = SPRAT_MARKER_TYPE_POINT;
    m.position = (Vector2){ x, y };
    return m;
}

static SpratMarker circle_marker(float x, float y, float radius)
{
    SpratMarker m = point_marker(x, y);
    m.name = "circle";
    m.type = SPRAT_MARKER_TYPE_CIRCLE;
    m.radius = radius;
    return m;
}

static SpratMarker rect_marker(float x, float y, float w, float h)
{
    SpratMarker m = point_marker(x, y);
    m.name = "rect";
    m.type = SPRAT_MARKER_TYPE_RECTANGLE;
    m.size = (Vector2){ w, h };
    return m;
}

static SpratMarker polygon_marker(Vector2 *vertices, int vertexCount)
{
    SpratMarker m = point_marker(vertices[0].x, vertices[0].y);
    m.name = "polygon";
    m.type = SPRAT_MARKER_TYPE_POLYGON;
    m.vertices = vertices;
    m.vertexCount = vertexCount;
    return m;
}

/* A sprite of frame 0 of the marker sheet. */
static SpratSprite placed(float x, float y, float scaleX, float scaleY, float rotation, int flip)
{
    SpratSprite sprite = MakeSpratSprite(markerSheet, 0, (Vector2){ x, y });
    sprite.scale = (Vector2){ scaleX, scaleY };
    sprite.rotation = rotation;
    sprite.flip = flip;
    return sprite;
}

static SpratSprite at(float x, float y)
{
    return placed(x, y, 1, 1, 0, 0);
}

/* The sprite, showing another frame. */
static SpratSprite on_frame(int frame, SpratSprite sprite)
{
    sprite.frame = frame;
    return sprite;
}

/* Checks whether markers of two sprites collide, in both orders, and for a
 * point marker, that the point check agrees. */
#define CHECK_COLLISION(markerA, spriteA, markerB, spriteB, expected) \
    check_collision(__LINE__, (markerA), (spriteA), (markerB), (spriteB), (expected))

static void check_collision(int line, SpratMarker a, SpratSprite sa, SpratMarker b, SpratSprite sb, bool expected)
{
    bool ab = CheckCollisionSpratMarkers(sa, &a, sb, &b);
    bool ba = CheckCollisionSpratMarkers(sb, &b, sa, &a);

    if (ab != expected || ba != expected)
    {
        fprintf(stderr, "%s:%d: %s and %s collide: %d, the other way %d, expected %d\n", __FILE__, line, a.name, b.name, ab, ba, expected);
        failures++;
    }

    if (a.type == SPRAT_MARKER_TYPE_POINT && CheckCollisionPointSpratMarker(GetSpratSpritePoint(sa, a.position), sb, &b) != expected)
    {
        fprintf(stderr, "%s:%d: the point check of %s disagrees\n", __FILE__, line, b.name);
        failures++;
    }
}

static void test_markers_collide(void)
{
    static Vector2 uShape[] = {     /* a U, open at the top: the notch is x 3..7, y 0..7 */
        { 0, 0 }, { 3, 0 }, { 3, 7 }, { 7, 7 }, { 7, 0 }, { 10, 0 }, { 10, 10 }, { 0, 10 }
    };
    static Vector2 segment[] = { { -5, 5 }, { 15, 5 } };
    SpratMarker square = rect_marker(0, 0, 10, 10);
    SpratMarker small = rect_marker(0, 0, 2, 2);
    SpratMarker origin = point_marker(0, 0);
    SpratMarker circle = circle_marker(0, 0, 5);
    SpratMarker unit = circle_marker(0, 0, 1);
    SpratMarker u = polygon_marker(uShape, COUNT(uShape));
    SpratMarker line = polygon_marker(segment, 2);
    SpratMarker vertex = polygon_marker(segment, 1);
    SpratMarker wide = rect_marker(-10, -1, 20, 2);
    SpratMarker tall = rect_marker(-1, -10, 2, 20);
    SpratMarker bar = rect_marker(2, 0, 3, 1);
    SpratMarker centered = rect_marker(-1, -1, 2, 2);
    SpratMarker strip = rect_marker(0, 0, 10, 2);
    SpratMarker dot = circle_marker(0, 0, 0);

    make_marker_sheet();

    // Rectangles: crossing, touching on an edge or a corner, apart, one inside the other
    CHECK_COLLISION(square, at(0, 0), square, at(5, 5), true);
    CHECK_COLLISION(square, at(0, 0), square, at(10, 0), true);
    CHECK_COLLISION(square, at(0, 0), square, at(10, 10), true);
    CHECK_COLLISION(square, at(0, 0), square, at(10.5f, 0), false);
    CHECK_COLLISION(square, at(0, 0), square, at(-10.5f, -3), false);
    CHECK_COLLISION(square, at(0, 0), small, at(4, 4), true);
    CHECK_COLLISION(wide, at(0, 0), tall, at(0, 0), true);         // a cross: no corner inside the other

    // Points: in a shape, on its edge, outside, on each other
    CHECK_COLLISION(origin, at(5, 5), square, at(0, 0), true);
    CHECK_COLLISION(origin, at(10, 5), square, at(0, 0), true);
    CHECK_COLLISION(origin, at(11, 5), square, at(0, 0), false);
    CHECK_COLLISION(origin, at(3, 4), origin, at(3, 4), true);
    CHECK_COLLISION(origin, at(3, 4), origin, at(3, 4.5f), false);
    CHECK_COLLISION(origin, at(3, 4), circle, at(0, 0), true);      // on the circle: 3-4-5
    CHECK_COLLISION(origin, at(3, 4.1f), circle, at(0, 0), false);

    // Circles: crossing, touching, apart, inside
    CHECK_COLLISION(circle, at(0, 0), circle, at(10, 0), true);
    CHECK_COLLISION(circle, at(0, 0), circle, at(6, 8), true);
    CHECK_COLLISION(circle, at(0, 0), circle, at(6.1f, 8), false);
    CHECK_COLLISION(circle, at(0, 0), unit, at(1, 1), true);

    // A circle and a rectangle: along an edge, at a corner, each inside the other
    CHECK_COLLISION(circle, at(0, 0), square, at(5, -5), true);
    CHECK_COLLISION(circle, at(0, 0), square, at(5.01f, -5), false);
    CHECK_COLLISION(circle, at(0, 0), square, at(3, 4), true);     // the corner on the circle
    CHECK_COLLISION(circle, at(0, 0), square, at(4, 4), false);    // the corner 5.66 away
    CHECK_COLLISION(unit, at(5, 5), square, at(0, 0), true);
    CHECK_COLLISION(circle, at(0, 0), small, at(-1, -1), true);

    // A concave polygon: its notch is outside it
    CHECK_COLLISION(origin, at(5, 3), u, at(0, 0), false);
    CHECK_COLLISION(origin, at(1, 3), u, at(0, 0), true);
    CHECK_COLLISION(origin, at(5, 8), u, at(0, 0), true);
    CHECK_COLLISION(unit, at(5, 3), u, at(0, 0), false);
    CHECK_COLLISION(unit, at(5, 6), u, at(0, 0), true);            // reaches the notch's floor at y 7
    CHECK_COLLISION(small, at(4, 2), u, at(0, 0), false);
    CHECK_COLLISION(small, at(6, 2), u, at(0, 0), true);           // touches the notch's side at x 7

    // Polygons of one and two vertices: a point and a segment
    CHECK_COLLISION(line, at(0, 0), square, at(0, 0), true);       // both ends outside
    CHECK_COLLISION(line, at(0, 6), square, at(0, 0), false);
    CHECK_COLLISION(vertex, at(5, 0), square, at(0, 0), true);     // (0, 5)
    CHECK_COLLISION(vertex, at(-0.5f, 0), square, at(0, 0), false);

    // Flipping mirrors around the pivot; a negative scale does too, and both undo each other
    CHECK_COLLISION(origin, at(-3, 0.5f), bar, at(0, 0), false);
    CHECK_COLLISION(origin, at(-3, 0.5f), bar, placed(0, 0, 1, 1, 0, SPRAT_FLIP_HORIZONTAL), true);
    CHECK_COLLISION(origin, at(-3, 0.5f), bar, placed(0, 0, -1, 1, 0, 0), true);
    CHECK_COLLISION(origin, at(-3, 0.5f), bar, placed(0, 0, -1, 1, 0, SPRAT_FLIP_HORIZONTAL), false);
    CHECK_COLLISION(origin, at(3, -0.5f), bar, placed(0, 0, 1, 1, 0, SPRAT_FLIP_VERTICAL), true);

    // Turning: clockwise around the pivot, exact at quarter turns
    CHECK_COLLISION(origin, at(-1, 5), strip, placed(0, 0, 1, 1, 90, 0), true);
    CHECK_COLLISION(origin, at(-2, 10), strip, placed(0, 0, 1, 1, 90, 0), true);    // a corner
    CHECK_COLLISION(origin, at(5, 1), strip, placed(0, 0, 1, 1, 90, 0), false);
    CHECK_COLLISION(origin, at(-5, -1), strip, placed(0, 0, 1, 1, -180, 0), true);
    CHECK_COLLISION(origin, at(1.3f, 0), centered, placed(0, 0, 1, 1, 45, 0), true);
    CHECK_COLLISION(origin, at(0.9f, 0.9f), centered, placed(0, 0, 1, 1, 45, 0), false);

    // Scaling; a circle scaled unevenly is an ellipse, which turns with its frame
    CHECK_COLLISION(origin, at(6, 6), small, placed(0, 0, 3, 3, 0, 0), true);
    CHECK_COLLISION(origin, at(6.5f, 6), small, placed(0, 0, 3, 3, 0, 0), false);
    CHECK_COLLISION(origin, at(3, 0.6f), unit, placed(0, 0, 4, 1, 0, 0), true);    // 0.9225
    CHECK_COLLISION(origin, at(3, 0.7f), unit, placed(0, 0, 4, 1, 0, 0), false);   // 1.05
    CHECK_COLLISION(origin, at(0.6f, 3), unit, placed(0, 0, 4, 1, 90, 0), true);
    CHECK_COLLISION(origin, at(3, 0.6f), unit, placed(0, 0, 4, 1, 90, 0), false);
    CHECK_COLLISION(unit, placed(0, 0, 4, 1, 0, 0), unit, placed(4.5f, 0, 1, 4, 0, 0), true);
    CHECK_COLLISION(unit, placed(0, 0, 4, 1, 0, 0), unit, placed(5.1f, 0, 1, 4, 0, 0), false);
    CHECK_COLLISION(unit, placed(0, 0, 4, 1, 90, 0), unit, at(1.9f, 0), true);
    CHECK_COLLISION(unit, placed(0, 0, 4, 1, 90, 0), unit, at(2.1f, 0), false);
    CHECK_COLLISION(unit, placed(0, 0, 4, 1, 90, 0), unit, at(0, 4.9f), true);
    CHECK_COLLISION(unit, placed(0, 0, 2, 2, 0, 0), unit, placed(3, 0, 1, 1, 0, 0), true);  // touching

    // Scaled to 0: flattened to a segment or a point, which still collides with what it touches
    CHECK_COLLISION(origin, at(0, 1), centered, placed(0, 0, 0, 1, 0, 0), true);
    CHECK_COLLISION(origin, at(0.1f, 0), centered, placed(0, 0, 0, 1, 0, 0), false);
    CHECK_COLLISION(origin, at(0, 1.9f), circle_marker(0, 0, 2), placed(0, 0, 0, 1, 0, 0), true);
    CHECK_COLLISION(origin, at(0, 2.1f), circle_marker(0, 0, 2), placed(0, 0, 0, 1, 0, 0), false);
    CHECK_COLLISION(origin, at(1.9f, 0), circle_marker(0, 0, 2), placed(0, 0, 1, 0, 0, 0), true);
    CHECK_COLLISION(unit, placed(5, 5, 0, 0, 0, 0), square, at(0, 0), true);
    CHECK_COLLISION(unit, placed(5, 5, 0, 0, 0, 0), unit, placed(5, 5, 0, 0, 0, 0), true);
    CHECK_COLLISION(unit, placed(0, 0, 0, 1, 0, 0), unit, placed(0.5f, 0, 1, 0, 0, 0), true);
    CHECK_COLLISION(dot, at(3, 4), circle, at(0, 0), true);        // a radius of 0 is a point
    CHECK_COLLISION(dot, at(3, 4.1f), circle, at(0, 0), false);

    // Each marker is placed by its own frame's pivot: frame 1's (3, -2)
    CHECK_COLLISION(origin, on_frame(1, at(0, 0)), origin, at(-3, 2), true);
    CHECK_COLLISION(origin, on_frame(1, at(0, 0)), origin, on_frame(1, at(0, 0)), true);
    CHECK_COLLISION(origin, on_frame(1, at(0, 0)), origin, at(0, 0), false);
    CHECK_COLLISION(small, on_frame(1, placed(0, 0, 1, 1, 0, SPRAT_FLIP_HORIZONTAL)), origin, at(2, 3), true);   // x 1..3, y 2..4
    CHECK_COLLISION(small, on_frame(1, placed(0, 0, 1, 1, 0, SPRAT_FLIP_HORIZONTAL)), origin, at(4, 3), false);

    // Nothing collides with a missing or bad marker, or on a frame out of range
    SpratSprite sprite = at(0, 0);
    CHECK(!CheckCollisionSpratMarkers(sprite, NULL, sprite, &origin));
    CHECK(!CheckCollisionSpratMarkers(sprite, &origin, sprite, NULL));
    CHECK(!CheckCollisionSpratMarkers(on_frame(2, sprite), &origin, sprite, &origin));
    CHECK(!CheckCollisionSpratMarkers(sprite, &origin, on_frame(-1, sprite), &origin));
    CHECK(!CheckCollisionPointSpratMarker((Vector2){ 0, 0 }, sprite, NULL));
    CHECK(!CheckCollisionPointSpratMarker((Vector2){ 0, 0 }, on_frame(2, sprite), &origin));
    SpratMarker bad = polygon_marker(uShape, 0);
    CHECK(!CheckCollisionSpratMarkers(sprite, &origin, sprite, &bad));
    bad = point_marker(0, 0);
    bad.type = 9;
    CHECK(!CheckCollisionSpratMarkers(sprite, &bad, sprite, &origin));

    // An empty sheet has no frames to place markers with
    SpratSprite none = { 0 };
    CHECK(!CheckCollisionSpratMarkers(none, &origin, none, &origin));
}

/* A reference for the collision test, written apart from the library: points
 * of one marker, sampled densely, are looked up in the other through the
 * inverse of its sprite's placement. */

#define SAMPLE_STEP 0.25    /* between samples, in original-image pixels */
#define PI_D 3.14159265358979323846

typedef struct ref_point {
    double x, y;
} ref_point;

static void get_mirrored_scale(SpratSprite sprite, double *sx, double *sy)
{
    *sx = (sprite.flip & SPRAT_FLIP_HORIZONTAL)? -sprite.scale.x : sprite.scale.x;
    *sy = (sprite.flip & SPRAT_FLIP_VERTICAL)? -sprite.scale.y : sprite.scale.y;
}

static ref_point to_screen(SpratSprite sprite, double x, double y)
{
    Vector2 pivot = sprite.sheet.frames[sprite.frame].pivot;
    double radians = sprite.rotation*PI_D/180.0;
    double sx, sy;

    get_mirrored_scale(sprite, &sx, &sy);
    double dx = (x - pivot.x)*sx;
    double dy = (y - pivot.y)*sy;
    ref_point r = { sprite.position.x + dx*cos(radians) - dy*sin(radians), sprite.position.y + dx*sin(radians) + dy*cos(radians) };
    return r;
}

static ref_point to_original(SpratSprite sprite, ref_point s)
{
    Vector2 pivot = sprite.sheet.frames[sprite.frame].pivot;
    double radians = sprite.rotation*PI_D/180.0;
    double dx = s.x - sprite.position.x, dy = s.y - sprite.position.y;
    double sx, sy;

    get_mirrored_scale(sprite, &sx, &sy);
    ref_point r = { (dx*cos(radians) + dy*sin(radians))/sx + pivot.x, (-dx*sin(radians) + dy*cos(radians))/sy + pivot.y };
    return r;
}

static double distance_to_segment(ref_point p, double ax, double ay, double bx, double by)
{
    double dx = bx - ax, dy = by - ay;
    double length = dx*dx + dy*dy;
    double t = (length > 0.0)? fmin(fmax(((p.x - ax)*dx + (p.y - ay)*dy)/length, 0.0), 1.0) : 0.0;
    double ex = ax + t*dx - p.x, ey = ay + t*dy - p.y;

    return sqrt(ex*ex + ey*ey);
}

/* How far an original-image point is inside a marker: positive inside, negative outside. */
static double depth_in_marker(const SpratMarker *m, ref_point p)
{
    double x0 = m->position.x, y0 = m->position.y;

    switch (m->type)
    {
        case SPRAT_MARKER_TYPE_CIRCLE:
            return m->radius - sqrt((p.x - x0)*(p.x - x0) + (p.y - y0)*(p.y - y0));
        case SPRAT_MARKER_TYPE_RECTANGLE:
        {
            double inside = fmin(fmin(p.x - x0, x0 + m->size.x - p.x), fmin(p.y - y0, y0 + m->size.y - p.y));
            double ox = fmax(fmax(x0 - p.x, p.x - (x0 + m->size.x)), 0.0);
            double oy = fmax(fmax(y0 - p.y, p.y - (y0 + m->size.y)), 0.0);
            return (inside >= 0.0)? inside : -sqrt(ox*ox + oy*oy);
        }
        case SPRAT_MARKER_TYPE_POLYGON:
        {
            double nearest = 1e30;
            bool inside = false;

            for (int i = 0; i < m->vertexCount; i++)
            {
                Vector2 a = m->vertices[i];
                Vector2 b = m->vertices[(i + 1)%m->vertexCount];

                nearest = fmin(nearest, distance_to_segment(p, a.x, a.y, b.x, b.y));
                if (m->vertexCount >= 3 && (a.y > p.y) != (b.y > p.y) &&
                    p.x < (double)(b.x - a.x)*(p.y - a.y)/(double)(b.y - a.y) + a.x) inside = !inside;
            }
            return inside? nearest : -nearest;
        }
        default:
            return -sqrt((p.x - x0)*(p.x - x0) + (p.y - y0)*(p.y - y0));
    }
}

/* Samples a marker's points, in original-image pixels: its outline and its inside. */
static int sample_marker(const SpratMarker *m, ref_point *samples, int capacity)
{
    double x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    int count = 0;

#define ADD_SAMPLE(px, py) do { if (count < capacity) { samples[count].x = (px); samples[count].y = (py); count++; } } while (0)

    switch (m->type)
    {
        case SPRAT_MARKER_TYPE_POINT:
            ADD_SAMPLE(m->position.x, m->position.y);
            return count;
        case SPRAT_MARKER_TYPE_CIRCLE:
        {
            int steps = (int)ceil(2.0*PI_D*m->radius/SAMPLE_STEP) + 1;
            for (int k = 0; k < steps; k++)
            {
                ADD_SAMPLE(m->position.x + m->radius*cos(2.0*PI_D*k/steps), m->position.y + m->radius*sin(2.0*PI_D*k/steps));
            }
            x0 = m->position.x - m->radius;
            y0 = m->position.y - m->radius;
            x1 = m->position.x + m->radius;
            y1 = m->position.y + m->radius;
        } break;
        case SPRAT_MARKER_TYPE_RECTANGLE:
            x0 = m->position.x;
            y0 = m->position.y;
            x1 = x0 + m->size.x;
            y1 = y0 + m->size.y;
            break;
        default:
            x0 = x1 = m->vertices[0].x;
            y0 = y1 = m->vertices[0].y;
            for (int i = 0; i < m->vertexCount; i++)
            {
                Vector2 a = m->vertices[i];
                Vector2 b = m->vertices[(i + 1)%m->vertexCount];
                double length = sqrt((double)(b.x - a.x)*(b.x - a.x) + (double)(b.y - a.y)*(b.y - a.y));
                int steps = (int)ceil(length/SAMPLE_STEP) + 1;

                for (int k = 0; k <= steps; k++)
                {
                    ADD_SAMPLE(a.x + (b.x - a.x)*(double)k/steps, a.y + (b.y - a.y)*(double)k/steps);
                }
                x0 = fmin(x0, a.x);
                y0 = fmin(y0, a.y);
                x1 = fmax(x1, a.x);
                y1 = fmax(y1, a.y);
            }
            break;
    }

    // The inside, and for a rectangle its outline too, on a grid that ends on its far edges
    for (double y = y0;; y = fmin(y + SAMPLE_STEP, y1))
    {
        for (double x = x0;; x = fmin(x + SAMPLE_STEP, x1))
        {
            ref_point p = { x, y };
            if (depth_in_marker(m, p) >= 0.0) ADD_SAMPLE(x, y);
            if (x >= x1) break;
        }
        if (y >= y1) break;
    }
#undef ADD_SAMPLE
    return count;
}

static SpratMarker random_marker(Vector2 vertices[8])
{
    float x = random_float()*8.0f - 4.0f, y = random_float()*8.0f - 4.0f;

    switch (random_int(6))
    {
        case 0: return point_marker(x, y);
        case 1: return circle_marker(x, y, 0.5f + random_float()*3.5f);
        case 2: return rect_marker(x, y, 0.5f + random_float()*5.5f, 0.5f + random_float()*5.5f);
        case 3:
            // A segment
            vertices[0] = (Vector2){ x, y };
            vertices[1] = (Vector2){ x + random_float()*8.0f - 4.0f, y + random_float()*8.0f - 4.0f };
            return polygon_marker(vertices, 2);
        default:
        {
            // Star-shaped around (x, y), so simple, and often concave
            int count = 3 + random_int(6);
            for (int i = 0; i < count; i++)
            {
                double angle = 2.0*PI_D*(i + 0.2 + 0.6*random_float())/count;
                float radius = 0.5f + random_float()*4.0f;
                vertices[i] = (Vector2){ x + radius*(float)cos(angle), y + radius*(float)sin(angle) };
            }
            return polygon_marker(vertices, count);
        }
    }
}

static SpratSprite random_sprite(int frame, float spread)
{
    static const float scales[] = { 1.0f, 2.0f, 0.5f, -1.0f, 1.5f, 0.75f };
    static const float turns[] = { 0, 90, 180, 270 };
    float x = random_float()*2.0f*spread - spread;
    float y = random_float()*2.0f*spread - spread;
    float scaleX = scales[random_int(COUNT(scales))];
    float scaleY = scales[random_int(COUNT(scales))];
    float rotation = random_int(2)? turns[random_int(COUNT(turns))] : random_float()*360.0f;
    int flip = random_int(4);

    return on_frame(frame, placed(x, y, scaleX, scaleY, rotation, flip));
}

/* Random markers on random sprites: when they collide, a sample of one is
 * within the sampling distance of the other; when they don't, no sample is
 * inside it. */
static void test_markers_collide_match_reference(void)
{
    enum { CASES = 3000, CAPACITY = 4096 };
    static ref_point samples[CAPACITY];
    int disagreements = 0, collisions = 0;

    make_marker_sheet();
    seed_random(20261026);

    for (int i = 0; i < CASES; i++)
    {
        Vector2 verticesA[8], verticesB[8];
        SpratMarker ma = random_marker(verticesA);
        SpratMarker mb = random_marker(verticesB);
        int fa = random_int(2), fb = random_int(2);

        if (mb.type == SPRAT_MARKER_TYPE_POINT) continue;     // a sample never lands exactly on a point

        SpratSprite a = random_sprite(fa, 1.0f);
        SpratSprite b = random_sprite(fb, 5.0f);
        bool collision = CheckCollisionSpratMarkers(a, &ma, b, &mb);

        if (collision != CheckCollisionSpratMarkers(b, &mb, a, &ma))
        {
            fprintf(stderr, "collision case %d: the result depends on the order\n", i);
            failures++;
        }
        collisions += collision;

        // Neighbouring samples of a are reach apart on the screen, at most tolerance in b's original image
        double sxa, sya, sxb, syb;
        get_mirrored_scale(a, &sxa, &sya);
        get_mirrored_scale(b, &sxb, &syb);
        double reach = SAMPLE_STEP*fmax(fabs(sxa), fabs(sya))*1.5;
        double tolerance = reach/fmin(fabs(sxb), fabs(syb));

        int count = sample_marker(&ma, samples, CAPACITY);
        bool near = false, inside = false;
        CHECK(count < CAPACITY);
        for (int s = 0; s < count; s++)
        {
            double depth = depth_in_marker(&mb, to_original(b, to_screen(a, samples[s].x, samples[s].y)));
            near = near || depth >= -tolerance;
            inside = inside || depth > 1e-6;
        }

        if ((collision && !near) || (!collision && inside))
        {
            if (disagreements++ < 10)
            {
                fprintf(stderr, "collision case %d: %s on frame %d at %g,%g scale %gx%g turn %g flip %d, %s on frame %d at %g,%g scale %gx%g turn %g flip %d: "
                        "collision is %d, the reference finds a sample %s\n", i,
                        ma.name, fa, a.position.x, a.position.y, a.scale.x, a.scale.y, a.rotation, a.flip,
                        mb.name, fb, b.position.x, b.position.y, b.scale.x, b.scale.y, b.rotation, b.flip,
                        collision, collision? "nowhere near" : "inside");
            }
        }
    }

    CHECK_INT(disagreements, 0);
    // Both answers are common, so both directions are checked
    CHECK(collisions > CASES/10);
    CHECK(collisions < CASES*9/10);
}

/* An unevenly scaled, turned circle grown until it touches another marker:
 * just short of that, they don't collide, and just past it they do. The
 * ellipse is the unit circle scaled by k*(ax, ay), so in its unit space at
 * k = 1 the other marker is d away from the center, and they touch at k = d. */
static void test_markers_collide_at_threshold(void)
{
    enum { CASES = 300, OUTLINE = 20000 };
    SpratMarker unit = circle_marker(0, 0, 1);
    int checked = 0;

    make_marker_sheet();
    seed_random(1013);

    for (int i = 0; i < CASES; i++)
    {
        Vector2 vertices[8];
        SpratMarker mb = random_marker(vertices);
        double ax = 0.3 + random_float()*3.0, ay = 0.3 + random_float()*3.0;
        float rotation = random_int(2)? 90.0f : random_float()*360.0f;
        int flip = random_int(4);
        double d = 1e30;

        if (mb.type == SPRAT_MARKER_TYPE_POINT) continue;

        SpratSprite a = placed(0, 0, (float)ax, (float)ay, rotation, flip);
        SpratSprite b = random_sprite(random_int(2), 6.0f);

        // The ellipse's center must be clearly outside the other marker
        ref_point center = { 0.0, 0.0 };
        if (depth_in_marker(&mb, to_original(b, center)) > -0.05) continue;

        if (mb.type == SPRAT_MARKER_TYPE_CIRCLE)
        {
            for (int k = 0; k < OUTLINE; k++)
            {
                double t = 2.0*PI_D*k/(double)OUTLINE;
                ref_point p = to_original(a, to_screen(b, mb.position.x + mb.radius*cos(t), mb.position.y + mb.radius*sin(t)));
                d = fmin(d, sqrt(p.x*p.x + p.y*p.y));
            }
        }
        else
        {
            Vector2 corners[4];
            const Vector2 *v = mb.vertices;
            int n = mb.vertexCount;

            if (mb.type == SPRAT_MARKER_TYPE_RECTANGLE)
            {
                corners[0] = mb.position;
                corners[1] = (Vector2){ mb.position.x + mb.size.x, mb.position.y };
                corners[2] = (Vector2){ mb.position.x + mb.size.x, mb.position.y + mb.size.y };
                corners[3] = (Vector2){ mb.position.x, mb.position.y + mb.size.y };
                v = corners;
                n = 4;
            }
            for (int k = 0; k < n; k++)
            {
                ref_point p0 = to_original(a, to_screen(b, v[k].x, v[k].y));
                ref_point p1 = to_original(a, to_screen(b, v[(k + 1)%n].x, v[(k + 1)%n].y));
                ref_point origin = { 0.0, 0.0 };
                d = fmin(d, distance_to_segment(origin, p0.x, p0.y, p1.x, p1.y));
            }
        }

        a.scale = (Vector2){ (float)(ax*d*0.999), (float)(ay*d*0.999) };
        if (CheckCollisionSpratMarkers(a, &unit, b, &mb) || CheckCollisionSpratMarkers(b, &mb, a, &unit))
        {
            fprintf(stderr, "threshold case %d: the ellipse collides with %s 0.1%% short of touching it\n", i, mb.name);
            failures++;
        }
        a.scale = (Vector2){ (float)(ax*d*1.001), (float)(ay*d*1.001) };
        if (!CheckCollisionSpratMarkers(a, &unit, b, &mb) || !CheckCollisionSpratMarkers(b, &mb, a, &unit))
        {
            fprintf(stderr, "threshold case %d: the ellipse doesn't collide with %s 0.1%% past touching it\n", i, mb.name);
            failures++;
        }
        checked++;
    }
    CHECK(checked > CASES/2);
}

#define CHECK_CENTER(marker, sprite, ex, ey) \
    check_center(__LINE__, (marker), (sprite), (ex), (ey))

static void check_center(int line, SpratMarker marker, SpratSprite sprite, float ex, float ey)
{
    Vector2 p = GetSpratMarkerCenter(sprite, &marker);

    if (fabsf(p.x - ex) > 1e-4f || fabsf(p.y - ey) > 1e-4f)
    {
        fprintf(stderr, "%s:%d: %s's center is %g,%g, expected %g,%g\n", __FILE__, line, marker.name, p.x, p.y, ex, ey);
        failures++;
    }
}

static void test_marker_center(void)
{
    static Vector2 triangle[] = { { 0, 0 }, { 6, 0 }, { 0, 3 } };
    static Vector2 square5[] = { { 0, 0 }, { 2, 0 }, { 4, 0 }, { 4, 4 }, { 0, 4 } };   /* an extra vertex on the top edge */
    static Vector2 lShape[] = { { 0, 0 }, { 4, 0 }, { 4, 1 }, { 1, 1 }, { 1, 4 }, { 0, 4 } };
    static Vector2 clockwise[] = { { 0, 0 }, { 0, 4 }, { 4, 4 }, { 4, 0 } };
    static Vector2 segment[] = { { 1, 1 }, { 5, 3 } };
    static Vector2 collinear[] = { { 2, 2 }, { 0, 0 }, { 6, 6 }, { 3, 3 } };
    static Vector2 lone[] = { { 7, -2 } };
    SpratMarker rect = rect_marker(2, 4, 6, 10);

    make_marker_sheet();

    // Each type, unplaced
    CHECK_CENTER(point_marker(3, 4), at(0, 0), 3, 4);
    CHECK_CENTER(circle_marker(3, 4, 9), at(0, 0), 3, 4);
    CHECK_CENTER(rect, at(0, 0), 5, 9);
    CHECK_CENTER(polygon_marker(triangle, 3), at(0, 0), 2, 1);
    CHECK_CENTER(polygon_marker(clockwise, 4), at(0, 0), 2, 2);    // either winding

    // A polygon's centroid is its center of mass: not moved by an extra vertex, and outside an L
    CHECK_CENTER(polygon_marker(square5, 5), at(0, 0), 2, 2);
    CHECK_CENTER(polygon_marker(lShape, 6), at(0, 0), 19.0f/14.0f, 19.0f/14.0f);

    // Without area: a segment's middle, a line's ends' middle, a lone vertex
    CHECK_CENTER(polygon_marker(segment, 2), at(0, 0), 3, 2);
    CHECK_CENTER(polygon_marker(collinear, 4), at(0, 0), 3, 3);
    CHECK_CENTER(polygon_marker(lone, 1), at(0, 0), 7, -2);

    // Placed: moved, by the frame's pivot, flipped and scaled around it, turned clockwise
    CHECK_CENTER(rect, at(100, 50), 105, 59);
    CHECK_CENTER(rect, on_frame(1, at(100, 50)), 102, 61);                 // (5, 9) - pivot (3, -2)
    CHECK_CENTER(rect, placed(100, 50, 1, 1, 0, SPRAT_FLIP_HORIZONTAL), 95, 59);
    CHECK_CENTER(rect, placed(100, 50, 1, 1, 0, SPRAT_FLIP_VERTICAL), 105, 41);
    CHECK_CENTER(rect, placed(100, 50, -1, 1, 0, 0), 95, 59);
    CHECK_CENTER(rect, placed(100, 50, 2, 3, 0, 0), 110, 77);
    CHECK_CENTER(rect, placed(100, 50, 1, 1, 90, 0), 91, 55);
    CHECK_CENTER(rect, placed(100, 50, 1, 1, -90, 0), 109, 45);
    CHECK_CENTER(rect, placed(100, 50, 0, 0, 33, 0), 100, 50);           // scaled to 0: on the pivot
    CHECK_CENTER(polygon_marker(lShape, 6), on_frame(1, placed(10, 20, 2, 2, 180, SPRAT_FLIP_HORIZONTAL)),
                 10 + 2*(19.0f/14.0f - 3), 20 - 2*(19.0f/14.0f + 2));

    // Wherever it is placed, it is where GetSpratSpritePoint() puts the unplaced center, and a
    // circle's center is inside the circle
    seed_random(77);
    for (int i = 0; i < 200; i++)
    {
        Vector2 vertices[8];
        SpratMarker m = random_marker(vertices);
        int frame = random_int(2);
        SpratSprite r = random_sprite(frame, 50.0f);
        Vector2 center = GetSpratMarkerCenter(on_frame(frame, at(0, 0)), &m);

        if (frame == 1)     // at(0, 0) on frame 1 moves it by the pivot
        {
            center.x += 3.0f;
            center.y -= 2.0f;
        }
        Vector2 expected = GetSpratSpritePoint(r, center);
        Vector2 p = GetSpratMarkerCenter(r, &m);
        if (fabsf(p.x - expected.x) > 1e-3f || fabsf(p.y - expected.y) > 1e-3f)
        {
            fprintf(stderr, "center case %d: %s placed at %g,%g is %g,%g, GetSpratSpritePoint() says %g,%g\n",
                    i, m.name, r.position.x, r.position.y, p.x, p.y, expected.x, expected.y);
            failures++;
        }
        if (m.type == SPRAT_MARKER_TYPE_CIRCLE) CHECK(CheckCollisionPointSpratMarker(p, r, &m));
    }

    // Without a marker, or on a frame out of range: the sprite's position
    SpratSprite sprite = at(12, 34);
    CHECK_FLOAT(GetSpratMarkerCenter(sprite, NULL).x, 12);
    CHECK_FLOAT(GetSpratMarkerCenter(on_frame(2, sprite), &rect).y, 34);
    SpratMarker bad = polygon_marker(triangle, 0);
    CHECK_FLOAT(GetSpratMarkerCenter(sprite, &bad).x, 12);
    bad = point_marker(0, 0);
    bad.type = 9;
    CHECK_FLOAT(GetSpratMarkerCenter(sprite, &bad).y, 34);
}

static void test_marker_outline(void)
{
    static Vector2 pentagon[] = { { 6, 0 }, { 10, 6 }, { 10, 40 }, { 3, 40 }, { 3, 6 } };
    static Vector2 segment[] = { { 1, 1 }, { 5, 3 } };
    static Vector2 lone[] = { { 7, -2 } };
    Vector2 points[300];
    SpratMarker rect = rect_marker(2, 4, 6, 10);
    SpratMarker polygon = polygon_marker(pentagon, COUNT(pentagon));
    SpratMarker circle = circle_marker(3, 4, 5);
    SpratMarker origin = { 0 };
    int count;

    make_marker_sheet();

    // A point is its point; a rectangle, its corners from the top-left and back
    SpratSprite sprite = on_frame(1, placed(100, 50, 2, 3, 90, SPRAT_FLIP_HORIZONTAL));
    CHECK_INT(GetSpratMarkerOutline(sprite, &origin, points, COUNT(points)), 1);
    count = GetSpratMarkerOutline(sprite, &rect, points, COUNT(points));
    CHECK_INT(count, 5);
    for (int i = 0; i < 5; i++)
    {
        Vector2 corner = { rect.position.x + ((i == 1 || i == 2)? rect.size.x : 0.0f), rect.position.y + ((i == 2 || i == 3)? rect.size.y : 0.0f) };
        Vector2 expected = GetSpratSpritePoint(sprite, corner);
        CHECK_FLOAT(points[i].x, expected.x);
        CHECK_FLOAT(points[i].y, expected.y);
    }

    // A polygon: its vertices and back to the first; a segment back and forth; a lone vertex
    sprite.frame = 0;
    count = GetSpratMarkerOutline(sprite, &polygon, points, COUNT(points));
    CHECK_INT(count, 6);
    for (int i = 0; i < 6; i++)
    {
        Vector2 expected = GetSpratSpritePoint(sprite, pentagon[i%5]);
        CHECK_FLOAT(points[i].x, expected.x);
        CHECK_FLOAT(points[i].y, expected.y);
    }
    CHECK_INT(GetSpratMarkerOutline(sprite, &(SpratMarker){ "s", SPRAT_MARKER_TYPE_POLYGON, { 1, 1 }, 0, { 0, 0 }, segment, 2 }, points, COUNT(points)), 3);
    CHECK_INT(GetSpratMarkerOutline(sprite, &(SpratMarker){ "l", SPRAT_MARKER_TYPE_POLYGON, { 7, -2 }, 0, { 0, 0 }, lone, 1 }, points, COUNT(points)), 1);

    // A circle: points on the ellipse it is placed as, closed, more of them as it grows, and
    // each side within a quarter of a pixel of the curve
    static const Vector2 scales[] = { { 0.01f, 0.01f }, { 1, 1 }, { 4, 1 }, { 1, 4 }, { 10, 10 }, { 1000, 1000 } };
    int previous = 0;
    for (size_t k = 0; k < COUNT(scales); k++)
    {
        sprite = on_frame(1, placed(40, 30, scales[k].x, scales[k].y, 33, SPRAT_FLIP_VERTICAL));
        count = GetSpratMarkerOutline(sprite, &circle, points, COUNT(points));
        CHECK(count >= 9 && count <= 257);
        CHECK(count >= previous || k == 3);    // (1, 4) is (4, 1) turned the other way
        previous = count;
        CHECK_FLOAT(points[count - 1].x, points[0].x);
        CHECK_FLOAT(points[count - 1].y, points[0].y);

        for (int i = 0; i < count; i++)
        {
            ref_point p = { points[i].x, points[i].y };
            ref_point o = to_original(sprite, p);
            double distance = sqrt((o.x - 3)*(o.x - 3) + (o.y - 4)*(o.y - 4));

            if (fabs(distance - 5.0) > 5e-3*5.0)
            {
                fprintf(stderr, "outline point %d of a circle scaled %gx%g is %g from its center\n", i, sprite.scale.x, sprite.scale.y, distance);
                failures++;
                break;
            }
        }
        if (scales[k].x == scales[k].y && count < 257)
        {
            // Evenly scaled: a side's middle is inside the circle by at most the tolerance
            double r = 5.0*scales[k].x;
            ref_point c = to_screen(sprite, 3, 4);

            for (int i = 0; i + 1 < count; i++)
            {
                double mx = (points[i].x + points[i + 1].x)/2.0 - c.x, my = (points[i].y + points[i + 1].y)/2.0 - c.y;
                CHECK(r - sqrt(mx*mx + my*my) <= 0.25 + 1e-3);
            }
        }
    }
    CHECK_INT(previous, 257);   // the largest, of a radius of 5000 pixels, is capped

    // Like snprintf(): the count, and only what fits written
    sprite = at(0, 0);
    CHECK_INT(GetSpratMarkerOutline(sprite, &rect, NULL, 0), 5);
    CHECK_INT(GetSpratMarkerOutline(sprite, &rect, NULL, 5), 5);
    for (int i = 0; i < 5; i++) points[i] = (Vector2){ -1, -1 };
    CHECK_INT(GetSpratMarkerOutline(sprite, &rect, points, 2), 5);
    CHECK_FLOAT(points[1].x, 8);
    CHECK_FLOAT(points[2].x, -1);
    CHECK_INT(GetSpratMarkerOutline(sprite, &rect, points, -1), 5);
    CHECK_FLOAT(points[2].x, -1);
    CHECK_INT(GetSpratMarkerOutline(sprite, &circle, NULL, 0), GetSpratMarkerOutline(sprite, &circle, points, COUNT(points)));

    // Nothing for a missing or bad marker, or on a frame out of range
    CHECK_INT(GetSpratMarkerOutline(sprite, NULL, points, 5), 0);
    CHECK_INT(GetSpratMarkerOutline(on_frame(2, sprite), &rect, points, 5), 0);
    SpratMarker bad = polygon_marker(pentagon, 0);
    CHECK_INT(GetSpratMarkerOutline(sprite, &bad, points, 5), 0);
    bad = point_marker(0, 0);
    bad.type = 9;
    CHECK_INT(GetSpratMarkerOutline(sprite, &bad, points, 5), 0);
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
    test_convert_loops();
    test_add_size_overflow();
    test_join_path();
    test_lookups();
    test_animation_player();
    test_animation_pingpong();
    test_set_animation();
    test_animation_sprite();
    test_draw_matches_original();
    test_sprite_point_matches_draw();
    test_markers_collide();
    test_markers_collide_match_reference();
    test_markers_collide_at_threshold();
    test_marker_center();
    test_marker_outline();
    test_npatch_matches_original();

    if (failures != 0) fprintf(stderr, "%d failures\n", failures);
    else printf("all tests passed\n");
    return (failures == 0)? 0 : 1;
}
