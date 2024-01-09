#include "Scene.h"

//#define DEBUG

#include <iostream>
#include <algorithm>

Scene::Scene( GLTracer* glTracer )
{
    m_glTracer = glTracer;
}

Scene::~Scene()
{

}

void Scene::Update() // Must be called at the top of overridden Update() method
{
    m_updatedObjects.clear();
}

// Performs pre-processing on world objects in prep for sending to OpenGL
void Scene::addPrimitive( Primitive* Primitive )
{
    // Assign unique per-primitive ID
    Primitive->ID = m_primitives.size();

    m_primitives.push_back( Primitive );
}

// Performs pre-processing on world object, then update's it at it's info texture index
void Scene::updatePrimitive( Primitive* primitive )
{
    std::vector<Primitive*>::iterator it = std::find(m_primitives.begin(), m_primitives.end(), primitive);
    int index = std::distance( m_primitives.begin(), it );

    m_updatedObjects.insert( std::pair< int, Primitive* >( index, primitive ) );
}

// Calculates and updates the translation offset
// and rotation axis/angle for a pair of portals
void Scene::generatePortalTransform( Primitive* objA, Primitive* objB )
{
#ifdef DEBUG
    std::cout << "Generating portal transform for objects of type " << objA->Type << std::endl;
#endif

    glm::quat diff = objB->Orientation * glm::inverse( objA->Orientation );
    glm::vec3 axis = glm::axis( diff );
    float angle = glm::angle( diff );

    objA->Material.PortalAxis = -axis;
    objA->Material.PortalAngle = angle;
    objB->Material.PortalAxis = axis;
    objB->Material.PortalAngle = angle;
#ifdef DEBUG
    std::cout << "Rotation: Axis: X: " << axis.x << " Y: " << axis.y << " Z: " << axis.z << " Angle: " << angle << std::endl;
#endif

    glm::vec3 translation = objB->Position - objA->Position;
    objA->Material.PortalOffset = translation;
    objB->Material.PortalOffset = -translation;
#ifdef DEBUG
    std::cout << "Translation: X: " << translation.x << " Y: " << translation.y << " Z: " << translation.z << std::endl << std::endl;
#endif
}
