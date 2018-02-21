#version 150

in vec2 pass_tex_coords;

out vec4 out_color;

uniform sampler2D quad_tex;

void main() {
  //out_color = vec4(1.0,0.0,0.0,1.0);
  out_color = texture(quad_tex, pass_tex_coords);
}