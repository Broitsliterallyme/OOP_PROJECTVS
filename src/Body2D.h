#ifndef BODY2D_H
#define BODY2D_H

#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <cmath>
#include "Polygon.h"
#include "Transform.h"
#include "AABB.h"
#include "limits"

// Circle keeps its own fast-path (radius test, no vertex list).
// Everything else - a hand-built rectangle, a regular pentagon, or a
// convex hull traced out by the player - is a Polygon: an arbitrary,
// convex, N-vertex shape. Box no longer exists as a distinct case; a
// rectangle is simply a 4-vertex Polygon, which is why CreateRectangle
// now just forwards to CreatePolygon.
enum class ShapeType
{
    Circle,
    Polygon
};

class Body2D
{
private:
    Vector2 Position;
    Vector2 Velocity;
    Vector2 Acceleration;
    Vector2 Force;
    float Rotation;
    float RotationalVelocity;
    float Density;
    float Mass;
    float InvMass;
    float Inertia;
    float InvInteria;
    float Restitution;
    float StaticFriction;
    float DynamicFriction;
    float RollingFriction = 1.0f;
    float Area;
    bool isStatic;
    float Radius;
    bool updatevertices;
    std::vector<Vector2> vertices;            // local space, relative to the body's own centroid
    std::vector<Vector2> transformedvertices;  // world space, rebuilt lazily when the body moves/rotates
    bool aabbUpdate;

    // Precomputed mass, inertia (already multiplied by density) and centroid are
    // passed in directly rather than recomputed here, so every shape - circle,
    // rectangle, or arbitrary N-gon - goes through one uniform code path instead
    // of duplicating per-shape formulas inside the constructor.
    Body2D(Vector2 position, float density, float mass, float inertia, float restitution, float area,
           bool isstatic, float radius, ShapeType shapeType, std::vector<Vector2> localVertices);

public:
    ShapeType shape;
    AABB aabb;
    Body2D();
    void CreateCircle(Vector2 position, float density, float radius, float restitution, bool isstatic, Body2D &body);
    void CreateRectangle(Vector2 position, float density, float length, float width, float restitution, bool isstatic, Body2D &body);

    // Builds an arbitrary convex-polygon body. `vertices` must already describe
    // a convex polygon (use Geometry::ConvexHull first if the points came from
    // free-hand drawing) and must have at least 3 points; anything smaller is
    // silently ignored (no body created). `vertices` may be given either in
    // local space around the origin (e.g. Geometry::RegularPolygon output,
    // paired with `position` = the desired world placement) or directly in
    // world space (paired with `position` = {0,0}) - both are supported
    // because the body's true centroid is computed from the vertices
    // themselves and `position` is just added on top as an offset.
    void CreatePolygon(Vector2 position, float density, std::vector<Vector2> vertices, float restitution, bool isstatic, Body2D &body);

    void Draw();
    void getTransformedVertices();
    void GetAABB();
    void Move(Vector2);
    void Moveto(Vector2);
    void Rotate(float);
    void step(float, float);
    Vector2 getPosition() const;
    Vector2 getVelocity() const;
    float getRotation() const;
    float getRotationalVelocity() const;
    void setRotation(float);
    void setRotationalVelocity(float);
    void addForce(Vector2);
    Vector2 getForce();
    void setVelocity(Vector2);
    float getRestitution() const;
    bool isStaticBody() const;
    float getStaticFriction() const;
    float getDynamicFriction() const;
    float getRollingFriction() const;
    float getMass() const;
    float getInvMass() const;
    float getInvInertia() const;
    float getRadius() const;
    void getVertices(std::vector<Vector2> &) const;
};

#endif // BODY2D_H
