#version 330
#extension GL_ARB_explicit_attrib_location : require

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uvs;

uniform mat4 ProjectionMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ModelMatrix;

uniform mat4 SecondView;

out vec2 pass_uvs;
out float pass_depth;

void main() {
  vec4 temp = ModelMatrix * vec4(in_position, 1.0); 
  gl_Position = (ProjectionMatrix * ViewMatrix * ModelMatrix) * vec4(in_position, 1.0); 
  pass_uvs = in_uvs;
  pass_depth = normalize(gl_Position).z;
}