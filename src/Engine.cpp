#include "Engine.h"
#include "Geometry.h"
#include <string>

Engine::Engine()
{
    camera.target = {0.0f, 0.0f};
    camera.offset = {0.0f, 0.0f};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    Body2D body;
    body.CreateRectangle({-400.0f, -100.0f}, 1.0f, 30.0f, 400.0f, 1.0f, true, body);
    body.Rotate(0.2f);
    world.AddBody(body);
    body.CreateRectangle({400.0f, -100.0f}, 1.0f, 30.0f, 400.0f, 1.0f, true, body);
    body.Rotate(-0.2f);
    world.AddBody(body);
    body.CreateRectangle({0.0f, 200.0f}, 1.0f, 30.0f, 1000.0f, 1.0f, true, body);
    world.AddBody(body);

    InitButtons();
}

void Engine::InitButtons()
{
    struct Entry
    {
        SpawnShape shape;
        const char *label;
    };
    const Entry entries[] = {
        {SpawnShape::Circle, "Circle"},
        {SpawnShape::Box, "Box"},
        {SpawnShape::Triangle, "Tri"},
        {SpawnShape::Pentagon, "Penta"},
        {SpawnShape::Hexagon, "Hexa"},
        {SpawnShape::Custom, "Draw"},
    };

    float x = 10.0f;
    const float y = 10.0f;
    const float w = 70.0f;
    const float h = 32.0f;
    const float gap = 8.0f;

    for (const Entry &entry : entries)
    {
        buttons.push_back({Rectangle{x, y, w, h}, entry.shape, entry.label});
        x += w + gap;
    }
}

bool Engine::IsOverUI(Vector2 screenPos) const
{
    for (const ShapeButton &button : buttons)
    {
        if (CheckCollisionPointRec(screenPos, button.rect))
            return true;
    }
    return false;
}

bool Engine::HandleButtonClick(Vector2 screenPos)
{
    for (const ShapeButton &button : buttons)
    {
        if (CheckCollisionPointRec(screenPos, button.rect))
        {
            if (currentShape == SpawnShape::Custom && button.shape != SpawnShape::Custom && isDrawing)
            {
                // Switching tools mid-draw abandons whatever was being traced.
                drawPoints.clear();
                isDrawing = false;
            }
            currentShape = button.shape;
            return true;
        }
    }
    return false;
}

void Engine::SpawnPreset(Vector2 worldPos)
{
    Body2D body;
    switch (currentShape)
    {
    case SpawnShape::Circle:
        body.CreateCircle(worldPos, 1.0f, 30.0f, 0.4f, false, body);
        world.AddBody(body);
        break;
    case SpawnShape::Box:
        body.CreateRectangle(worldPos, 1.0f, 60.0f, 60.0f, 0.6f, false, body);
        world.AddBody(body);
        break;
    case SpawnShape::Triangle:
        body.CreatePolygon(worldPos, 1.0f, Geometry::RegularPolygon(3, 35.0f), 0.5f, false, body);
        world.AddBody(body);
        break;
    case SpawnShape::Pentagon:
        body.CreatePolygon(worldPos, 1.0f, Geometry::RegularPolygon(5, 32.0f), 0.5f, false, body);
        world.AddBody(body);
        break;
    case SpawnShape::Hexagon:
        body.CreatePolygon(worldPos, 1.0f, Geometry::RegularPolygon(6, 32.0f), 0.5f, false, body);
        world.AddBody(body);
        break;
    case SpawnShape::Custom:
        break; // handled by FinishCustomShape() instead
    }
}

void Engine::FinishCustomShape()
{
    if (drawPoints.size() >= 3)
    {
        // SAT (and therefore this whole engine) only handles convex shapes,
        // so a freehand doodle gets wrapped in its convex hull before it
        // becomes a body. A concave outline will visibly "puff out" to its
        // hull - that's expected, not a bug.
        std::vector<Vector2> hull = Geometry::ConvexHull(drawPoints);
        if (hull.size() >= 3)
        {
            Body2D body;
            // Points are already in world space, so position is just {0,0}.
            body.CreatePolygon({0.0f, 0.0f}, 1.0f, hull, 0.5f, false, body);
            world.AddBody(body);
        }
    }
    drawPoints.clear();
    isDrawing = false;
}

