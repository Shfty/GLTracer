#ifndef Primitive_H
#define Primitive_H

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

struct ObjectMaterial
{
    enum MaterialType { None = -1, SolidColor, Texture, Portal, Spacewarp };
    MaterialType Type = None;
    glm::vec4 Color = glm::vec4(0, 0, 0, 1);

    GLfloat Diffuse = 1.0f;
    GLfloat Specular = 1.0f;
    GLfloat SpecularFactor = 16.0f;
    GLfloat Emissive = 0.0f;

    GLfloat Reflection = 0.0f;
    GLfloat RefractiveIndex = 1.0f;
    GLfloat CastShadow = 1.0f;

    glm::vec3 PortalOffset;
    glm::vec3 PortalAxis;
    float PortalAngle = 0.0f;
};

struct Primitive
{
    enum ObjectType { None = -1, Plane, Sphere, Disc, AABB, ConvexPoly };
    GLfloat ID = 0.0f;
    ObjectType Type = None;
    glm::vec3 Position;
    glm::quat Orientation;
    glm::vec3 Scale = glm::vec3( 1.0f );
    ObjectMaterial Material;
    GLfloat Sides = 3.0f;

    Primitive() {}

    Primitive( ObjectType type,
              glm::vec3 position,
              glm::quat orientation,
              glm::vec3 scale)
              : Type(type),
              Position(position),
              Orientation(orientation),
              Scale(scale)
              {}
};

#endif // Primitive_H
