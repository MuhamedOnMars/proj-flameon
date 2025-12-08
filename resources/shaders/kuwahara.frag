#version 330 core

in vec3 uv;
out vec4 fragColor;

uniform sampler2D u_tex;
uniform vec2 u_texelSize;
uniform bool bloom;

const int R = 6;

float luminance(vec3 c) {
    return dot(c, vec3(0.299, 0.587, 0.114));
}

void main() {
    vec3 meanColor[4];
    float meanLum[4];
    float meanLum2[4];

    for (int k = 0; k < 4; ++k) {
        meanColor[k] = vec3(0.0);
        meanLum[k]   = 0.0;
        meanLum2[k]  = 0.0;
    }

    float n = float((R + 1) * (R + 1));

    // 0: top-left
    for (int j = -R; j <= 0; ++j) {
        for (int i = -R; i <= 0; ++i) {
            vec2 offset = vec2(i, j) * u_texelSize;
            vec3 c = texture(u_tex, uv.xy + offset).rgb;
            float L = luminance(c);
            meanColor[0] += c;
            meanLum[0]   += L;
            meanLum2[0]  += L * L;
        }
    }

    // 1: top-right
    for (int j = -R; j <= 0; ++j) {
        for (int i = 0; i <= R; ++i) {
            vec2 offset = vec2(i, j) * u_texelSize;
            vec3 c = texture(u_tex, uv.xy + offset).rgb;
            float L = luminance(c);
            meanColor[1] += c;
            meanLum[1]   += L;
            meanLum2[1]  += L * L;
        }
    }

    // 2: bottom-left
    for (int j = 0; j <= R; ++j) {
        for (int i = -R; i <= 0; ++i) {
            vec2 offset = vec2(i, j) * u_texelSize;
            vec3 c = texture(u_tex, uv.xy + offset).rgb;
            float L = luminance(c);
            meanColor[2] += c;
            meanLum[2]   += L;
            meanLum2[2]  += L * L;
        }
    }

    // 3: bottom-right
    for (int j = 0; j <= R; ++j) {
        for (int i = 0; i <= R; ++i) {
            vec2 offset = vec2(i, j) * u_texelSize;
            vec3 c = texture(u_tex, uv.xy + offset).rgb;
            float L = luminance(c);
            meanColor[3] += c;
            meanLum[3]   += L;
            meanLum2[3]  += L * L;
        }
    }

    float minVar = 1e20;
    vec3 bestColor = texture(u_tex, uv.xy).rgb;

    for (int k = 0; k < 4; ++k) {
        vec3 mc  = meanColor[k] / n;
        float mL  = meanLum[k]  / n;
        float mL2 = meanLum2[k] / n;
        float varL = mL2 - mL * mL;

        if (varL < minVar) {
            minVar = varL;
            bestColor = mc;
        }
    }

    vec3 original = texture(u_tex, uv.xy).rgb;
    vec3 finalColor = bloom ? bestColor : original;
    fragColor = vec4(finalColor, 1.0);
}