void Engine::HandleInput()
{
    Vector2 mouseScreen = GetMousePosition();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        if (HandleButtonClick(mouseScreen))
            return; // clicked a tool button, don't also spawn/place a vertex

        if (!IsOverUI(mouseScreen))
        {
            Vector2 worldPos = GetScreenToWorld2D(mouseScreen, camera);
            if (currentShape == SpawnShape::Custom)
            {
                isDrawing = true;
                drawPoints.push_back(worldPos);
            }
            else
            {
                SpawnPreset(worldPos);
            }
        }
    }

    if (currentShape == SpawnShape::Custom && isDrawing)
    {
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || IsKeyPressed(KEY_ENTER))
            FinishCustomShape();
        else if (IsKeyPressed(KEY_ESCAPE))
        {
            drawPoints.clear();
            isDrawing = false;
        }
    }
}

void Engine::DrawUI()
{
    for (const ShapeButton &button : buttons)
    {
        bool active = (button.shape == currentShape);
        DrawRectangleRec(button.rect, active ? SKYBLUE : LIGHTGRAY);
        DrawRectangleLinesEx(button.rect, 2.0f, DARKGRAY);
        int textWidth = MeasureText(button.label, 16);
        DrawText(button.label,
                 (int)(button.rect.x + (button.rect.width - textWidth) / 2.0f),
                 (int)(button.rect.y + (button.rect.height - 16) / 2.0f),
                 16, BLACK);
    }

    if (currentShape == SpawnShape::Custom)
        DrawText("Left-click: add point   Right-click / Enter: finish shape   Esc: cancel", 10, 50, 16, DARKGRAY);
}

void Engine::DrawCustomPreview()
{
    for (size_t i = 0; i + 1 < drawPoints.size(); i++)
        DrawLineV(drawPoints[i], drawPoints[i + 1], DARKGREEN);
    for (const Vector2 &p : drawPoints)
        DrawCircleV(p, 3.0f, DARKGREEN);
}

void Engine::bodydraw()
{
    std::vector<Body2D> &bodies = world.GetBodies();

    for (Body2D &body : bodies)
        body.Draw();

    DrawCustomPreview(); // world-space preview of the in-progress freehand shape

    // The previous version never closed the camera's 2D mode, so every HUD
    // element after this point (button bar, body count, frame time) was
    // silently being drawn in world/camera space instead of screen space.
    EndMode2D();

    DrawUI();
    DrawText(std::to_string(bodies.size()).c_str(), 10, 90, 20, BLACK);
    DrawText(TextFormat("Frame Time: %.3f ms", GetFrameTime() * 1000.0f), 10, 115, 20, BLACK);
    DrawFPS(GetScreenWidth() - 90, 10);
}

void Engine::camerahandle()
{
    camera.offset = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
    camera.zoom += ((float)GetMouseWheelMove() * 0.05f);
    if (camera.zoom > 3.0f)
        camera.zoom = 3.0f;
    else if (camera.zoom < 0.1f)
        camera.zoom = 0.1f;
    BeginMode2D(camera);
}

void Engine::Update()
{
    std::vector<Body2D> &bodies = world.GetBodies();
    float dt = GetFrameTime();
    world.Step(dt, 64);

    // Iterate by index in reverse rather than by reference in a range-for:
    // the previous version called world.RemoveBody() (which erases from the
    // very vector being ranged over) mid-iteration, which is undefined
    // behavior in C++ once an element is erased out from under a live
    // iterator. Reverse index iteration keeps every not-yet-visited index
    // valid even as later elements are erased.
    for (int i = (int)bodies.size() - 1; i >= 0; i--)
    {
        if (bodies[i].aabb.max.y >= 400)
            world.RemoveBodyAt(i);
    }
}
