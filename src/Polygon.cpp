#include "Polygon.h"

void DrawFilledPolygon(std::vector<Vector2> &points, Color color)
{
    if (points.size() < 3)
        return;

    // raylib's DrawTriangle only fills a triangle when its vertices are
    // given in the winding order it expects - feed it the opposite order
    // and it silently draws nothing. Our vertices (from CreateRectangle,
    // Geometry::RegularPolygon, Geometry::ConvexHull, etc.) come out in the
    // opposite winding once you account for raylib's screen space having Y
    // pointing down, so the last two points of every fan triangle are
    // swapped here to correct it.
    for (int i = 1; i < (int)points.size() - 1; i++)
    {
        DrawTriangle(points[0], points[i + 1], points[i], color);
    }
}

void DrawPolygonLine(std::vector<Vector2> &points, Color color)
{
    if (points.size() < 3)
        return;

    for (int i = 0; i < (int)points.size(); i++)
    {
        DrawLineV(points[i], points[(i + 1) % points.size()], color);
    }
}