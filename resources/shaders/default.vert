#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

out vec3 world_pos;
out vec3 world_norm;

uniform mat4 model_mat;
uniform mat4 view_mat;
uniform mat4 proj_mat;

void main() {
    world_pos = vec3(model_mat * vec4(position, 1.0));
    world_norm = normalize(mat3(inverse(transpose(model_mat))) * normal);

    mat4 mvp = proj_mat * view_mat * model_mat;
    gl_Position = mvp * vec4(position, 1.0);
}


