#version 330 core

in vec2 vUv;
out vec4 fragmentColour;

uniform sampler2D celestialTexture;
uniform float alpha;

void main()
{
    vec4 texel = texture(celestialTexture, vUv);
    fragmentColour = vec4(texel.rgb, texel.a * alpha);
}
