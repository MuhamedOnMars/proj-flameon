#version 330 core
in vec3 uv;

out vec4 fragColor;

uniform sampler2D scene;
uniform sampler2D blur;
uniform bool bloom;
uniform float exposure;

void main()
{
    vec3 sceneColor = texture(scene, vec2(uv.x, uv.y)).rgb;
    vec3 blurColor = texture(blur, vec2(uv.x, uv.y)).rgb;
    if(bloom) {
        // Add blur to scene
        sceneColor += blurColor;
    }

    vec3 tone_mapped = vec3(1.0) - exp(-sceneColor * exposure);
    fragColor = vec4(tone_mapped, 1.0);
}
