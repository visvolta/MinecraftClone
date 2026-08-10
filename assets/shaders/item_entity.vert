#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aUv;
layout(location = 2) in vec3 aColour;

out vec2 vUv;
out vec3 vColour;
out float vFogDistance;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vUv = aUv;
    vColour = aColour;

    vec4 viewPosition = view * model * vec4(aPosition, 1.0);
    vFogDistance = length(viewPosition.xyz);
    gl_Position = projection * viewPosition;
}
