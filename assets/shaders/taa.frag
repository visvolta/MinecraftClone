#version 330 core

in vec2 vUv;
out vec4 fragmentColour;

uniform sampler2D currentColour;
uniform sampler2D depthTexture;
uniform sampler2D historyTexture;
uniform vec2 inverseResolution;
uniform mat4 inverseCurrentViewProjection;
uniform mat4 previousViewProjection;
uniform bool historyValid;

void main()
{
    vec3 current = texture(currentColour, vUv).rgb;
    if (!historyValid)
    {
        fragmentColour = vec4(current, 1.0);
        return;
    }

    float depth = texture(depthTexture, vUv).r;
    vec4 clipPosition = vec4(vUv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 worldPosition = inverseCurrentViewProjection * clipPosition;
    worldPosition /= max(abs(worldPosition.w), 0.00001);
    vec4 previousClip = previousViewProjection * worldPosition;
    vec2 previousUv = previousClip.xy /
        max(abs(previousClip.w), 0.00001) * 0.5 + 0.5;

    if (previousClip.w <= 0.0 ||
        any(lessThan(previousUv, vec2(0.0))) ||
        any(greaterThan(previousUv, vec2(1.0))))
    {
        fragmentColour = vec4(current, 1.0);
        return;
    }

    vec3 neighbourhoodMinimum = current;
    vec3 neighbourhoodMaximum = current;
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            vec3 sampleColour = texture(
                currentColour,
                vUv + vec2(x, y) * inverseResolution
            ).rgb;
            neighbourhoodMinimum = min(neighbourhoodMinimum, sampleColour);
            neighbourhoodMaximum = max(neighbourhoodMaximum, sampleColour);
        }
    }

    vec3 history = texture(historyTexture, previousUv).rgb;
    history = clamp(history, neighbourhoodMinimum, neighbourhoodMaximum);
    float motion = length(previousUv - vUv);
    float historyWeight = mix(
        0.90,
        0.15,
        clamp(motion * 120.0, 0.0, 1.0)
    );

    // Reprojection only accounts for camera motion. Animated fluids, alpha-
    // tested leaves, and rotating dropped items have no velocity buffer, so
    // reject history when its colour clearly belongs to older geometry.
    float colourChange = max(
        max(abs(history.r - current.r), abs(history.g - current.g)),
        abs(history.b - current.b)
    );
    float reactive = smoothstep(0.04, 0.30, colourChange);
    historyWeight *= 1.0 - reactive;
    fragmentColour = vec4(mix(current, history, historyWeight), 1.0);
}
