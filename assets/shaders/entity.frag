#version 330 core
in vec2 vUv;
in vec3 vColour;
in float vDistance;
out vec4 fragmentColour;
uniform sampler2D entityTexture;
uniform vec3 entityTint;
uniform float daylightBrightness;
uniform int fogMode;
uniform vec3 fogColour;
uniform float fogStart;
uniform float fogEnd;
uniform float fogDensity;
void main()
{
    vec4 texel = texture(entityTexture, vUv);
    if (texel.a < 0.1) discard;
    vec3 colour = texel.rgb * entityTint * vColour * daylightBrightness;
    float fog = fogMode == 0
        ? clamp((fogEnd - vDistance) / max(0.001, fogEnd - fogStart), 0.0, 1.0)
        : clamp(exp(-fogDensity * fogDensity * vDistance * vDistance), 0.0, 1.0);
    fragmentColour = vec4(mix(fogColour, colour, fog), texel.a);
}
