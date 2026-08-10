#version 330 core

in vec2 vUv;
out vec4 fragmentColour;

uniform sampler2D sourceTexture;

void main()
{
    fragmentColour = texture(sourceTexture, vUv);
}
