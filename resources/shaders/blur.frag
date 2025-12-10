#version 330 core

in vec3 uv;

out vec4 fragColor;

uniform sampler2D tex;
uniform bool horizontal;
// Possibly change to take more samples
// This is from Gaussian weights from the linked resource
uniform float weight[5] = float[] (0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162);

void main()
{
    fragColor = vec4(1);

    // Finds distance between pixels in texture space (1/texture_width & 1/texture_height where 1 = size in uv)
    vec2 tex_offset = 1.0 / textureSize(tex, 0);
    // Get starting pixel's contribution
    vec3 result = textureLod(tex, vec2(uv.x, uv.y), 0.0).xyz * weight[0];
    int loop_amt = 5;
    // Convolution --> sampling over pixels surrounding
    if(horizontal) {
        for (int i = 1; i < loop_amt; ++i) {
           result += textureLod(tex, vec2(uv.x, uv.y) + vec2(tex_offset.x * i, 0.0), 0.0).xyz * weight[i];
           result += textureLod(tex, vec2(uv.x, uv.y) - vec2(tex_offset.x * i, 0.0), 0.0).xyz * weight[i];
        }
    }
    else {
        for (int i = 1; i < loop_amt; ++i) {
            result += textureLod(tex, vec2(uv.x, uv.y) + vec2(0.0, tex_offset.y * i), 0.0).xyz * weight[i];
            result += textureLod(tex, vec2(uv.x, uv.y) - vec2(0.0, tex_offset.y * i), 0.0).xyz * weight[i];
        }
    }
    fragColor = vec4(result, 1.0);
}
