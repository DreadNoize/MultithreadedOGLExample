#version 330

in vec2 pass_uvs;
in float pass_depth;

uniform sampler2D tex_handle;

layout(location = 0) out vec4 out_color;
// layout(location = 1) out vec4 out_depth;

void main() {
  out_color = texture(tex_handle, pass_uvs);
  // out_color = vec4(1.0,0.0,0.0,1.0);
  // out_color = vec4(pass_uvs.x,pass_uvs.y, 0.0, 1.0);
  // out_depth =vec4(pass_depth, pass_depth, pass_depth, 1.0);
  gl_FragDepth = pass_depth;

}