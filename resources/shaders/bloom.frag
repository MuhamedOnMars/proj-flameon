#version 330 core
in vec3 uv;

out vec4 fragColor;

uniform sampler2D scene;
uniform sampler2D blur;
uniform sampler3D LUT;
uniform bool bloom;
uniform bool graded;
uniform float exposure;

//Kuwahara shader
uniform bool kuwaharaOn;
uniform vec2 u_texelSize;   // 1.0 / vec2(width, height)
const int R = 6; //Original filter constant

// across-edge radius / Edge preservation (smaller = sharper, larger = blurrier)
float Rn = 5;
// along-edge radius  / Brushstroke length
float Rt = 10;
// max radius for loop bounds / Sampling
const int Rmax = 16;   // >= max(Rn, Rt)

float luminance(vec3 c) {
    return dot(c, vec3(0.299, 0.587, 0.114));
}

// vec3 kuwaharaFilter(sampler2D tex, vec2 uvCoord, vec2 texelSize) {
//     vec3 meanColor[4];
//     float meanLum[4];
//     float meanLum2[4];

//     for (int k = 0; k < 4; ++k) {
//         meanColor[k] = vec3(0.0);
//         meanLum[k]   = 0.0;
//         meanLum2[k]  = 0.0;
//     }

//     float n = float((R + 1) * (R + 1));

//     // 0: top-left
//     for (int j = -R; j <= 0; ++j) {
//         for (int i = -R; i <= 0; ++i) {
//             vec2 offset = vec2(i, j) * texelSize;
//             vec3 c = texture(tex, uvCoord + offset).rgb;
//             float L = luminance(c);
//             meanColor[0] += c;
//             meanLum[0]   += L;
//             meanLum2[0]  += L * L;
//         }
//     }

//     // 1: top-right
//     for (int j = -R; j <= 0; ++j) {
//         for (int i = 0; i <= R; ++i) {
//             vec2 offset = vec2(i, j) * texelSize;
//             vec3 c = texture(tex, uvCoord + offset).rgb;
//             float L = luminance(c);
//             meanColor[1] += c;
//             meanLum[1]   += L;
//             meanLum2[1]  += L * L;
//         }
//     }

//     // 2: bottom-left
//     for (int j = 0; j <= R; ++j) {
//         for (int i = -R; i <= 0; ++i) {
//             vec2 offset = vec2(i, j) * texelSize;
//             vec3 c = texture(tex, uvCoord + offset).rgb;
//             float L = luminance(c);
//             meanColor[2] += c;
//             meanLum[2]   += L;
//             meanLum2[2]  += L * L;
//         }
//     }

//     // 3: bottom-right
//     for (int j = 0; j <= R; ++j) {
//         for (int i = 0; i <= R; ++i) {
//             vec2 offset = vec2(i, j) * texelSize;
//             vec3 c = texture(tex, uvCoord + offset).rgb;
//             float L = luminance(c);
//             meanColor[3] += c;
//             meanLum[3]   += L;
//             meanLum2[3]  += L * L;
//         }
//     }

//     float minVar = 1e20;
//     vec3 bestColor = texture(tex, uvCoord).rgb;

//     for (int k = 0; k < 4; ++k) {
//         vec3 mc  = meanColor[k] / n;
//         float mL  = meanLum[k]  / n;
//         float mL2 = meanLum2[k] / n;
//         float varL = mL2 - mL * mL;

//         if (varL < minVar) {
//             minVar = varL;
//             bestColor = mc;
//         }
//     }

//     return bestColor;
// }

