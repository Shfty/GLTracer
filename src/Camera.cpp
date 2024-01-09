#include "Camera.h"

#include <GLFW/glfw3.h>
#include <time.h>

#include "WorldClock.h"
#include "Collisions.h"
#include "Controls.h"

const float TRANSLATE_PER_SEC = 20.0f;
int previousWorldClock;

Camera::Camera()
{
    Controls::SetMouseLock( true );
}

void Camera::Update( const std::vector<Primitive*>& primitives )
{
    float deltaTime = WorldClock::Instance()->DeltaTime();

    // Rotation
    glm::ivec2 mousePos = Controls::GetMousePos();
    glm::ivec2 deltaMousePos = Controls::MouseOrigin - mousePos;
    if( Controls::GetMouseLock() )
    {
        Controls::ResetMousePos();
    }

    float deltaAngleX = deltaMousePos.x * 0.0009f;
    float deltaAngleY = deltaMousePos.y * 0.0009f;

    m_cameraRotX *= glm::rotate( deltaAngleX, glm::vec3( 0, 1, 0 ) );
    m_cameraRotY *= glm::rotate( deltaAngleY, glm::vec3( 1, 0, 0 ) );

    // Position
    float deltaPosition = TRANSLATE_PER_SEC * deltaTime;
    m_prevPosition = m_position;

    if( Controls::Left() )
    {
        m_position += glm::vec3( m_cameraRotX * glm::vec4(-deltaPosition, 0, 0, 1 ) ) * m_warpFactor;
    }
    if( Controls::Right() )
    {
        m_position += glm::vec3( m_cameraRotX * glm::vec4( deltaPosition, 0, 0, 1 ) ) * m_warpFactor;
    }
    if( Controls::Forward() )
    {
        m_position += glm::vec3( m_cameraRotX * glm::vec4( 0, 0, -deltaPosition, 1 ) ) * m_warpFactor;
    }
    if( Controls::Back() )
    {
        m_position += glm::vec3( m_cameraRotX * glm::vec4( 0, 0, deltaPosition, 1 ) ) * m_warpFactor;
    }
    if( Controls::Up() )
    {
        m_position += glm::vec3( m_cameraRotX * glm::vec4( 0,  deltaPosition, 0, 1 ) ) * m_warpFactor;
    }
    if( Controls::Down() )
    {
        m_position += glm::vec3( m_cameraRotX * glm::vec4( 0, -deltaPosition, 0, 1 ) ) * m_warpFactor;
    }

    // Check for portal collision and transfer if necessary
    if( m_position != m_prevPosition )
    {
        Ray movementVector( m_prevPosition, normalize( m_position - m_prevPosition ) );

        for( int i = 0; i < primitives.size(); ++i )
        {
            IsectData isectData;
            isectData.Distance = glm::distance( m_position, m_prevPosition );
            if( primitives[ i ]->Material.Type == ObjectMaterial::Portal )
            {
                switch( primitives[ i ]->Type )
                {
                case Primitive::Sphere:
                    if( Collisions::IsectSpherePrimitive( movementVector, primitives[ i ], isectData ) )
                    {
                        portalTransport( primitives[ i ], isectData );
                    }
                    break;
                case Primitive::Disc:
                    if( Collisions::IsectDiscPrimitive( movementVector, primitives[ i ], isectData ) )
                    {
                        portalTransport( primitives[ i ], isectData );
                    }
                    break;
                case Primitive::ConvexPoly:
                    if( Collisions::IsectConvexPolyPrimitive( movementVector, primitives[ i ], isectData ) )
                    {
                        portalTransport( primitives[ i ], isectData );
                    }
                    break;
                }
            }
        }
    }

    updateWarpFactor( primitives );
}

void Camera::portalTransport( const Primitive* portal, const IsectData& isectData )
{
    mat4 portalRotation = mat4( 1.0f );
    if( portal->Material.PortalAngle != 0.0f )
    {
        portalRotation = rotate( -portal->Material.PortalAngle, portal->Material.PortalAxis );
    }
    vec3 outNormal = vec3( portalRotation * vec4( isectData.Normal, 1.0f ) );
    vec3 inOriginRelative = isectData.Position - portal->Position;
    vec3 inOriginRelativeRotated = vec3( portalRotation * vec4( inOriginRelative, 1.0f ) );
    m_position = inOriginRelativeRotated + ( portal->Position + portal->Material.PortalOffset ) - outNormal * 0.1f;
    m_cameraRotX *= portalRotation;
    //m_cameraRotY *= portalRotation;
}

void Camera::updateWarpFactor( const std::vector<Primitive*>& primitives )
{
    m_warpFactor = vec3( 1.0f );

    for( uint16_t i = 0; i < primitives.size(); ++i )
    {
        if( primitives[ i ]->Material.Type == ObjectMaterial::Spacewarp )
        {
            switch( primitives[ i ]->Type )
            {
                case Primitive::Sphere:
                {
                    if( Collisions::SphereContainsPoint( m_position, primitives[ i ]->Position, primitives[ i ]->Orientation, primitives[ i ]->Scale ) )
                    {
                        m_warpFactor = primitives[ i ]->Material.PortalOffset;
                    }
                    break;
                }
                case Primitive::AABB:
                {
                    glm::vec3 b0 = primitives[ i ]->Position - primitives[ i ]->Scale * 0.5f;
                    glm::vec3 b1 = primitives[ i ]->Position + primitives[ i ]->Scale * 0.5f;
                    if( Collisions::AABBContainsPoint( m_position, b0, b1 ) )
                    {
                        m_warpFactor = primitives[ i ]->Material.PortalOffset;
                    }
                    break;
                }
                default:
                {
                    break;
                }
            }
        }
    }
}
