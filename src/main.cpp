#include "Engine.h"
Engine engine;
int main()
{
    InitWindow(1280, 720, "2D Physics Playground");
    SetTargetFPS(0);
    while (!WindowShouldClose())
    {
        engine.HandleInput(); // now polled every frame (buttons, presets, free-draw), not just on click
        engine.Update();
        BeginDrawing();
        ClearBackground(RAYWHITE);
        engine.camerahandle();
        engine.bodydraw();
        EndDrawing();
    }
    CloseWindow();

    return 0;
}
