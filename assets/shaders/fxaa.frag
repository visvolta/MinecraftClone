#version 330 core

in vec2 vUv;
out vec4 fragmentColour;

uniform sampler2D sourceTexture;
uniform vec2 inverseResolution;

float luma(vec3 colour)
{
    return dot(colour, vec3(0.299, 0.587, 0.114));
}

void main()
{
    vec3 centre = texture(sourceTexture, vUv).rgb;
    float northWest = luma(texture(
        sourceTexture, vUv + vec2(-1.0, 1.0) * inverseResolution).rgb);
    float northEast = luma(texture(
        sourceTexture, vUv + vec2(1.0, 1.0) * inverseResolution).rgb);
    float southWest = luma(texture(
        sourceTexture, vUv + vec2(-1.0, -1.0) * inverseResolution).rgb);
    float southEast = luma(texture(
        sourceTexture, vUv + vec2(1.0, -1.0) * inverseResolution).rgb);
    float centreLuma = luma(centre);

    float minimumLuma = min(
        centreLuma,
        min(min(northWest, northEast), min(southWest, southEast))
    );
    float maximumLuma = max(
        centreLuma,
        max(max(northWest, northEast), max(southWest, southEast))
    );

    vec2 direction;
    direction.x = -((northWest + northEast) - (southWest + southEast));
    direction.y = (northWest + southWest) - (northEast + southEast);
    float reduction = max(
        (northWest + northEast + southWest + southEast) * 0.03125,
        0.0078125
    );
    float reciprocalMinimum = 1.0 /
        (min(abs(direction.x), abs(direction.y)) + reduction);
    direction = clamp(
        direction * reciprocalMinimum,
        vec2(-8.0),
        vec2(8.0)
    ) * inverseResolution;

    vec3 resultA = 0.5 * (
        texture(sourceTexture, vUv + direction * (1.0 / 3.0 - 0.5)).rgb +
        texture(sourceTexture, vUv + direction * (2.0 / 3.0 - 0.5)).rgb
    );
    vec3 resultB = resultA * 0.5 + 0.25 * (
        texture(sourceTexture, vUv + direction * -0.5).rgb +
        texture(sourceTexture, vUv + direction * 0.5).rgb
    );
    float resultLuma = luma(resultB);
    vec3 result = resultLuma < minimumLuma || resultLuma > maximumLuma
        ? resultA
        : resultB;
    fragmentColour = vec4(result, 1.0);
}
