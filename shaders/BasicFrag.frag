// #version def and CPU-exposed constants are appended in ShaderProgram.cpp

in vec2 ScreenCoord;
out vec4 color;

uniform sampler2D ScreenTextureSampler;

void main()
{
    color = texture2D( ScreenTextureSampler, ScreenCoord );
}
