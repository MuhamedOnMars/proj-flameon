#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 offset;
layout (location = 2) in vec3 color;
out vec3 col;

uniform mat4 model_mat;
uniform mat4 view_mat;
uniform mat4 proj_mat;

void main() {
   mat4 mvp = proj_mat * view_mat * model_mat;
   gl_Position = mvp * vec4(position+offset, 1.0);
   col = color;
}
