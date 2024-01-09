#include "ShaderProgram.h"

#include <string>
#include <fstream>
#include <iostream>

ShaderProgram::ShaderProgram( const std::string& path, const GLenum shaderType )
{
    std::string shaderString;
    std::ifstream fileStream( path.c_str() );

    if( fileStream )
    {
        shaderString.assign( ( std::istreambuf_iterator< char >( fileStream ) ), std::istreambuf_iterator< char >() );

        m_shaderID = glCreateShader( shaderType );

        const GLchar* shaderSource = shaderString.c_str();
        glShaderSource( m_shaderID, 1, &shaderSource, NULL );

        glCompileShader( m_shaderID );

        GLint shaderCompiled = GL_FALSE;
        glGetShaderiv( m_shaderID, GL_COMPILE_STATUS, &shaderCompiled );
        std::cout << "Shader " << m_shaderID << " compilation status: " << (shaderCompiled ? "GL_TRUE" : "GL_FALSE") << std::endl;

        GLchar infoLog[ GL_INFO_LOG_LENGTH ] = {0};
        glGetShaderInfoLog( m_shaderID, GL_INFO_LOG_LENGTH, NULL, infoLog );
        std::cout << std::endl << "Shader info log:" << std::endl << infoLog << std::endl;
    }
    else
    {
        std::cerr << "Could not open file: " << path << std::endl;
    }
}

ShaderProgram::~ShaderProgram()
{
    glDeleteShader( m_shaderID );
}
