#version 330 core

in vec2 vUv;
out vec4 fragmentColour;

uniform sampler2D sourceTexture;
uniform vec2 inverseResolution;

float luma(vec3 colour)
{
    return dot(colour, vec3(0.2126, 0.7152, 0.0722));
}

void main()
{
    vec3 centre = texture(sourceTexture, vUv).rgb;
    float centreLuma = luma(centre);
    float left = luma(texture(
        sourceTexture, vUv - vec2(inverseResolution.x, 0.0)).rgb);
    float right = luma(texture(
        sourceTexture, vUv + vec2(inverseResolution.x, 0.0)).rgb);
    float down = luma(texture(
        sourceTexture, vUv - vec2(0.0, inverseResolution.y)).rgb);
    float up = luma(texture(
        sourceTexture, vUv + vec2(0.0, inverseResolution.y)).rgb);

    float verticalEdge = max(abs(centreLuma - left), abs(centreLuma - right));
    float horizontalEdge = max(abs(centreLuma - down), abs(centreLuma - up));
    float strongestEdge = max(verticalEdge, horizontalEdge);
    if (strongestEdge < 0.045)
    {
        fragmentColour = vec4(centre, 1.0);
        return;
    }

    vec2 normal = verticalEdge > horizontalEdge
        ? vec2(inverseResolution.x, 0.0)
        : vec2(0.0, inverseResolution.y);
    vec3 nearBlend =
        texture(sourceTexture, vUv - normal * 0.5).rgb +
        texture(sourceTexture, vUv + normal * 0.5).rgb;
    vec3 farBlend =
        texture(sourceTexture, vUv - normal * 1.5).rgb +
        texture(sourceTexture, vUv + normal * 1.5).rgb;
    float weight = clamp((strongestEdge - 0.045) * 5.0, 0.0, 0.75);
    vec3 morphology = nearBlend * 0.4 + farBlend * 0.1;
    fragmentColour = vec4(mix(centre, morphology, weight), 1.0);
}
