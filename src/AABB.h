#ifndef AABB_H
#define AABB_H

#include "raylib.h"

struct AABB
{
    Vector2 max;
    Vector2 min;
    AABB(Vector2, Vector2);
    AABB(float, float, float, float);
    AABB();
};

#endif // AABB_H
