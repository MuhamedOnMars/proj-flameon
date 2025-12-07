#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 offset;
layout (location = 2) in vec3 color;
out vec3 col;

uniform mat4 model_mat;
uniform mat4 view_mat;
uniform mat4 proj_mat;

vec3 center = vec3(0,0,0);
vec2 size = vec2(1,1);

void main() {
   vec3 u = vec3(view_mat[0][0], view_mat[1][0], view_mat[2][0]); //right
   vec3 v = vec3(view_mat[0][1], view_mat[1][1], view_mat[2][1]); //up

   vec3 particle_pos = position+offset;

   vec3 world_space_pos = center + u*particle_pos.x*size.x + v*particle_pos.y*size.y;
   mat4 mvp = proj_mat * view_mat * model_mat;
   gl_Position = mvp * vec4(world_space_pos, 1.0);
   col = color;
}
