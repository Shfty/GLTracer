// #version def and CPU-exposed constants are appended in ShaderProgram.cpp

in vec3 vertex;
in vec2 texCoord;

out vec2 ScreenCoord;

void main(void) {
        gl_Position = vec4(vertex, 1.0);
        ScreenCoord = texCoord;
}
