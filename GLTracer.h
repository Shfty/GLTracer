#ifndef GLTRACER_H
#define GLTRACER_H

#include <vector>

#include <GL/glew.h>
#include <GL/glfw.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "ShaderProgram.h"
#include "Primitive.h"
#include "Camera.h"
#include "Grid.h"
#include "kdTree.h"

class GLTracer
{
public:
    GLTracer();
    ~GLTracer();

    void Update();
    void Draw();

    void BufferPrimitive( const Primitive* Primitive, const int idx );

private:
    //DEBUG
    void walkKDTree();

    void initGL();
    void terminateGL();
    void setupRenderTexture();
    void setupVertexBuffer();
    void setupShaders();

    void generateObjectInfoTex();
    void bufferPrimitives( std::vector< Primitive* > primitives );

    void generateGridTex();
    void bufferGrid();

    void generateKDTreeTex();
    void bufferKDTree();

    static void GLFWCALL callbackResizeWindow( const int width, const int height );
    static int GLFWCALL callbackCloseWindow( void );

    // CPU
    Camera* m_camera;
    glm::vec2 m_viewportBounds;
    glm::vec2 m_viewportPadding;
    kdTree* m_kdTree;
    Grid* m_grid;

    // GPU
    GLuint m_vertexBuffer;
    GLuint m_texCoordBuffer;

    GLuint m_framebuffer;
    GLuint m_screenTexture;

    GLuint m_objectInfoTex;
    GLuint m_objectInfoTBO;

    GLuint m_accellStructureTex;
    GLuint m_accellStructureTBO;

    GLuint m_objectRefTex;
    GLuint m_objectRefTBO;

    ShaderProgram* m_basicVS;
    ShaderProgram* m_basicFS;
    ShaderProgram* m_raytracerFS;

    GLuint m_raytracerProgram;
        GLint m_uniform_CameraPos;
        GLint m_uniform_CameraRot;
        GLint m_uniform_AmbientIntensity;
        GLint m_uniform_SkyLightColor;
        GLint m_uniform_SkyLightDirection;

    GLuint m_basicProgram;
        GLint m_uniform_ScreenTexture;
};

#endif // GLTRACER_H
