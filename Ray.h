#ifndef RAY_H
#define RAY_H

#include <glm/glm.hpp>

struct Ray
{
    glm::vec3 Origin;
    glm::vec3 Direction;

    Ray( glm::vec3 origin, glm::vec3 direction )
    {
        Origin = origin;
        Direction = direction;
    }
};

struct IsectData
{
    int HitID; // -1.0 = No hit, >= 0 = Hit Object ID
    float HitType;
    glm::vec3 Position;
    float Distance;
    glm::vec3 Normal;
    glm::vec4 Color;
    glm::vec3 PortalPosition;
    glm::vec3 PortalOffset;
    glm::vec3 PortalAxis;
    float PortalAngle;
};

#endif // RAY_H
