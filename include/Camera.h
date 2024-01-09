#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <vector>

#include "Ray.h"
#include "Primitive.h"

class Camera
{
public:
    Camera();

    void Update( const std::vector<Primitive*>& primitives );

    glm::vec3 GetPosition() const { return m_position; };
    glm::mat4 GetRotation() const { return m_cameraRotX * m_cameraRotY; }
    glm::vec3 GetWarpFactor() const { return m_warpFactor; }

private:
    void portalTransport( const Primitive* portal, const IsectData& isectData );
    void updateWarpFactor( const std::vector<Primitive*>& primitives );

    glm::vec3 m_position = glm::vec3( 10.0f, 0, 15.0f );
    glm::vec3 m_prevPosition = m_position;
    glm::mat4 m_rotation;
    glm::mat4 m_cameraRotX;
    glm::mat4 m_cameraRotY;

    glm::vec3 m_warpFactor = glm::vec3( 1.0f );
};

#endif // CAMERA_H
