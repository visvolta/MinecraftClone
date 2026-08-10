#version 330 core
in vec2 vUv;
in vec3 vTint;
in vec2 vOverlayUv;
in vec3 vOverlayTint;
flat in int vHasOverlay;
flat in int vIsLeaf;
flat in int vMaterialTexture;
in float vFogDistance;
out vec4 fragmentColour;

uniform sampler2D blockTexture;
uniform sampler2DArray waterStillTexture;
uniform sampler2DArray waterFlowTexture;
uniform sampler2DArray lavaStillTexture;
uniform sampler2DArray lavaFlowTexture;
uniform int waterFrame;
uniform int waterFlowFrame;
uniform int lavaFrame;
uniform int lavaFlowFrame;
uniform bool fastLeaves;
uniform float daylightBrightness;
uniform int fogMode;
uniform vec3 fogColour;
uniform float fogStart;
uniform float fogEnd;
uniform float fogDensity;

void main()
{
    const int MATERIAL_ATLAS = 0;
    const int MATERIAL_WATER_STILL = 1;
    const int MATERIAL_WATER_FLOW = 2;
    const int MATERIAL_LAVA_STILL = 3;
    const int MATERIAL_LAVA_FLOW = 4;

    vec4 texel;
    if (vMaterialTexture == MATERIAL_WATER_STILL)
        texel = texture(waterStillTexture, vec3(vUv, float(waterFrame)));
    else if (vMaterialTexture == MATERIAL_WATER_FLOW)
        texel = texture(waterFlowTexture, vec3(vUv, float(waterFlowFrame)));
    else if (vMaterialTexture == MATERIAL_LAVA_STILL)
        texel = texture(lavaStillTexture, vec3(vUv, float(lavaFrame)));
    else if (vMaterialTexture == MATERIAL_LAVA_FLOW)
        texel = texture(lavaFlowTexture, vec3(vUv, float(lavaFlowFrame)));
    else
        texel = texture(blockTexture, vUv);

    vec3 colour = texel.rgb * vTint;
    float alpha = texel.a;

    if (vHasOverlay == 1)
    {
        vec4 o = texture(blockTexture, vOverlayUv);
        colour = mix(colour, o.rgb * vOverlayTint, o.a);
        alpha = max(alpha, o.a);
    }

    if (vMaterialTexture == MATERIAL_ATLAS ||
        vMaterialTexture == MATERIAL_WATER_STILL ||
        vMaterialTexture == MATERIAL_WATER_FLOW)
        colour *= daylightBrightness;

    if (vIsLeaf == 1 && fastLeaves)
    {
        // Beta fast graphics uses an opaque leaf volume. Fill transparent
        // texels from the already lit biome tint instead of painting them
        // black, which caused flickering black specks under TAA.
        if (alpha < 0.1)
            colour = vTint * daylightBrightness * 0.55;
        alpha = 1.0;
    }
    if (alpha < 0.1) discard;
    float visibility = fogMode == 1
        ? exp(-fogDensity * vFogDistance)
        : clamp((fogEnd - vFogDistance) / (fogEnd - fogStart), 0.0, 1.0);
    fragmentColour = vec4(mix(fogColour, colour, visibility), alpha);
}
