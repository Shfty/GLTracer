#version 140

out vec4 color;
in vec2 ScreenCoord;

uniform sampler2D ScreenTextureSampler;

void main()
{
    color = texture2D( ScreenTextureSampler, ScreenCoord );
}
