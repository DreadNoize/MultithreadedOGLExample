#version 330

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in Vertex {
  vec2 uvs;
  float depth;
} vertex[];

out Fragment {
  vec2 uvs;
  float depth;
} fragment;

void main() {
  for(int i = 0; i < gl_in.length(); i++){
    if(gl_in[i].gl_Position.z > 0.999) {
      return;
    }
  }
  for(int i = 0; i < gl_in.length(); i++){
    // if(gl_in[i].gl_Position.z < 0.1) {
    //   fragment.depth = 0.0;
    // } else {
    //   fragment.depth = vertex[i].depth;
    // }
    fragment.depth = vertex[i].depth;
    fragment.uvs = vertex[i].uvs;
    gl_Position = gl_in[i].gl_Position;
    EmitVertex();
  }
  EndPrimitive();
}