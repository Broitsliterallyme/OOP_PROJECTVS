#include "Body2D.h"

namespace
{
    struct PolygonMassData
    {
        float area;      // unsigned
        Vector2 centroid; // in the same space the input vertices were given
        float inertia;    // about the centroid, per unit density
    };

    // Triangle-fan mass/centroid/inertia computation for an arbitrary convex
    // polygon. Fans out from vertices[0] rather than the origin so the math
    // stays numerically stable even when the polygon sits far from (0,0) -
    // e.g. a shape drawn wherever the player happened to click on a large
    // world. This is the same well-known approach used by most 2D physics
    // engines for polygon mass properties.
    PolygonMassData ComputePolygonMassData(const std::vector<Vector2> &verts)
    {
        PolygonMassData data{0.0f, verts.empty() ? Vector2{0, 0} : verts[0], 0.0f};
        if (verts.size() < 3)
            return data;

        const Vector2 origin = verts[0];
        const float inv3 = 1.0f / 3.0f;

        float area = 0.0f;
        Vector2 center = {0.0f, 0.0f};
        float inertia = 0.0f;

        for (size_t i = 1; i + 1 < verts.size(); i++)
        {
            Vector2 e1 = Vector2Subtract(verts[i], origin);
            Vector2 e2 = Vector2Subtract(verts[i + 1], origin);

            float d = e1.x * e2.y - e1.y * e2.x; // 2x signed triangle area
            float triangleArea = 0.5f * d;
            area += triangleArea;

            center.x += triangleArea * inv3 * (e1.x + e2.x);
            center.y += triangleArea * inv3 * (e1.y + e2.y);

            float intx2 = e1.x * e1.x + e1.x * e2.x + e2.x * e2.x;
            float inty2 = e1.y * e1.y + e1.y * e2.y + e2.y * e2.y;
            inertia += (0.25f * inv3 * d) * (intx2 + inty2);
        }

        if (fabsf(area) < 1e-8f)
            return data; // degenerate (zero-area) polygon, caller should ignore

        center = Vector2Scale(center, 1.0f / area);
        Vector2 centroidLocal = center;               // relative to `origin`
        Vector2 centroidAbsolute = Vector2Add(origin, center);

        // Shift inertia from being about `origin` to being about the centroid.
        float centroidInertia = inertia - area * Vector2DotProduct(centroidLocal, centroidLocal);

        data.area = fabsf(area);
        data.centroid = centroidAbsolute;
        data.inertia = fabsf(centroidInertia);
        return data;
    }
}

Body2D::Body2D(Vector2 position, float density, float mass, float inertia, float restitution, float area,
               bool isstatic, float radius, ShapeType shapeType, std::vector<Vector2> localVertices)
    : Position(position), Density(density), Mass(mass), Inertia(inertia), Restitution(restitution),
      Area(area), isStatic(isstatic), Radius(radius), shape(shapeType)
{
    Velocity = {0, 0};
    Acceleration = {0, 0};
    Force = {0, 0};
    RotationalVelocity = 0.0f;
    Rotation = 0.0f;
    RollingFriction = 0.0f;
    aabbUpdate = true;
    updatevertices = true;

    if (shapeType == ShapeType::Circle)
    {
        StaticFriction = 0.2f;
        DynamicFriction = 0.2f;
    }
    else
    {
        StaticFriction = 0.4f;
        DynamicFriction = 0.17f;
        vertices = std::move(localVertices);
        transformedvertices.resize(vertices.size());
    }

    if (isStatic)
    {
        InvMass = 0.0f;
        InvInteria = 0.0f;
    }
    else
    {
        InvMass = (mass > 0.0f) ? 1.0f / mass : 0.0f;
        InvInteria = (inertia > 0.0f) ? 1.0f / inertia : 0.0f;
    }
}

Body2D::Body2D()
    : Position({0, 0}), Density(0), Mass(0), InvMass(0), Inertia(0), InvInteria(0), Restitution(0),
      StaticFriction(0), DynamicFriction(0), Area(0), isStatic(false), Radius(0),
      updatevertices(true), aabbUpdate(true), shape(ShapeType::Circle)
{
}

void Body2D::CreateCircle(Vector2 position, float density, float radius, float restitution, bool isstatic, Body2D &body)
{
    float area = PI * radius * radius;
    float mass = area * density;
    float inertia = 0.5f * mass * radius * radius;
    body = Body2D(position, density, mass, inertia, restitution, area, isstatic, radius, ShapeType::Circle, {});
}

void Body2D::CreateRectangle(Vector2 position, float density, float length, float width, float restitution, bool isstatic, Body2D &body)
{
    // Kept for backwards compatibility with existing call sites (length = height
    // dimension, width = width dimension, matching the original API). A
    // rectangle is just a 4-vertex convex polygon, so this now forwards to
    // CreatePolygon instead of duplicating the mass/inertia math.
    std::vector<Vector2> localVerts(4);
    localVerts[0] = {-width / 2.0f, -length / 2.0f};
    localVerts[1] = {width / 2.0f, -length / 2.0f};
    localVerts[2] = {width / 2.0f, length / 2.0f};
    localVerts[3] = {-width / 2.0f, length / 2.0f};
    body.CreatePolygon(position, density, localVerts, restitution, isstatic, body);
}

