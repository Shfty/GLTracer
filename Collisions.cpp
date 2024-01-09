#include "Collisions.h"

namespace Collisions
{
    float NEAR_PLANE = 0.0f;
    float FAR_PLANE = 100000.0f;
    vec3 PLANE_NORMAL = vec3( 0.0, -1.0, 0.0 );

    // Utility
    // Returns the cardinal ( unit ) direction vector of input vector v
    vec3 cardinalDirection( vec3 v )
    {
        float mi = 0;
        float mv = 0;

        for( int i = 0; i < 3; ++i )
        {
            float avi = abs( v[ i ] );
            float diff = avi - mv;
            float f = clamp( sign( diff ), 0.0f, 1.0f ); // diff > 0
            mv = mix( mv, avi, f );
            mi = mix( mi, float( i ), f );
        }

        vec3 ov = vec3( 0.0f );
        ov[ int( mi ) ] = sign( v[ int( mi ) ] );

        return ov;
    }

    // Point Containment
    bool SphereContainsPoint( const vec3& pt, const vec3& spherePos, const quat& sphereOrientation, const vec3& sphereScale )
    {
        vec3 localPt = pt;
        localPt -= spherePos;
        localPt = conjugate( sphereOrientation ) * localPt;
        localPt *= vec3( 1.0f ) / sphereScale;

        float dist = distance( localPt, vec3( 0 ) );

        if( dist <= 1.0f )
        {
            return true;
        }

        return false;
    }

    bool AABBContainsPoint( const vec3& pt, const vec3& b0, const vec3& b1 )
    {
        bool hit = true;

        for( int i = 0; i < 3; ++i )
        {
            if( pt[ i ] < b0[ i ] ) hit = false;
            if( pt[ i ] >= b1[ i ] ) hit = false;
        }

        return hit;
    }

    // AABB Containment
    bool AABBContainsAABB( const vec3& a0, const vec3& a1, const vec3& b0, const vec3& b1 )
    {
        bool hit = true;

        for( int i = 0; i < 3; ++i )
        {
            if( a0[ i ] > b1[ i ] || a1[ i ] < b0[ i ] )
            {
                hit = false;
            }
        }

        return hit;
    }

    // Primitive-Box Intersection
    bool PlaneIntersectsAABB( const vec3& planePosition, const vec3& planeNormal, const vec3& p0, const vec3& p1 )
    {
        bool result = true;

        vec3 pVertex = p0;
        if( planeNormal.x >= 0 )
            pVertex.x = p1.x;
        if( planeNormal.y >= 0 )
            pVertex.y = p1.y;
        if( planeNormal.z >= 0 )
            pVertex.z = p1.z;

        vec3 nVertex = p1;
        if( planeNormal.x >= 0 )
            nVertex.x = p0.x;
        if( planeNormal.y >= 0 )
            nVertex.y = p0.y;
        if( planeNormal.z >= 0 )
            nVertex.z = p0.z;

        float pDist = dot( planeNormal, nVertex - planePosition );
        float nDist = dot( planeNormal, pVertex - planePosition );

        return pDist < 0 && nDist > 0;
    }

    // Ray Intersection (Generic)
    bool IsectPlane(
        const Ray& ray,
        const vec3& planePosition,
        const vec3& planeNormal,
        IsectData& isectData
        )
    {
        float hit = 1.0f;

        vec3 pn = planeNormal;

        float ndr = dot( pn, ray.Direction );
        hit = mix( 0.0f, hit, ceil( abs( ndr ) ) ); // ndr == 0.0

        float D = glm::dot(pn, planePosition);
        float t = -( glm::dot( pn, ray.Origin ) - D ) / dot( pn, ray.Direction );

        if( hit == 1.0f )
        {
            isectData.Position = ray.Origin + ray.Direction * t;
            isectData.Normal = pn * -sign( ndr );
        }

        return hit == 1.0f;
    }

    bool IsectAABB(
        const Ray& ray,
        const glm::vec3& p0,
        const glm::vec3& p1,
        IsectData& isectData)
    {
        float hit = 1.0;
        float backface = AABBContainsPoint( ray.Origin, p0, p1 );

        // Calculate intersection using the slab method ( clip ray against box per-axis )
        vec3 t0 = ( p0 - ray.Origin ) * vec3( 1.0f ) / ray.Direction;
        vec3 t1 = ( p1 - ray.Origin ) * vec3( 1.0f ) / ray.Direction;
        float tmin = min( t0.x, t1.x );
        float tmax = max( t0.x, t1.x );
        tmin = max( tmin, min( t0.y, t1.y ) );
        tmax = min( tmax, max( t0.y, t1.y ) );
        tmin = max( tmin, min( t0.z, t1.z ) );
        tmax = min( tmax, max( t0.z, t1.z ) );
        hit = step( tmin, tmax );

        // Check hit against ray limits
        float t = mix( tmin, tmax, backface );
        hit = mix( 0.0f, hit, step( NEAR_PLANE, t ) );

        glm::vec3 pt = ray.Origin + ( ray.Direction * t ) ;

        isectData.Position = mix( isectData.Position, pt, hit );
        // NOTE: Normal calculation is incorrect for non-cubic bounding boxes
        isectData.Normal = mix( isectData.Normal, cardinalDirection( pt ) * mix( 1.0f, -1.0f, backface ), hit );

        return hit;
    }

