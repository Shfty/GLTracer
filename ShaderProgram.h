#ifndef SHADERPROGRAM_H
#define SHADERPROGRAM_H

#include <string>
#include <GL/glew.h>
#include <GL/glfw.h>

class ShaderProgram
{
public:
    ShaderProgram( const std::string& path, const GLenum shaderType );
    ~ShaderProgram();

    const GLuint GetID() const { return m_shaderID; };

private:
    GLuint m_shaderID = 0;
};

#endif // SHADERPROGRAM_H
