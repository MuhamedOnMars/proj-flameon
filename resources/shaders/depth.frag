#version 330 core

in vec3 uv;
const float EPSILON = 1e-6;


out vec4 fragColor;

uniform vec2 wh;
uniform sampler2D depthSampler;
uniform sampler2D colorSampler;

uniform float aperture;
uniform float focalPlane;
uniform float focalLength;
uniform float nearPlane;
uniform float farPlane;
const int MAX_LEVEL = 3;

void main()
{
    float z = texture(depthSampler, uv.xy).r;

    float objectDistance = -farPlane * nearPlane / (z * (farPlane - nearPlane) - farPlane);

    float scale = 1.f;
    // 0.01 recommended, but i'll need to play with it more....

    float denom = max(abs(objectDistance * (focalPlane - focalLength)), EPSILON);

    float coc = (scale * aperture * focalLength * abs(objectDistance - focalPlane)) / denom;
    coc = clamp(coc, 0.0, 1.0);


    vec4 textureSample = textureLod(colorSampler, uv.xy, coc * MAX_LEVEL)
            //* (1-texture(depthSampler, sampleLoc).r)
            ;

    vec4 baseColor   = texture(colorSampler, uv.xy);

    float blur = min(max(coc / 1.f, 0.0), 1.0);


    fragColor = mix(baseColor, textureSample, blur);

}
