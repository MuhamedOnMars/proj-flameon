#version 330 core

layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec4 brightColor;

in vec3 world_pos;
in vec3 world_norm;

uniform vec3 camera_pos;
uniform float ka;
uniform float kd;
uniform float ks;
uniform float shininess;
uniform vec4 ambient;
uniform vec4 diffuse;
uniform vec4 specular;

struct Light {
    int id;
    int type;
    vec4 color;
    vec3 function;
    vec4 pos;
    vec4 dir;
    float penumbra;
    float angle;
    float width, height;
};
uniform int light_size;
uniform Light lights[8];

uniform float max_dist;
uniform float min_dist;

//skydome
uniform bool u_isSky;
uniform vec3 u_skyTopColor;
uniform vec3 u_skyBottomColor;

void main() {
    vec3 norm = normalize(world_norm);
    fragColor = vec4(0.0, 0.0, 0.0, 1.0);

    if (u_isSky) {
            float t = clamp(norm.y * 0.5 + 0.5, 0.0, 1.0);
            vec3 col = mix(u_skyBottomColor, u_skyTopColor, t);
            fragColor = vec4(col, 1.0);
            return;
        }

    // Ambient
    vec3 illumination = ka * vec3(ambient);

    for (int i = 0; i < light_size; i++) {
        Light light = lights[i];
        float attenuation = 1.0;
        vec3 to_light;

        if (light.type == 1) {
            // Negate light direction (light to point) to get it to be point to light
            to_light = normalize(vec3(-light.dir));
        }
        else if (light.type == 0) {
            // No given direction vector for point lights
            to_light = normalize(vec3(light.pos) - world_pos);

            float distance = distance(world_pos, vec3(light.pos));
            attenuation = min(1, 1/(light.function.x + distance*light.function.y
                                        + distance*distance*light.function.z));
        } else {
            to_light = normalize(vec3(light.pos) - world_pos);
            vec3 light_dir = normalize(vec3(light.dir));

            // Angle between current direction & spotlight direction
           float x = acos(clamp(dot(light_dir, -to_light), -1.0, 1.0));

            float outer = light.angle;
            float inner = light.angle - light.penumbra;
            float falloff = 1.0f;
            if (x > inner && x <= outer) {
                float result = (x - inner) / (outer - inner);
                falloff = 1 - ((-2 * pow(result, 3)) + (3 * pow(result, 2)));
            } else if (x > outer){
                // If outside of spotlight should be completely dark
                falloff = 0.0f;
            }

            float distance = distance(world_pos, vec3(light.pos));
            attenuation = falloff * min(1, 1/(light.function.x + distance*light.function.y
                                            + distance*distance*light.function.z));
        }

        vec3 reflected_light = reflect(-to_light, norm);
        float closeness = clamp(dot(reflected_light, normalize(camera_pos - world_pos)), 0.0, 1.0);
        closeness = pow(closeness, shininess);

        // Make sure facing towards the source
        float facing_source = clamp(dot(norm, to_light), 0.0, 1.0);
        vec3 diffuse_term = vec3(0.0);
        vec3 specular_term = vec3(0.0);
        if (facing_source > 0.0) {
            specular_term = ks * vec3(specular) * closeness;
            diffuse_term = kd * vec3(diffuse) * facing_source;
        }

        illumination += attenuation * vec3(light.color) * (diffuse_term + specular_term);
    }

    vec3 output_color = illumination;
    // Fog
    if (max_dist > min_dist) {
        // Should be generally same as clear color
        vec3 fog_colour = vec3(0.0, 0.0, 0.0);
        float dist = length(camera_pos.xyz - world_pos.xyz);
        float fog_factor = (max_dist - dist) / (max_dist - min_dist);
        fog_factor = clamp(fog_factor, 0.0, 1.0);
        output_color = mix(fog_colour, illumination, fog_factor);
    }

    // Screen space bloom: determine if color is above threshold (transform to grayscale before comparison)
    float brightness = dot(output_color, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 1.0) { // HDR
        brightColor = vec4(output_color, 1.0);
    }
    else {
        brightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
    fragColor = vec4(output_color, 1.0);
}
