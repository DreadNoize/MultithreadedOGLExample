#version 330
#extension GL_ARB_bindless_texture : require

in Fragment {
  vec2 uvs;
  float depth;
} fragment;

out vec4 out_color;

layout(bindless_sampler) uniform sampler2D quad_tex;
layout(bindless_sampler) uniform sampler2D depth_tex;


void main() {
  if(fragment.depth >= 1.0f) {
    discard;
  }
  /* if(pass_uvs.x < 0.5) {
    if(pass_uvs.y < 0.5) {
      out_color = vec4(0.752, 0.223, 0.168, 1.0); //red
    } else {
      out_color = vec4(0.952, 0.611, 0.070, 1.0); //orange
    }
  } else {
    if(pass_uvs.y < 0.5) {
      out_color = vec4(0.086, 0.627, 0.521, 1.0); //turquoise
    } else {
      out_color = vec4(0.172, 0.243, 0.313, 1.0); // dark blue
    }
  } */
  out_color = texture(quad_tex, fragment.uvs);
  // out_color = vec4(fragment.depth,fragment.depth,fragment.depth,1.0);
  // out_color = vec4(fragment.uvs.x,0.0,0.0,1.0);

}