#ifndef GLTRACER_H
#define GLTRACER_H

#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

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
    glm::mat4 m_projectionMatrix = glm::mat4(1.0);
    glm::vec2 m_viewportBounds = glm::vec2(0.0);
    glm::vec2 m_viewportPadding = glm::vec2(0.0);
    kdTree* m_kdTree = 0;
    Grid* m_grid = 0;

    // GPU
    GLFWwindow* m_window = 0;

    GLuint m_vertexBuffer = 0;
    GLuint m_texCoordBuffer = 0;

    GLuint m_framebuffer = 0;
    GLuint m_screenColorTexture = 0;
    GLuint m_screenDepthTexture = 0;

    GLuint m_objectInfoTex = 0;
    GLuint m_objectInfoTBO = 0;

    GLuint m_accellStructureTex = 0;
    GLuint m_accellStructureTBO = 0;

    GLuint m_objectRefTex = 0;
    GLuint m_objectRefTBO = 0;

    ShaderProgram* m_basicVS = 0;
    ShaderProgram* m_basicFS = 0;
    ShaderProgram* m_raytracerFS = 0;

    GLuint m_basicProgram = 0;
    GLint m_uniform_ScreenTexture = 0;

    GLuint m_raytracerProgram = 0;
    GLint m_uniform_CameraPos = 0;
    GLint m_uniform_CameraRot = 0;
    GLint m_uniform_CameraRotInverse = 0;
    GLint m_uniform_AmbientIntensity = 0;
    GLint m_uniform_SkyLightColor = 0;
    GLint m_uniform_SkyLightDirection = 0;
};

#endif // GLTRACER_H
