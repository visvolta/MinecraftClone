#version 330 core

in vec2 vUv;
in vec3 vColour;
in float vFogDistance;

out vec4 fragmentColour;

uniform sampler2D atlasTexture;
uniform float daylightBrightness;
uniform int fogMode;
uniform vec3 fogColour;
uniform float fogStart;
uniform float fogEnd;
uniform float fogDensity;

void main()
{
    vec4 texel = texture(atlasTexture, vUv);

    // Beta uses cutout rendering for dropped block items
    if (texel.a < 0.1)
        discard;

    vec3 colour = texel.rgb * vColour * daylightBrightness;
    float visibility = fogMode == 1
        ? exp(-fogDensity * vFogDistance)
        : clamp((fogEnd - vFogDistance) / (fogEnd - fogStart), 0.0, 1.0);
    fragmentColour = vec4(mix(fogColour, colour, visibility), texel.a);
}
