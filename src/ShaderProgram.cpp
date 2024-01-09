#include "ShaderProgram.h"

#include <fstream>
#include <iostream>
#include <sstream>

const std::string GLSL_VERSION_STR = "#version 140";

ShaderProgram::ShaderProgram( const std::string& path, const GLenum shaderType )
{
    std::ifstream fileStream( path.c_str() );
    m_shaderType = shaderType;

    if( fileStream )
    {
        m_shaderString.assign( std::istreambuf_iterator< char >( fileStream ), std::istreambuf_iterator< char >() );
    }
    else
    {
        std::cerr << "Could not open file: " << path << std::endl;
    }
}

ShaderProgram::~ShaderProgram()
{
    if( m_shaderID != 0 )
    {
        glDeleteShader( m_shaderID );
    }
}

// Prepend the strings stored in constants to the shader source
// and attempt to compile it
void ShaderProgram::Compile( const std::vector< std::vector< std::string > >* constants )
{
    if( m_shaderID != 0 )
    {
        glDeleteShader( m_shaderID );
    }

    std::stringstream finalShader;
    finalShader << GLSL_VERSION_STR << std::endl;

    if( constants != NULL )
    {
        for( int i = 0; i < constants->size(); ++i )
        {
            std::vector< std::string > c = ( *constants )[ i ];
            finalShader << "const " << c[ 0 ] << " " << c[ 1 ] << " = " << c[ 2 ] << ";" << std::endl;
        }
    }

    finalShader << m_shaderString;
    std::cout << finalShader.str();

    const GLchar* shaderSource = finalShader.str().c_str();

    m_shaderID = glCreateShader( m_shaderType );
    glShaderSource( m_shaderID, 1, &shaderSource, NULL );

    glCompileShader( m_shaderID );

    GLint shaderCompiled = GL_FALSE;
    glGetShaderiv( m_shaderID, GL_COMPILE_STATUS, &shaderCompiled );
    std::cout << "Shader " << m_shaderID << " compilation status: " << (shaderCompiled ? "GL_TRUE" : "GL_FALSE") << std::endl;

    GLchar infoLog[ GL_INFO_LOG_LENGTH ] = {0};
    glGetShaderInfoLog( m_shaderID, GL_INFO_LOG_LENGTH, NULL, infoLog );
    std::cout << std::endl << "Shader info log:" << std::endl << infoLog << std::endl;
}
