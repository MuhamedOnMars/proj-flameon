#version 330 core

in vec3 col;
out vec4 fragColor;
out vec4 brightColor;

void main() {
   float alpha = 1.f;
   fragColor = vec4(col, 1);
   if (col.r < 0.9) {
      alpha = col.r;
   }
   brightColor = vec4(col, alpha);
}
