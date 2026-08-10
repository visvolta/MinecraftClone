#version 330 core
in vec2 vUv;
out vec4 fragmentColour;
uniform sampler2D damageTexture;
void main()
{
    vec4 texel = texture(damageTexture, vUv);
    if (texel.a < 0.05) discard;
    fragmentColour = texel;
}
