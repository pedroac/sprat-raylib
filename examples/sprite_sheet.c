/*******************************************************************************************
*
*   sprat-raylib example - Animated sprites, markers and nine-slices
*
*   Controls:
*       LEFT/RIGHT  Run
*       SPACE       Swing the sword
*       P           Pause and resume the spins
*       D           Show pivots and hitboxes
*
*   Resources: resources/sheet.sprat and resources/atlas.png, packed by sprat-cli
*   (see resources/make_resources.sh). Most frames are stored rotated in the atlas.
*
*   Copyright (c) 2026 Pedro Amaral Couto
*
********************************************************************************************/

#include "raylib.h"

#include "sprat-raylib.h"       // Linked as a library; elsewhere, define SPRAT_RAYLIB_IMPLEMENTATION in one file

#include <math.h>               // Required for: sinf(), fminf(), fabsf()
#include <stddef.h>             // Required for: NULL

#define HERO_SCALE      3.0f
#define HERO_SPEED      240.0f
#define SWORD_SCALE     2.0f
#define SWING_TIME      0.3f    // Seconds a sword swing lasts

// Draw a frame's hitbox marker, placed as DrawSpratFramePro() places the frame
// NOTE: Ordering two corners only works without rotation; with it, map all four corners
static void DrawHitbox(SpratSheet sheet, int frame, Vector2 position, float scale, int flip, Color color)
{
    const SpratMarker *hitbox = GetSpratFrameMarker(sheet, frame, "hitbox");
    if (hitbox == NULL) return;

    Vector2 s = { scale, scale };

    if (hitbox->type == SPRAT_MARKER_TYPE_RECTANGLE)
    {
        Vector2 a = GetSpratFramePoint(sheet, frame, hitbox->position, position, s, 0.0f, flip);
        Vector2 b = GetSpratFramePoint(sheet, frame, (Vector2){ hitbox->position.x + hitbox->size.x, hitbox->position.y + hitbox->size.y }, position, s, 0.0f, flip);
        Rectangle rec = { fminf(a.x, b.x), fminf(a.y, b.y), fabsf(b.x - a.x), fabsf(b.y - a.y) };
        DrawRectangleLinesEx(rec, 2.0f, color);
    }
    else if (hitbox->type == SPRAT_MARKER_TYPE_CIRCLE)
    {
        Vector2 center = GetSpratFramePoint(sheet, frame, hitbox->position, position, s, 0.0f, flip);
        DrawCircleLinesV(center, hitbox->radius*scale, color);
    }
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "sprat-raylib - sprite sheet");

    ChangeDirectory(GetApplicationDirectory());     // Find resources/ next to the executable

    SpratSheet sheet = LoadSpratSheet("resources/sheet.sprat");

    // Look names up once, then draw by index, and play animations with a player each
    SpratAnimationPlayer run = MakeSpratAnimationPlayer(GetSpratAnimation(sheet, "run"));
    SpratAnimationPlayer runLeft = MakeSpratAnimationPlayer(GetSpratAnimation(sheet, "run-left"));
    SpratAnimationPlayer heroRun = run;
    const float spinSpeeds[3] = { 1.0f, 0.5f, -1.0f };     // normal, half speed, reversed
    SpratAnimationPlayer spins[3] = { 0 };
    for (int i = 0; i < 3; i++)
    {
        spins[i] = MakeSpratAnimationPlayer(GetSpratAnimation(sheet, "spin"));
        SetSpratAnimationSpeed(&spins[i], spinSpeeds[i]);
    }
    int idle = GetSpratFrameIndex(sheet, "hero/run_01.png");
    int sword = GetSpratFrameIndex(sheet, "sword");
    int panel = GetSpratFrameIndex(sheet, "panel");

    Vector2 hero = { screenWidth/2.0f, 380.0f };
    Vector2 heroScale = { HERO_SCALE, HERO_SCALE };
    Vector2 swordScale = { SWORD_SCALE, SWORD_SCALE };
    int facing = 0;                 // 0 or SPRAT_FLIP_HORIZONTAL
    float swingTime = 0.0f;
    bool debug = false;

    SetTargetFPS(60);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        float dt = GetFrameTime();
        bool running = false;

        if (IsKeyDown(KEY_RIGHT)) { hero.x += HERO_SPEED*dt; facing = 0; running = true; }
        if (IsKeyDown(KEY_LEFT)) { hero.x -= HERO_SPEED*dt; facing = SPRAT_FLIP_HORIZONTAL; running = true; }
        if (hero.x < 0.0f) hero.x = 0.0f;
        if (hero.x > screenWidth) hero.x = (float)screenWidth;

        if (running) UpdateSpratAnimation(&heroRun, dt);
        else StopSpratAnimation(&heroRun);

        UpdateSpratAnimation(&run, dt);
        UpdateSpratAnimation(&runLeft, dt);
        for (int i = 0; i < 3; i++) UpdateSpratAnimation(&spins[i], dt);

        if (IsKeyPressed(KEY_P))
        {
            for (int i = 0; i < 3; i++)
            {
                if (IsSpratAnimationPlaying(spins[i])) PauseSpratAnimation(&spins[i]);
                else ResumeSpratAnimation(&spins[i]);
            }
        }

        if (IsKeyPressed(KEY_SPACE)) swingTime = SWING_TIME;
        if (swingTime > 0.0f) swingTime -= dt;

        if (IsKeyPressed(KEY_D)) debug = !debug;

        // The run animation's first frame doubles as the idle pose
        int heroFrame = running? GetSpratAnimationFrame(heroRun) : idle;

        // The sword is held by the hero's "hand" marker, mirrored with the hero
        Vector2 hand = hero;
        const SpratMarker *handMarker = GetSpratFrameMarker(sheet, heroFrame, "hand");
        if (handMarker != NULL) hand = GetSpratFramePoint(sheet, heroFrame, handMarker->position, hero, heroScale, 0.0f, facing);

        float swordAngle = 60.0f - ((swingTime > 0.0f)? 150.0f*(1.0f - swingTime/SWING_TIME) : 0.0f);
        if (facing) swordAngle = -swordAngle;

        // The panel breathes, to show its corners stay sharp
        float t = (float)GetTime();
        Rectangle panelRec = { 20, 20, 300 + 60*sinf(t*1.5f), 110 + 20*sinf(t*2.0f) };
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(RAYWHITE);

            DrawSpratFrameNPatch(sheet, panel, panelRec, WHITE);
            DrawText("DrawSpratFrameNPatch()", (int)panelRec.x + 20, (int)panelRec.y + 24, 20, RAYWHITE);
            DrawText("LEFT/RIGHT run, SPACE swing\nD pivots and hitboxes, P pause", (int)panelRec.x + 20, (int)panelRec.y + 56, 10, LIGHTGRAY);

            // Looping animations, with the flip the sheet gave each one
            DrawText("run", 470, 40, 10, GRAY);
            DrawText("run-left (alias, flip h)", 580, 40, 10, GRAY);
            DrawText("spin", 735, 40, 10, GRAY);
            DrawSpratAnimation(sheet, run, (Vector2){ 490, 150 }, WHITE);
            DrawSpratAnimation(sheet, runLeft, (Vector2){ 630, 150 }, WHITE);
            for (int i = 0; i < 3; i++)
            {
                DrawSpratAnimation(sheet, spins[i], (Vector2){ 750, 90.0f + i*40 }, WHITE);
                DrawText(TextFormat("x%g", spinSpeeds[i]), 770, 85 + i*40, 10, GRAY);
            }

            // Every frame, rotated around its pivot
            for (int i = 0; i < sheet.frameCount; i++)
            {
                Vector2 position = { 40.0f + i*46.0f, 230.0f };
                DrawSpratFrameEx(sheet, i, position, 15.0f*sinf(t + i), 0.75f, WHITE);
                if (debug) DrawCircleV(position, 2.0f, RED);
            }

            DrawLine(0, (int)hero.y, screenWidth, (int)hero.y, LIGHTGRAY);

            // Controlled hero, with the sword drawn behind when facing left
            if (facing) DrawSpratFramePro(sheet, sword, hand, swordScale, swordAngle, 0, WHITE);
            DrawSpratFramePro(sheet, heroFrame, hero, heroScale, 0.0f, facing, WHITE);
            if (!facing) DrawSpratFramePro(sheet, sword, hand, swordScale, swordAngle, 0, WHITE);

            if (debug)
            {
                DrawHitbox(sheet, heroFrame, hero, HERO_SCALE, facing, RED);
                DrawCircleV(hero, 4.0f, RED);
                DrawCircleV(hand, 4.0f, BLUE);
            }

            if (!IsSpratSheetValid(sheet)) DrawText("Failed to load resources/sheet.sprat", 20, 420, 20, MAROON);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadSpratSheet(sheet);        // Unload atlas textures and frames

    CloseWindow();                  // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
