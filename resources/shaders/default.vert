#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

out vec3 world_pos;
out vec3 world_norm;

uniform mat4 model_mat;
uniform mat4 view_mat;
uniform mat4 proj_mat;

//instance rendering
layout (location = 2) in vec4 instance_col0;
layout (location = 3) in vec4 instance_col1;
layout (location = 4) in vec4 instance_col2;
layout (location = 5) in vec4 instance_col3;
uniform int useInstancing; // 0 = no instancing, 1 = use instance_mat

void main() {
    mat4 M = model_mat;

    if(useInstancing == 1){
        mat4 instance_mat = mat4(instance_col0,
                                instance_col1,
                                instance_col2,
                                instance_col3);
        M = M * instance_mat;
    }

    // world_pos = vec3(model_mat * vec4(position, 1.0));
    // world_norm = normalize(mat3(inverse(transpose(model_mat))) * normal);

    // mat4 mvp = proj_mat * view_mat * model_mat;
    // gl_Position = mvp * vec4(position, 1.0);

    world_pos = vec3(M * vec4(position, 1.0));
    world_norm = normalize(mat3(inverse(transpose(M))) * normal);

    mat4 mvp = proj_mat * view_mat * M;
    gl_Position = mvp * vec4(position, 1.0);
}


