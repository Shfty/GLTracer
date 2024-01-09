#ifndef COLLISIONS_H
#define COLLISIONS_H

#include <glm/glm.hpp>
using namespace glm;

#include "Ray.h"
#include "Primitive.h"

static Ray localRay( const Ray& ray, const vec3& position, const quat& orientation, const vec3& scale )
{
    Ray lRay = ray;

    lRay.Origin -= position;
    lRay.Origin = conjugate( orientation ) * lRay.Origin;
    lRay.Origin *= vec3( 1.0 ) / scale;

    lRay.Direction = conjugate( orientation ) * lRay.Direction;
    lRay.Direction *= vec3( 1.0 ) / scale;

    return lRay;
}

static IsectData worldIsectData( const IsectData& isectData, const vec3& position, const quat& orientation, const vec3& scale )
{
    IsectData wIsectData = isectData;

    wIsectData.Position *= scale;
    wIsectData.Position = orientation * wIsectData.Position;
    wIsectData.Position += position;

    wIsectData.Normal *= scale;
    wIsectData.Normal = orientation * wIsectData.Normal;
    wIsectData.Normal = normalize( wIsectData.Normal );

    return wIsectData;
}

namespace Collisions
{
    extern float NEAR_PLANE;
    extern float FAR_PLANE;
    extern vec3 PLANE_NORMAL;

    // Utility
    extern vec3 cardinalDirection( vec3 v );

    // Point Containment
    extern bool SphereContainsPoint( const vec3& pt, const vec3& spherePos, const quat& sphereOrientation, const vec3& sphereScale );
    extern bool AABBContainsPoint( const vec3& pt, const vec3& b0, const vec3& b1 );

    // Primitive-Primitive Intersection
    extern bool AABBContainsAABB( const vec3& a0, const vec3& a1, const vec3& b0, const vec3& b1 );
    extern bool PlaneIntersectsAABB( const vec3& planePosition, const vec3& planeNormal, const vec3& p0, const vec3& p1 );

    // Ray Intersection (Generic)
    extern bool IsectPlane(
        const Ray& ray,
        const vec3& planePosition,
        const vec3& planeNormal,
        IsectData& isectData
        );

    extern bool IsectAABB(
        const Ray& ray,
        const vec3& p0,
        const vec3& p1,
        IsectData& isectData
        );

    // Ray Intersection (Object)
    extern bool IsectPlanePrimitive(
        const Ray& ray,
        const Primitive* object,
        IsectData& isectData
        );

    extern bool IsectSpherePrimitive(
        const Ray& ray,
        const Primitive* object,
        IsectData& isectData
        );

    extern bool IsectDiscPrimitive(
        const Ray& ray,
        const Primitive* object,
        IsectData& isectData
    );

    extern bool IsectConvexPolyPrimitive(
        const Ray& ray,
        const Primitive* object,
        IsectData& isectData
        );
}

#endif // COLLISIONS_H
