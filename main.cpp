#include "CS3113/cs3113.h"
#include "CS3113/Entity.h"
#include <math.h>
#include <string>
/**
* Author: Alaya Chowdhury
* Assignment: Lunar Lander
* Date due: 2025-10-27, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on Academic Misconduct.
**/

constexpr int SCREEN_WIDTH         = 1200;
constexpr int SCREEN_HEIGHT        = 675;
constexpr int FPS                  = 60;
AppStatus gAppStatus       = RUNNING;
float     gPreviousTicks   = 0.0f;
float     gTimeAccumulator = 0.0f;

// Physics
constexpr float ACCELERATION_OF_GRAVITY = 100.0f;
// acceleration when keyboard input detected (against gravity if up arrow, sideways acceleration if left/right)
constexpr float KB_ACCELERATION     = 250.0f;
constexpr float FIXED_TIMESTEP     = 1.0f / 60.0f;

// Lander
Entity* lander        = nullptr;
Vector2 landerPosition     = {0.0f, 0.0f};
Vector2 landerVelocity     = {0.0f, 0.0f};
Vector2 landerAcceleration = {0.0f, 0.0f};
bool  gLanderWon     = false;

//winning platform
Entity* safePlatform  = nullptr;
constexpr float PLATFORM_SPEED      = 160.0f;
float gPlatformDirection = 1.0f;

//losing platform
Entity* ground        = nullptr;

// Fuel
constexpr float FUEL_CONSUMPTION    = 10.0f; // decrement from gFuel per key press
float gFuel          = 100.0f;

//Game state
bool  gGameOver      = false;

// Function declarations
void initialise();
void processInput();
void update();
void render();
void shutdown();

//since isColliding is private in Entity.h but we need it to change gamestate we do it here
bool isColliding(const Vector2 *positionA, const Vector2 *scaleA,
                 const Vector2 *positionB, const Vector2 *scaleB)
{
    float xDistance = fabs(positionA->x - positionB->x) -
                      ((scaleA->x + scaleB->x) / 2.0f);
    float yDistance = fabs(positionA->y - positionB->y) -
                      ((scaleA->y + scaleB->y) / 2.0f);

    return (xDistance < 0.0f && yDistance < 0.0f);
}

//keep entities within screen
void stayInScreen(Vector2 &pos, Vector2 &vel, const Vector2 &scale) {
    if (pos.x - scale.x / 2.0f < 0.0f)  { 
        pos.x = scale.x / 2.0f; 
        vel.x = 0.0f; }
    if (pos.x + scale.x / 2.0f > SCREEN_WIDTH) { 
        pos.x = SCREEN_WIDTH - scale.x / 2.0f; 
        vel.x = 0.0f; }
    if (pos.y - scale.y / 2.0f < 0.0f)  { 
        pos.y = scale.y / 2.0f; 
        vel.y = 0.0f; }
    if (pos.y + scale.y / 2.0f > SCREEN_HEIGHT) { 
        pos.y = SCREEN_HEIGHT - scale.y / 2.0f; vel.y = 0.0f; }
}

void initialise() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Lunar Lander");
    SetTargetFPS(FPS);

    lander = new Entity(
        { SCREEN_WIDTH / 2.0f, 200.0f }, // position
        { 100.0f, 100.0f }, // scale
        "assets/voltorb.png", // texture file address
        PLAYER // EntityType
    );

    ground = new Entity(
        { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT - 40.0f }, //position
        { (float)SCREEN_WIDTH, 80.0f }, // scale,
        "assets/ground.png", // texture file address
        BLOCK // EntityType
    );

    safePlatform = new Entity(
        { 0, 0 }, // position
        { 150.0f, 50.0f }, // scale
        "assets/win.png", //texture file address
        PLATFORM // EntityType
    );

    // placing unsafe platform (ground) and safe platform in relative position 
    Vector2 groundScale = ground->getScale();
    Vector2 platformScale = safePlatform->getScale();
    float groundTopY = (SCREEN_HEIGHT - 40.0f) - (groundScale.y / 2.0f); // y of ground top edge
    float platformY = groundTopY - (platformScale.y / 2.0f) - 10.0f;

    Vector2 platformPos = safePlatform->getPosition();
    platformPos.y = platformY;
    // the platform starts in the middle of the screen but near ground initially
    platformPos.x = SCREEN_WIDTH / 2.0f;
    safePlatform->setPosition(platformPos);

    gPreviousTicks = GetTime();
}

