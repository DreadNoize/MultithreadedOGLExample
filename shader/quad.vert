#version 150
#extension GL_ARB_explicit_attrib_location : require

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 tex_position;

uniform mat4 ModelMatrix;

out vec2 pass_tex_coords;

void main() {
  gl_Position = ModelMatrix * vec4(in_position, 1.0); 
  pass_tex_coords = tex_position;
}