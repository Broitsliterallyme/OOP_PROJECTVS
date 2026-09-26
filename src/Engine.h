#ifndef ENGINE_H
#define ENGINE_H

#include "World.h"
#include "raylib.h"
#include <vector>

// What the next click on the canvas will spawn.
enum class SpawnShape
{
    Circle,
    Box,
    Triangle,
    Pentagon,
    Hexagon,
    Custom // free-draw: click to place vertices, right-click/Enter to close the shape
};

class Engine
{
private:
    struct ShapeButton
    {
        Rectangle rect;
        SpawnShape shape;
        const char *label;
    };

    World world;
    Camera2D camera;

    std::vector<ShapeButton> buttons;
    SpawnShape currentShape = SpawnShape::Box;

    std::vector<Vector2> drawPoints; // world-space points placed so far in Custom mode
    bool isDrawing = false;

    void InitButtons();
    bool HandleButtonClick(Vector2 screenPos); // true if the click landed on a button
    bool IsOverUI(Vector2 screenPos) const;
    void SpawnPreset(Vector2 worldPos);
    void FinishCustomShape();
    void DrawUI();
    void DrawCustomPreview();

public:
    Engine();
    void HandleInput(); // call every frame; replaces the old click-gated dropbody()
    void bodydraw();
    void camerahandle();
    void Update();
};

#endif
