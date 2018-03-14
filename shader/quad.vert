#version 330
#extension GL_ARB_explicit_attrib_location : require

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uvs;

// OLD PROJECTION VIEW
uniform mat4 ProjectionMatrix;
uniform mat4 ViewMatrix;
// NEW VIEW
uniform mat4 SecondView;
uniform mat4 ModelMatrix;
uniform sampler2D depth_tex;

// out vec2 pass_uvs;
// out float pass_depth;
out Vertex {
  vec2 uvs;
  float depth;
} vertex;

float get_depth(in vec2 uvs) {
  float zNear = 0.1;  
  float zFar  = 100.0;
  float depth = texture2D(depth_tex, uvs).x;
  depth = 2.0 * depth - 1.0;
  return depth;
}

void main() {
  // vec4 depth = texture(depth_tex, in_uvs);
  float z = get_depth(in_uvs);
  vec2 temp = vec2(in_uvs.x, in_uvs.y)*2.0 - 1.0;
  vec4 model_coords = vec4(temp, z, 1.0);
  model_coords = (inverse(ViewMatrix)*inverse(ProjectionMatrix)) * model_coords;
  model_coords /= model_coords.w;
  model_coords = (ProjectionMatrix * SecondView * ModelMatrix) * model_coords;
  gl_Position = model_coords;
  vertex.uvs = in_uvs;
  vertex.depth = gl_Position.z;
}