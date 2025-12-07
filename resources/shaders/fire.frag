#version 330 core

in vec3 col;
out vec4 fragColor;
out vec4 brightColor;

void main() {
   float alpha = 0.9f;
   fragColor = vec4(col, alpha);
   brightColor = vec4(col, alpha);
}
