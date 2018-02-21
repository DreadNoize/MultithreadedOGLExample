#version 150
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_separate_shader_objects : enable

in vec2 pass_uvs;

out vec4 color;

uniform sampler2D tex_sampler;

void main() {
  color = texture(tex_sampler, pass_uvs);
  // color = vec4(1.0,0,0,1.0);
}
