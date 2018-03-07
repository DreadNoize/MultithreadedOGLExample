#version 330

in vec2 pass_uvs;
in float pass_depth;

out vec4 out_color;

uniform sampler2D quad_tex;
uniform sampler2D depth_tex;


void main() {
  if(pass_depth >= 1.0f) {
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
  // out_color = texture(quad_tex, pass_uvs);
  out_color = vec4(pass_depth,pass_depth,pass_depth,1.0);

}