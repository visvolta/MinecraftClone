#version 330 core

in float vOpacity;
out vec4 fragmentColour;

uniform vec4 colour;

void main()
{
    fragmentColour = vec4(colour.rgb, colour.a * vOpacity);
}
