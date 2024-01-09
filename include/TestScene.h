#ifndef TESTSCENE_H
#define TESTSCENE_H

#include "Scene.h"

class TestScene : public Scene
{
public:
    TestScene( GLTracer* glTracer );
    ~TestScene() {}

    void Update( glm::vec4 skyColor );
};

#endif // TESTSCENE_H