void processInput() {
    // close when press x on window screen
    if (WindowShouldClose()) {
        gGameOver = true;
        gAppStatus = TERMINATED;
        return;
    }

    // after win/loss conditions are met no more lander input
    if (gGameOver) return;

    // lander acceleration is typically gravity
    landerAcceleration.y = ACCELERATION_OF_GRAVITY;

    // Read keyboard input
    bool leftDown  = IsKeyDown(KEY_LEFT);
    bool rightDown = IsKeyDown(KEY_RIGHT);
    bool upDown    = IsKeyDown(KEY_UP);

    // lander input + update presses
    int presses = 0;
    if (IsKeyPressed(KEY_LEFT))  {presses++;}
    if (IsKeyPressed(KEY_RIGHT)) {presses++;}
    if (IsKeyPressed(KEY_UP))    {presses++;}

    // after pressing up/left/right in total ten times fuel is completely depleted
    if (presses > 0 && gFuel > 0.0f) {
        gFuel -= FUEL_CONSUMPTION * static_cast<float>(presses);
        if (gFuel < 0.0f) gFuel = 0.0f;
    }

    // when fuel is finished only acceleration that applies to lander is gravity
    if (gFuel <= 0.0f) {
        leftDown = rightDown = false;
        upDown = false;
    }

    // only modify the values in acceleration to change movement
    //pressing left and right together should "cancel" out each other
    if (leftDown && !rightDown){
        landerAcceleration.x = -KB_ACCELERATION;}
    else if (rightDown && !leftDown){
        landerAcceleration.x = KB_ACCELERATION;}
    else{
        landerAcceleration.x = 0.0f;}

    if (upDown && gFuel > 0.0f) landerAcceleration.y = -KB_ACCELERATION;
}


void update() {
    // Delta time
    float ticks = (float) GetTime();
    float deltaTime = ticks - gPreviousTicks;
    gPreviousTicks = ticks;
    deltaTime += gTimeAccumulator;

    if (deltaTime < FIXED_TIMESTEP)
    {
        gTimeAccumulator = deltaTime;
        return;
    }

    while (deltaTime >= FIXED_TIMESTEP) {
        // check for win or loss by checking if collision from lander to sagfe/unsafe platform
        if (!gGameOver) {
            Vector2 landerScale = lander->getScale();
            Vector2 safePos = safePlatform->getPosition();
            Vector2 safeScale = safePlatform->getScale();
            Vector2 groundPos = ground->getPosition();
            Vector2 groundScale = ground->getScale();
            if (isColliding(&landerPosition, &landerScale, &safePos, &safeScale)) {
                // mission accomplished
                gLanderWon = true;
                gGameOver  = true;
                lander->setAcceleration({ 0.0f, 0.0f });
            } else if (isColliding(&landerPosition, &landerScale, &groundPos, &groundScale)) {
                // mission failed
                gLanderWon = false;
                gGameOver  = true;
                lander->setAcceleration({ 0.0f, 0.0f });
            }
        }
        // move safe platform horizontally from edge to edge
        Vector2 pPos = safePlatform->getPosition();
        Vector2 pScale = safePlatform->getScale();

        pPos.x += gPlatformDirection * PLATFORM_SPEED *  FIXED_TIMESTEP;
        if (pPos.x -  pScale.x / 2.0f <= 0.0f) {
            pPos.x =  pScale.x / 2.0f;
            gPlatformDirection = 1.0f;
        } else if (pPos.x +  pScale.x / 2.0f >= SCREEN_WIDTH) {
            pPos.x = SCREEN_WIDTH -  pScale.x / 2.0f;
            gPlatformDirection = -1.0f;
            }
        pPos.y = safePlatform->getPosition().y;
            safePlatform->setPosition(pPos);
        

        // after we win or lose game stop the lander from moving
        if (gGameOver) {
            lander->setAcceleration({ 0.0f, 0.0f });
            landerPosition = lander->getPosition();
            deltaTime -=  FIXED_TIMESTEP;
            continue;
        }

        // update lander acceleration using entity method
        lander->setAcceleration(landerAcceleration);

        //update lander velocity and position
        landerVelocity.x += landerAcceleration.x * FIXED_TIMESTEP;
        landerVelocity.y += landerAcceleration.y * FIXED_TIMESTEP;
        landerPosition.x += landerVelocity.x * FIXED_TIMESTEP;
        landerPosition.y += landerVelocity.y * FIXED_TIMESTEP;

        // make sure lander stays in screen
        stayInScreen(landerPosition, landerVelocity, lander->getScale());

        // set entity position
        lander->setPosition(landerPosition);
        deltaTime -= FIXED_TIMESTEP;
    }
}

void render() {
    BeginDrawing();
    ClearBackground(ColorFromHex("#FFFFFF"));

    ground->render();
    safePlatform->render();
    lander->render();

    // fuel ui
    DrawText(TextFormat("Fuel: %.0f", gFuel), 40, 40, 40, ColorFromHex("#000000"));

    if (gGameOver) {
        // we know if there is a win/loss with gGameOver and will display message accordingly
        const char* end_message = gLanderWon ? "Mission Accomplished" : "Mission Failed";
        int size  = 60;
        int width = MeasureText(end_message, size);
        DrawText(end_message,
                 SCREEN_WIDTH / 2 - width / 2,
                 SCREEN_HEIGHT / 2 - 30,
                 size,
                 gLanderWon ? ColorFromHex("#00FF00")
                            : ColorFromHex("#FF0000"));
    }

    EndDrawing();
}

void shutdown() {
    CloseWindow();
}

int main(void) {
    initialise();

    while (gAppStatus == RUNNING) {
        processInput();
        update();
        render();
    }

    shutdown();
    return 0;
}