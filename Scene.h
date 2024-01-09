#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <map>

#include "GLTracer.h"
#include "Primitive.h"

class Scene
{
public:
    Scene( GLTracer* glTracer );
    virtual ~Scene();

    virtual void Update();
    std::vector<Primitive*> GetObjects() const { return m_primitives; }
    std::vector<Primitive*> GetUpdatedObjects() const { return m_primitives; }

protected:
    void addPrimitive( Primitive* Primitive );
    void updatePrimitive( Primitive* Primitive );
    void generatePortalTransform( Primitive* objA, Primitive* objB );

    GLTracer* m_glTracer;
    std::vector<Primitive*> m_primitives;
    std::map< int, Primitive* > m_updatedObjects;
};

#endif // SCENE_H