vec3 kuwaharaFilter(sampler2D tex, vec2 uvCoord, vec2 texelSize) {
    vec2 texel = texelSize;
    vec3 centerColor = texture(tex, uvCoord).rgb;
    float Lc  = luminance(centerColor);
    float Lx1 = luminance(texture(tex, uvCoord + vec2(texel.x, 0.0)).rgb);
    float Lx0 = luminance(texture(tex, uvCoord - vec2(texel.x, 0.0)).rgb);
    float Ly1 = luminance(texture(tex, uvCoord + vec2(0.0, texel.y)).rgb);
    float Ly0 = luminance(texture(tex, uvCoord - vec2(0.0, texel.y)).rgb);

    float gx = 0.5 * (Lx1 - Lx0);
    float gy = 0.5 * (Ly1 - Ly0);
    vec2 g = vec2(gx, gy);

    // n = across edge, t = along edge
    vec2 n; // normal
    vec2 t; // tangent
    if (length(g) < 1e-4) {
        n = vec2(1.0, 0.0);
        t = vec2(0.0, 1.0);
    } else {
        n = normalize(g);
        t = vec2(-n.y, n.x); // perpendicular
    }

    vec3 meanColor[4];
    float meanLum[4];
    float meanLum2[4];
    float count[4];

    for (int k = 0; k < 4; ++k) {
        meanColor[k] = vec3(0.0);
        meanLum[k]   = 0.0;
        meanLum2[k]  = 0.0;
        count[k]     = 0.0;
    }

    for (int j = -Rmax; j <= Rmax; ++j) {
        for (int i = -Rmax; i <= Rmax; ++i) {
            vec2 p = vec2(i, j);

            float p_n = dot(p, n);
            float p_t = dot(p, t);

            // elliptical anisotropic kernel
            float e = (p_n * p_n) / (Rn * Rn) + (p_t * p_t) / (Rt * Rt);
            if (e > 1.0) {
                continue;
            }

            int idx;
            if (p_n <= 0.0) {
                idx = (p_t <= 0.0) ? 0 : 1;
            } else {
                idx = (p_t <= 0.0) ? 2 : 3;
            }

            vec2 offset = p * texel;
            vec3 c = texture(tex, uvCoord + offset).rgb;
            float L = luminance(c);

            meanColor[idx] += c;
            meanLum[idx]   += L;
            meanLum2[idx]  += L * L;
            count[idx]     += 1.0;
        }
    }

    // 4. Pick region with minimum variance
    float minVar = 1e20;
    vec3 bestColor = centerColor;

    for (int k = 0; k < 4; ++k) {
        if (count[k] < 1.0) continue;

        float nSamples = count[k];
        vec3 mc  = meanColor[k] / nSamples;
        float mL  = meanLum[k]  / nSamples;
        float mL2 = meanLum2[k] / nSamples;
        float varL = mL2 - mL * mL;

        if (varL < minVar) {
            minVar = varL;
            bestColor = mc;
        }
    }

    return bestColor;
}


void main()
{
    // vec3 sceneColor = texture(scene, vec2(uv.x, uv.y)).rgb;
    // vec3 blurColor = texture(blur, vec2(uv.x, uv.y)).rgb;
    // if(bloom) {
    //     // Add blur to scene
    //     sceneColor += blurColor;
    // }

    // vec3 toneMapped = vec3(1.0) - exp(-sceneColor * exposure);
    // fragColor = vec4(toneMapped, 1.0);

    // if (graded) {
    //     float lutSize = 64.0;
    //     float scale = (lutSize - 1.0) / lutSize;
    //     float offset = 1.0 / (2.0 * lutSize);

    //     vec3 newColor = texture(LUT, scale * toneMapped + offset).rgb;
    //     fragColor = vec4(newColor, 1.0);
    // }

    vec2 uv2 = uv.xy;

    // base scene
    vec3 sceneColor = texture(scene, uv2).rgb;

    // if (kuwaharaOn) {
    //     sceneColor = kuwaharaFilter(scene, uv2, u_texelSize);
    // }

    sceneColor = kuwaharaFilter(scene, uv2, u_texelSize);

    // bloom
    vec3 blurColor = texture(blur, uv2).rgb;
    if (bloom) {
        sceneColor += blurColor;
    }

    // tone mapping
    vec3 toneMapped = vec3(1.0) - exp(-sceneColor * exposure);
    vec3 finalColor = toneMapped;

    // grading
    if (graded) {
        float lutSize = 64.0;
        float scale = (lutSize - 1.0) / lutSize;
        float offset = 1.0 / (2.0 * lutSize);
        finalColor = texture(LUT, scale * toneMapped + offset).rgb;
    }

    fragColor = vec4(finalColor, 1.0);
}