    // Ray Intersection (Object)
    bool IsectPlanePrimitive(
        const Ray& ray,
        const Primitive* object,
        IsectData& isectData
        )
    {
        Ray lr = localRay( ray, object->Position, object->Orientation, object->Scale );

        float hit = 1.0f;

        vec3 pn = PLANE_NORMAL;

        float ndr = dot( pn, lr.Direction );
        hit = mix( 0.0f, hit, ceil( abs( ndr ) ) ); // ndr == 0.0

        float t = -dot( pn, lr.Origin ) / dot( pn, lr.Direction );

        hit = mix( 0.0f, hit, step( NEAR_PLANE, t ) ); // t >= NEAR_PLANE
        hit = mix( hit, 0.0f, step( isectData.Distance, t ) ); // t < isectData.Distance

        isectData.Position = mix( isectData.Position, lr.Origin + ( lr.Direction * t ), hit );
        isectData.Normal = mix( isectData.Normal, pn * -sign( ndr ), hit );
        isectData = worldIsectData( isectData, object->Position, object->Orientation, object->Scale );

        return hit == 1.0f;
    }

    bool IsectSpherePrimitive(
        const Ray& ray,
        const Primitive* object,
        IsectData& isectData
        )
    {
        Ray lr = localRay( ray, object->Position, object->Orientation, object->Scale );

        float hit = 1.0f;
        float backface = SphereContainsPoint( ray.Origin, vec3( 0 ), quat(), vec3( 1 ) );

        vec3 l = -lr.Origin;
        float tca = dot( l, lr.Direction );

        float ds = dot( l, l ) - tca * tca;
        hit = step( ds, 1.0f );

        float thc = sqrt( 1.0f - ds );

        float t0 = tca - thc; // Entry t
        float t1 = tca + thc; // Exit t

        float t = mix( t0, t1, backface );

        hit = mix( 0.0f, hit, step( NEAR_PLANE, t ) );
        hit = mix( hit, 0.0f, step( isectData.Distance, t ) ); // t < isectData.Distance

        isectData.Position = mix( isectData.Position, lr.Origin + ( lr.Direction * t ), hit );
        isectData.Normal = mix( isectData.Normal, normalize( isectData.Position ) * mix( 1.0f, -1.0f, backface ), hit );
        isectData = worldIsectData( isectData, object->Position, object->Orientation, object->Scale );

        return hit == 1.0f;
    }

    bool IsectDiscPrimitive(
        const Ray& ray,
        const Primitive* object,
        IsectData& isectData
    )
    {
        Ray lr = localRay( ray, object->Position, object->Orientation, object->Scale );

        float hit = 1.0f;

        vec3 dn = PLANE_NORMAL;

        // Plane intersection test
        float ndr = dot( dn, lr.Direction );
        hit = mix( 0.0f, hit, ceil( abs( ndr ) ) ); // ndr == 0.0

        float t = -( dot( dn, lr.Origin ) - dot( dn, vec3( 0 ) ) ) / dot( dn, lr.Direction );

        hit = mix( 0.0f, hit, step( NEAR_PLANE, t ) );
        hit = mix( hit, 0.0f, step( isectData.Distance, t ) ); // t < isectData.Distance

        // Disc intersection test
        vec3 pp = lr.Origin + ( lr.Direction * t );
        float d = dot( pp, pp );
        hit = mix( hit, 0.0f, step( 0.5f, d ) ); // d > object.Scale^2

        isectData.Position = mix( isectData.Position, lr.Origin + ( lr.Direction * t ), hit );
        isectData.Normal = mix( isectData.Normal, dn * -sign( ndr ), hit );
        isectData = worldIsectData( isectData, object->Position, object->Orientation, object->Scale );

        return hit == 1.0f;
    }

    bool IsectConvexPolyPrimitive(
        const Ray& ray,
        const Primitive* object,
        IsectData& isectData
        )
    {
        Ray lr = localRay( ray, object->Position, object->Orientation, object->Scale );

        float hit = 1.0f;

        vec3 pn = PLANE_NORMAL;
        // Plane intersection test
        float ndr = dot( pn, lr.Direction );
        //IF ( ndr == 0.0 ) hit = 0.0;
        hit = mix( 0.0f, hit, ceil( abs( ndr ) ) );
        //ENDIF

        float t = -( dot( pn, lr.Origin ) - dot( pn, vec3( 0 ) ) ) / dot( pn, lr.Direction );
        hit = mix( 0.0f, hit, step( NEAR_PLANE, t ) );
        hit = mix( hit, 0.0f, step( isectData.Distance, t ) ); // t < isectData.Distance

        // Poly intersection test
        vec3 pt = lr.Origin + ( lr.Direction * t ) ;

        vec3 bv = vec3( 0.0f, 0.0f, 0.5f );
        bv = normalize( angleAxis( glm::pi< float >() / object->Sides, vec3( 0.0f, 1.0f, 0.0f ) ) * bv );

        for( int i = 0; i < int( object->Sides ); ++i )
        {
            vec3 v0 = bv;

            bv = normalize( angleAxis( ( glm::pi< float >() * 2 ) / object->Sides, vec3( 0.0f, 1.0f, 0.0f ) ) * bv );
            vec3 v1 = bv;

            vec3 edge = normalize( v1 - v0 );

            vec3 c = normalize( pt - v0 );
            if( dot( pn, cross( edge, c ) ) > 0.0f ) hit = 0.0f; // dot( pn, cross( edge, c ) ) < 0.0
        }

        isectData.Position = mix( isectData.Position, lr.Origin + ( lr.Direction * t ), hit );
        isectData.Normal = mix( isectData.Normal, pn * -sign( ndr ), hit );
        isectData = worldIsectData( isectData, object->Position, object->Orientation, object->Scale );

        return hit == 1.0f;
    }

}
