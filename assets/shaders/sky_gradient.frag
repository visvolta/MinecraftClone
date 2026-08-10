#version 330 core

in float vAltitude;
out vec4 fragmentColour;

uniform vec3 skyColour;
uniform vec3 horizonColour;
uniform vec3 lowerSkyColour;

void main()
{
    vec3 colour;
    if (vAltitude >= 0.0)
    {
        float blend = smoothstep(0.0, 0.65, vAltitude);
        colour = mix(horizonColour, skyColour, blend);
    }
    else
    {
        float blend = smoothstep(0.0, 0.45, -vAltitude);
        colour = mix(horizonColour, lowerSkyColour, blend);
    }

    fragmentColour = vec4(colour, 1.0);
}
