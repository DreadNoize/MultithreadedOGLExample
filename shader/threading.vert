#version 150
#extension GL_ARB_explicit_attrib_location : require

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uvs;

// Uniforms
uniform mat4 ModelMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;
uniform mat4 NormalMatrix;
uniform mat4 TestMatrix;

//out vec3 pass_normal;
out vec2 pass_uvs;

void main() {
  gl_Position = (ProjectionMatrix  * ViewMatrix * ModelMatrix) * vec4(in_position, 1.0);
  // pass_normal = (NormalMatrix * vec4(in_normal, 0.)).xyz;
  pass_uvs = in_uvs;
}
