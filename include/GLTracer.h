#ifndef GLTRACER_H
#define GLTRACER_H

#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "ShaderProgram.h"
#include "Primitive.h"
#include "Camera.h"
#include "accell/Grid.h"
#include "accell/kdTree.h"

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
    void compileShaders();
    void setupUniforms();

    void generateObjectInfoTex();
    void bufferPrimitives( std::vector< Primitive* > primitives );

    void generateGridTex();
    void bufferGrid();

    void generateKDTreeTex();
    void bufferKDTree();

    static void callbackResizeWindow( GLFWwindow* window, int width, int height );
    static void callbackCloseWindow( GLFWwindow* window );
    static void callbackFocusWindow( GLFWwindow* window, int focused );

    // CPU
    Camera* m_camera = 0;
    glm::mat4 m_projectionMatrix;
    glm::vec2 m_viewportBounds;
    glm::vec2 m_viewportPadding;
    kdTree* m_kdTree = 0;
    Grid* m_grid = 0;

    // GPU
    GLFWwindow* m_window = 0;

    GLuint m_vertexBuffer;
    GLuint m_texCoordBuffer;

    GLuint m_framebuffer;
    GLuint m_screenColorTexture;
    GLuint m_screenDepthTexture;

    GLuint m_objectInfoTex;
    GLuint m_objectInfoTBO;

    GLuint m_accellStructureTex;
    GLuint m_accellStructureTBO;

    GLuint m_objectRefTex;
    GLuint m_objectRefTBO;

    ShaderProgram* m_basicVS = 0;
    ShaderProgram* m_basicFS = 0;
    ShaderProgram* m_raytracerFS = 0;

    GLuint m_basicProgram;
        GLint m_uniform_ScreenTexture;

    GLuint m_raytracerProgram;
        GLint m_uniform_CameraPos;
        GLint m_uniform_CameraRot;
        GLint m_uniform_CameraRotInverse;
        GLint m_uniform_AmbientIntensity;
        GLint m_uniform_SkyLightColor;
        GLint m_uniform_SkyLightDirection;
};

#endif // GLTRACER_H
