#include "GLError.h"

#include <GL/gl.h>

void handle_error() {
    bool error = false;
    while (true) {
        GLenum gl_error = glGetError();
        if (gl_error == GL_NO_ERROR) {
            break;
        }

        error = true;

        if (gl_error == GL_INVALID_ENUM) {
            std::cout << "GL Error: Invalid Enum" << std::endl;
        }

        if (gl_error == GL_INVALID_VALUE) {
            std::cout << "GL Error: Invalid Value" << std::endl;
        }

        if (gl_error == GL_INVALID_OPERATION) {
            std::cout << "GL Error: Invalid Operation" << std::endl;
        }

        if (gl_error == GL_STACK_OVERFLOW) {
            std::cout << "GL Error: Stack Overflow" << std::endl;
        }

        if (gl_error & GL_STACK_UNDERFLOW) {
            std::cout << "GL Error: Stack Underflow" << std::endl;
        }

        if (gl_error == GL_OUT_OF_MEMORY) {
            std::cout << "GL Error: Out of memory" << std::endl;
        }
    }

    if (error) {
        throw "Error";
    }
}

