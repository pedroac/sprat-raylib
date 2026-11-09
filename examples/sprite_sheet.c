/*******************************************************************************************
*
*   sprat-raylib example - Animated sprites, markers, collisions and nine-slices
*
*   Controls:
*       LEFT/RIGHT  Run, and pick up the coins you run into
*       SPACE       Swing the sword, which picks up the coins it cuts
*       P           Pause and resume the spins
*       D           Show pivots, hitboxes and the blade
*
*   Resources: resources/sheet.sprat and resources/atlas.png, packed by sprat-cli
*   (see resources/make_resources.sh). Most frames are stored rotated in the atlas.
*
*   Copyright (c) 2026 Pedro Amaral Couto
*
********************************************************************************************/

#include "raylib.h"

#include "sprat-raylib.h"       // Linked as a library; elsewhere, define SPRAT_RAYLIB_IMPLEMENTATION in one file

#include <math.h>               // Required for: sinf()

#define HERO_SCALE      3.0f
#define HERO_SPEED      240.0f
#define SWORD_SCALE     2.0f
#define SWING_TIME      0.3f    // Seconds a sword swing lasts
#define COIN_COUNT      4
#define COIN_SCALE      2.0f
#define COIN_RESPAWN    3.0f    // Seconds a picked coin stays away

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
    const float spinSpeeds[3] = { 1.0f, 0.5f, -1.0f };     // normal, half speed, reversed
    SpratAnimationPlayer spins[3] = { 0 };
    for (int i = 0; i < 3; i++)
    {
        spins[i] = MakeSpratAnimationPlayer(GetSpratAnimation(sheet, "spin"));
        SetSpratAnimationSpeed(&spins[i], spinSpeeds[i]);
    }
    SpratAnimationPlayer coinSpin = MakeSpratAnimationPlayer(GetSpratAnimation(sheet, "spin"));
    SpratAnimationPlayer coinPingPong = MakeSpratAnimationPlayer(GetSpratAnimation(sheet, "spin-pingpong"));
    int panel = GetSpratFrameIndex(sheet, "panel");

    // Sprites are where frames are: drawn there, and their markers placed there
    const float groundY = 380.0f;
    // The hero keeps a sprite and a player together, switching between its idle and run animations
    SpratAnimation heroRun = run.animation;
    SpratAnimation heroIdle = GetSpratAnimation(sheet, "idle");
    SpratSprite hero = MakeSpratSprite(sheet, -1, (Vector2){ screenWidth/2.0f, groundY });
    SpratAnimationPlayer heroAnimation = MakeSpratAnimationPlayer(heroIdle);
    bool facingLeft = false;
    SpratSprite sword = MakeSpratSprite(sheet, GetSpratFrameIndex(sheet, "sword"), hero.position);
    sword.scale = (Vector2){ SWORD_SCALE, SWORD_SCALE };

    // Coins along the hero's path, at the height of the hero's chest and of the sword's swing
    const float coinX[COIN_COUNT] = { 110.0f, 260.0f, 540.0f, 690.0f };
    SpratSprite coins[COIN_COUNT] = { 0 };
    float coinAway[COIN_COUNT] = { 0 };     // Seconds until a picked coin comes back
    int coinsPicked = 0;
    for (int i = 0; i < COIN_COUNT; i++)
    {
        coins[i] = MakeSpratSprite(sheet, -1, (Vector2){ coinX[i], 290.0f });
        coins[i].scale = (Vector2){ COIN_SCALE, COIN_SCALE };
    }

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

        if (IsKeyDown(KEY_RIGHT)) { hero.position.x += HERO_SPEED*dt; facingLeft = false; running = true; }
        if (IsKeyDown(KEY_LEFT)) { hero.position.x -= HERO_SPEED*dt; facingLeft = true; running = true; }
        if (hero.position.x < 0.0f) hero.position.x = 0.0f;
        if (hero.position.x > screenWidth) hero.position.x = (float)screenWidth;

        UpdateSpratAnimation(&run, dt);
        UpdateSpratAnimation(&runLeft, dt);
        for (int i = 0; i < 3; i++) UpdateSpratAnimation(&spins[i], dt);
        UpdateSpratAnimation(&coinSpin, dt);
        UpdateSpratAnimation(&coinPingPong, dt);

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

        // Setting the animation every update restarts it only when it changes. The player sets
        // the hero's frame and flip; facing left mirrors with the scale, which it leaves alone
        SetSpratAnimation(&heroAnimation, running? heroRun : heroIdle);
        UpdateSpratSpriteAnimation(&hero, &heroAnimation, dt);
        hero.scale = (Vector2){ facingLeft? -HERO_SCALE : HERO_SCALE, HERO_SCALE };

        // The sword is held by the hero's "hand" marker, mirrored with the hero
        sword.position = GetSpratMarkerCenter(hero, GetSpratSpriteMarker(hero, "hand"));
        sword.rotation = 60.0f - ((swingTime > 0.0f)? 150.0f*(1.0f - swingTime/SWING_TIME) : 0.0f);
        if (facingLeft) sword.rotation = -sword.rotation;

        // A coin is picked when the hero's hitbox collides with the coin's, or the sword's blade
        // does while it swings
        const SpratMarker *heroHitbox = GetSpratSpriteMarker(hero, "hitbox");
        const SpratMarker *blade = GetSpratSpriteMarker(sword, "blade");
        int coinFrame = GetSpratAnimationFrame(coinSpin);
        const SpratMarker *coinHitbox = GetSpratFrameMarker(sheet, coinFrame, "hitbox");    // The coins spin together
        for (int i = 0; i < COIN_COUNT; i++)
        {
            SetSpratSpriteAnimationFrame(&coins[i], coinSpin);
            if (coinAway[i] > 0.0f) { coinAway[i] -= dt; continue; }

            if (CheckCollisionSpratMarkers(hero, heroHitbox, coins[i], coinHitbox) ||
                ((swingTime > 0.0f) && CheckCollisionSpratMarkers(sword, blade, coins[i], coinHitbox)))
            {
                coinAway[i] = COIN_RESPAWN;
                coinsPicked++;
            }
        }

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

            // Animations, with the flip and loop mode the sheet gave each one
            DrawText("run", 470, 40, 10, GRAY);
            DrawText("run-left (alias, flip h)", 580, 40, 10, GRAY);
            DrawText("spin", 735, 40, 10, GRAY);
            DrawText("pingpong", 405, 40, 10, GRAY);
            DrawSpratAnimation(sheet, coinPingPong, (Vector2){ 425, 90 }, WHITE);
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

            DrawLine(0, (int)groundY, screenWidth, (int)groundY, LIGHTGRAY);

            // Coins still to pick, and the count of those picked
            for (int i = 0; i < COIN_COUNT; i++)
            {
                if (coinAway[i] > 0.0f) continue;
                DrawSpratSprite(coins[i], WHITE);
                if (debug) DrawSpratMarker(coins[i], coinHitbox, RED);
            }
            DrawText(TextFormat("coins picked: %i", coinsPicked), 20, 390, 20, GRAY);

            // Controlled hero, with the sword drawn behind when facing left
            if (facingLeft) DrawSpratSprite(sword, WHITE);
            DrawSpratSprite(hero, WHITE);
            if (!facingLeft) DrawSpratSprite(sword, WHITE);

            if (debug)
            {
                DrawSpratMarker(hero, heroHitbox, RED);
                DrawSpratMarker(sword, blade, RED);
                DrawCircleV(hero.position, 4.0f, RED);
                DrawCircleV(sword.position, 4.0f, BLUE);
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