void Body2D::CreatePolygon(Vector2 position, float density, std::vector<Vector2> verts, float restitution, bool isstatic, Body2D &body)
{
    if (verts.size() < 3)
        return; // not a valid polygon; silently ignore rather than crash

    PolygonMassData massData = ComputePolygonMassData(verts);
    if (massData.area <= 0.0f)
        return; // degenerate (zero-area / collinear) input

    std::vector<Vector2> localVertices(verts.size());
    for (size_t i = 0; i < verts.size(); i++)
        localVertices[i] = Vector2Subtract(verts[i], massData.centroid);

    Vector2 worldPosition = Vector2Add(position, massData.centroid);
    float mass = massData.area * density;
    float inertia = massData.inertia * density;

    body = Body2D(worldPosition, density, mass, inertia, restitution, massData.area, isstatic, 0.0f,
                  ShapeType::Polygon, localVertices);
}

void Body2D::Draw()
{
    if (shape == ShapeType::Circle)
    {
        DrawCircleV(Position, Radius, BLUE);
    }
    else
    {
        // DrawFilledPolygon already fans out from vertex 0, so it works
        // unchanged for any vertex count - triangle, hexagon, or a 20-point
        // hand-drawn hull.
        DrawFilledPolygon(transformedvertices, RED);
    }
    // DrawRectangleLines(aabb.min.x, aabb.min.y, aabb.max.x - aabb.min.x, aabb.max.y - aabb.min.y, GREEN);
}

void Body2D::getTransformedVertices()
{
    if (updatevertices)
    {
        BodyTransform transform(Position, Rotation);
        transform.Transform(transform, vertices, transformedvertices);
        updatevertices = false;
    }
}

void Body2D::GetAABB()
{
    if (aabbUpdate)
    {
        float minx = std::numeric_limits<float>::max();
        float miny = std::numeric_limits<float>::max();
        float maxx = std::numeric_limits<float>::lowest();
        float maxy = std::numeric_limits<float>::lowest();

        if (shape == ShapeType::Circle)
        {
            minx = Position.x - Radius;
            maxx = Position.x + Radius;
            miny = Position.y - Radius;
            maxy = Position.y + Radius;
        }
        else
        {
            // Was previously hardcoded to 4 vertices; now scales to any N.
            for (size_t i = 0; i < transformedvertices.size(); i++)
            {
                minx = std::min(minx, transformedvertices[i].x);
                maxx = std::max(maxx, transformedvertices[i].x);
                miny = std::min(miny, transformedvertices[i].y);
                maxy = std::max(maxy, transformedvertices[i].y);
            }
        }
        aabb.min = {minx, miny};
        aabb.max = {maxx, maxy};
        aabbUpdate = false;
    }
}

void Body2D::Move(Vector2 velocity)
{
    Position.x += velocity.x;
    Position.y += velocity.y;
}

void Body2D::Moveto(Vector2 position)
{
    Position = position;
}
void Body2D::Rotate(float angle)
{
    Rotation += angle;
}
Vector2 Body2D::getPosition() const
{
    return Position;
}
Vector2 Body2D::getVelocity() const
{
    return Velocity;
}
float Body2D::getRadius() const
{
    return Radius;
}
float Body2D::getRestitution() const
{
    return Restitution;
}
float Body2D::getStaticFriction() const
{
    return StaticFriction;
}
float Body2D::getDynamicFriction() const
{
    return DynamicFriction;
}
float Body2D::getRollingFriction() const
{
    return RollingFriction;
}
float Body2D::getMass() const
{
    return Mass;
}
float Body2D::getInvMass() const
{
    return InvMass;
}
float Body2D::getInvInertia() const
{
    return InvInteria;
}
Vector2 Body2D::getForce()
{
    return Force;
}
bool Body2D::isStaticBody() const
{
    return isStatic;
}
void Body2D::addForce(Vector2 force)
{
    Force = force;
}
void Body2D::setVelocity(Vector2 velocity)
{
    Velocity = velocity;
}
float Body2D::getRotation() const
{
    return Rotation;
}
float Body2D::getRotationalVelocity() const
{
    return RotationalVelocity;
}
void Body2D::setRotation(float angle)
{
    Rotation = angle;
}
void Body2D::setRotationalVelocity(float velocity)
{
    RotationalVelocity = velocity;
}
void Body2D::getVertices(std::vector<Vector2> &Ref) const
{
    Ref = transformedvertices;
}
void Body2D::step(float dt, float gravity)
{
    if (!isStatic)
    {
        Velocity = Vector2Add(Velocity, Vector2Scale(Vector2{0, gravity}, dt));
        Position.x += Velocity.x * dt;
        Position.y += Velocity.y * dt;
        Rotation += RotationalVelocity * dt;
    }
    // Acceleration = Vector2Scale(Force, 1.0f / Mass);

    Force = {0, 0};
    updatevertices = true;
    aabbUpdate = true;
    GetAABB();
    getTransformedVertices();
}
