#version 150

in vec2 pass_color;

out vec4 out_color;

void main() {
  //out_color = vec4(1.0,0.0,0.0,1.0);
  out_color = vec4(pass_color.y,pass_color.x, pass_color.y, pass_color.x);
}