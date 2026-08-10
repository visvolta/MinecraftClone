#version 330 core
layout(location=0) in vec3 aPosition;
layout(location=1) in vec2 aTextureCoordinate;
layout(location=2) in vec3 aBiomeTint;
layout(location=3) in vec2 aOverlayTextureCoordinate;
layout(location=4) in vec3 aOverlayTint;
layout(location=5) in float aHasOverlay;
layout(location=6) in float aIsLeaf;
layout(location=7) in float aMaterialTexture;

out vec2 vUv;
out vec3 vTint;
out vec2 vOverlayUv;
out vec3 vOverlayTint;
flat out int vHasOverlay;
flat out int vIsLeaf;
flat out int vMaterialTexture;
out float vFogDistance;

uniform mat4 model, view, projection;

void main()
{
    vec4 viewPosition = view * model * vec4(aPosition,1.0);
    gl_Position = projection * viewPosition;
    vFogDistance = length(viewPosition.xyz);
    vUv=aTextureCoordinate; vTint=aBiomeTint;
    vOverlayUv=aOverlayTextureCoordinate; vOverlayTint=aOverlayTint;
    vHasOverlay=aHasOverlay>0.5?1:0;
    vIsLeaf=aIsLeaf>0.5?1:0;
    vMaterialTexture=int(aMaterialTexture+0.5);
}
