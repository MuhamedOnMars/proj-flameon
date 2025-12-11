#version 330 core

// in vec4 world_pos;
// in vec4 world_normal;
// in vec2 obj_uv;
// in vec2 sampleLoc;

// const float EPSILON = 1e-6;


// out vec4 fragColor;

// uniform vec2 wh;
// uniform sampler2D depthSampler;
// uniform sampler2D colorSampler;

// uniform float aperture;
// uniform float focalPlane;
// uniform float focalLength;
// uniform float nearPlane;
// uniform float farPlane;
// const int MAX_LEVEL = 3;

// void main()
// {
//     float z = texture(depthSampler, sampleLoc).r;

//     float objectDistance = -farPlane * nearPlane / (z * (farPlane - nearPlane) - farPlane);

//     // Task 17: Set fragColor using the sampler2D at the UV coordinate

//     // if foc plane = obj dist, full clarity

//     float scale = 1.f;
//     // 0.01 recommended, but i'll need to play with it more....

//     float denom = max(abs(objectDistance * (focalPlane - focalLength)), EPSILON);

//     float coc = (scale * aperture * focalLength * abs(objectDistance - focalPlane)) / denom;
//     coc = clamp(coc, 0.0, 1.0);


//     vec4 textureSample = textureLod(colorSampler, sampleLoc, coc * MAX_LEVEL)
//             //* (1-texture(depthSampler, sampleLoc).r)
//             ;

//     vec4 baseColor   = texture(colorSampler, sampleLoc);

//     float blur = min(max(coc / 1.f, 0.0), 1.0);


//     fragColor = mix(baseColor, textureSample, blur);


//     // if (focalPlane - 0.01 < objectDistance && focalPlane + 0.01 > objectDistance){
//     //     fragColor = vec4(0.f, 1.f, 0.f, 1.f);
//     // } else {
//     //     fragColor = mix(baseColor, textureSample, blur);
//     // }




//     // vec4 idk = world_pos - world_normal;
//     // vec2 idk_2 = obj_uv;
//     // Task 33: Invert fragColor's r, g, and b color channels if your bool is true
// }

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
const int MAX_LEVEL = 5;

void main()
{
    float z = texture(depthSampler, uv.xy).r;

    float ndc = z * 2.0 - 1.0; // back to NDC

    float objectDistance =
        (2.0 * nearPlane * farPlane) /
        (farPlane + nearPlane - ndc * (farPlane - nearPlane));
    // Task 17: Set fragColor using the sampler2D at the UV coordinate

    // if foc plane = obj dist, full clarity

    float scale = 0.01f;
    // 0.01 recommended, but i'll need to play with it more....

    float denom = max(abs(objectDistance * (focalPlane - focalLength)), EPSILON);

    float coc = scale * aperture * abs(objectDistance - focalPlane) / focalPlane;
    coc = clamp(coc, 0.0, 1.0);


    vec4 textureSample = textureLod(colorSampler, uv.xy, MAX_LEVEL)
            //* (1-texture(depthSampler, sampleLoc).r)
            ;

    vec4 baseColor   = texture(colorSampler, uv.xy);

    float blur = min(max(coc / 1.f, 0.0), 1.0);


    fragColor = textureSample;


    if (focalPlane - 0.01 < objectDistance && focalPlane + 0.01 > objectDistance){
        fragColor = vec4(0.f, 1.f, 0.f, 1.f);
    } else {
        fragColor = //fragColor = vec4(vec3(coc), 1.0);
                mix(baseColor, textureSample, blur);
    }




    // vec4 idk = world_pos - world_normal;
    // vec2 idk_2 = obj_uv;
    // Task 33: Invert fragColor's r, g, and b color channels if your bool is true
}
