#include "Geometry.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>

namespace
{
    // Cross product of (A - O) and (B - O). Positive => counter-clockwise turn.
    float Cross(const Vector2 &O, const Vector2 &A, const Vector2 &B)
    {
        return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
    }
}

namespace Geometry
{
    std::vector<Vector2> ConvexHull(std::vector<Vector2> points)
    {
        if (points.size() < 3)
            return {};

        std::sort(points.begin(), points.end(), [](const Vector2 &a, const Vector2 &b)
                  { return (a.x < b.x) || (a.x == b.x && a.y < b.y); });

        points.erase(std::unique(points.begin(), points.end(), [](const Vector2 &a, const Vector2 &b)
                                  { return Vector2Equals(a, b) != 0; }),
                     points.end());

        size_t n = points.size();
        if (n < 3)
            return {};

        std::vector<Vector2> hull(2 * n);
        int k = 0;

        // Lower hull.
        for (size_t i = 0; i < n; i++)
        {
            while (k >= 2 && Cross(hull[k - 2], hull[k - 1], points[i]) <= 0.0f)
                k--;
            hull[k++] = points[i];
        }

        // Upper hull.
        for (int i = (int)n - 2, lowerSize = k + 1; i >= 0; i--)
        {
            while (k >= lowerSize && Cross(hull[k - 2], hull[k - 1], points[i]) <= 0.0f)
                k--;
            hull[k++] = points[i];
        }

        hull.resize(k - 1); // last point duplicates the first, drop it
        if (hull.size() < 3)
            return {};
        return hull;
    }

    std::vector<Vector2> RegularPolygon(int sides, float circumradius)
    {
        if (sides < 3)
            sides = 3;

        std::vector<Vector2> vertices;
        vertices.reserve(sides);
        for (int i = 0; i < sides; i++)
        {
            float angle = 2.0f * PI * (float)i / (float)sides;
            vertices.push_back({circumradius * cosf(angle), circumradius * sinf(angle)});
        }
        return vertices;
    }
}
