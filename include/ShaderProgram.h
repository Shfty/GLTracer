#ifndef SHADERPROGRAM_H
#define SHADERPROGRAM_H

#include <vector>
#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

static const std::string STR_BOOL = "bool";
static const std::string STR_INT = "int";
static const std::string STR_FLOAT = "float";
static const std::string STR_TRUE = "true";
static const std::string STR_FALSE = "false";

class ShaderProgram
{
public:
    ShaderProgram( const std::string& path, const GLenum shaderType );
    ~ShaderProgram();

    const GLuint GetID() const { return m_shaderID; };

    void Compile( const std::vector< std::vector< std::string > >* constants = 0 );

private:
    GLuint m_shaderID = 0;
    GLenum m_shaderType;
    std::string m_shaderString;
};

#endif // SHADERPROGRAM_H
