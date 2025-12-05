#version 330 core

in vec3 col;
out vec4 fragColor;
out vec4 brightColor;

void main() {
   fragColor = vec4(col, 1.0);
   brightColor = vec4(col, 1.f);
}
