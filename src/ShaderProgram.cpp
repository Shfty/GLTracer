#include "ShaderProgram.h"
#include "GLError.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <cstring>

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
    // Delete shader if it already exists
    if( m_shaderID != 0 )
    {
        glDeleteShader( m_shaderID );
    }

    // Create a string stream to assemble the final shader
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

    // Copy into a char array to pass to GL
    const int length = finalShader.str().length(); 
  
    char* char_array = new char[length + 1]; 
  
    strcpy(char_array, finalShader.str().c_str()); 
    
    // Pass to GL and compile
    m_shaderID = glCreateShader( m_shaderType );

    std::cout << "Shader " << m_shaderID << std::endl;


    glShaderSource( m_shaderID, 1, &char_array, NULL );
    glCompileShader( m_shaderID );

    delete[] char_array; 

    // Fetch compilation result, print on error
    GLint shaderCompiled = GL_FALSE;
    glGetShaderiv( m_shaderID, GL_COMPILE_STATUS, &shaderCompiled );

    if (!shaderCompiled) {
        std::cout << "Final Shader:\n" << finalShader.str() << "\n\n";
    }

    std::cout << "Compilation status: " << (shaderCompiled ? "GL_TRUE" : "GL_FALSE") << std::endl;

    if (!shaderCompiled) {
        GLchar infoLog[ GL_INFO_LOG_LENGTH ] = {0};
        glGetShaderInfoLog( m_shaderID, GL_INFO_LOG_LENGTH, NULL, infoLog );
        std::cout << std::endl << "Shader info log:" << std::endl << infoLog << std::endl;
    }
}
