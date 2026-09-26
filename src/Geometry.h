#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "raylib.h"
#include <vector>

// Small set of standalone geometry helpers used to turn user input (mouse
// clicks) or a simple side-count into a set of local-space polygon vertices
// that Body2D::CreatePolygon can consume.
namespace Geometry
{
    // Computes the convex hull of an arbitrary point set using Andrew's
    // monotone chain algorithm (O(n log n)). Returned points are ordered
    // around the hull (consistent winding, no duplicate closing point).
    // Physics (SAT) requires convex shapes, so this is what turns a
    // freehand doodle into something the collision system can simulate.
    // Returns an empty vector if fewer than 3 distinct points remain
    // after de-duplication (i.e. not a valid polygon).
    std::vector<Vector2> ConvexHull(std::vector<Vector2> points);

    // Generates the local-space vertices of a regular N-sided polygon
    // (N clamped to >= 3) with the given circumradius, centered on the
    // origin. Used for the Triangle / Pentagon / Hexagon presets.
    std::vector<Vector2> RegularPolygon(int sides, float circumradius);
}

#endif // GEOMETRY_H
