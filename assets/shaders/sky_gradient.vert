#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in float aAltitude;

out float vAltitude;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    vAltitude = aAltitude;
    gl_Position = projection * view * vec4(aPosition, 1.0);
}
