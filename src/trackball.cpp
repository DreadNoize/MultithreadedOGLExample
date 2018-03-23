#include "trackball.hpp"
#include <iostream>
#include <glm/gtc/constants.hpp>


void Trackball::printMatrix(glm::fmat4 const& matrix, std::string const& name) {
  std::cout << "Matrix: " << name << std::endl;
  std::cout << matrix[0][0] << "," << matrix[0][1] << "," << matrix[0][2] << "," << matrix[0][3] << std::endl; 
  std::cout << matrix[1][0] << "," << matrix[1][1] << "," << matrix[1][2] << "," << matrix[1][3] << std::endl; 
  std::cout << matrix[2][0] << "," << matrix[2][1] << "," << matrix[2][2] << "," << matrix[2][3] << std::endl; 
  std::cout << matrix[3][0] << "," << matrix[3][1] << "," << matrix[3][2] << "," << matrix[3][3] << std::endl; 
}

void Trackball::rotate_T(float pitch, float yaw) {
  /* if(up > 0.0f) {
    theta += rTheta;
  } else {
    theta -= rTheta;
  }
  // theta += rTheta;
  phi += rPhi;

  if(phi > 2*glm::pi<float>()) {
    phi -= 2*glm::pi<float>();
  } else if ( phi < -2*glm::pi<float>()) {
    phi += 2*glm::pi<float>();
  }

  if ((phi > 0 && phi < glm::pi<float>()) || (phi < -glm::pi<float>() && phi > -2*glm::pi<float>())) {
		m_up = 1.0f;
	} else {
		m_up = -1.0f;
	}*/
  glm::fvec3 target_to_cam = m_position - m_target;
  glm::fvec3 right_vec = glm::normalize(glm::cross(target_to_cam, m_up));
  m_up = glm::normalize(glm::cross(right_vec, target_to_cam));
  // std::cout << "target vector:\n" << "X: " << target_to_cam.x << ", Y: " << target_to_cam.y << ", Z: " << target_to_cam.z << std::endl;
  // std::cout << "right vector:\n" << "X: " << right_vec.x << ", Y: " << right_vec.y << ", Z: " << right_vec.z << std::endl;
  // std::cout << "up vector:\n" << "X: " << m_up.x << ", Y: " << m_up.y << ", Z: " << m_up.z << std::endl;
  glm::fmat4 yaw_rot = glm::fmat4(1.0f);
  glm::fmat4 pitch_rot = glm::fmat4(1.0f);

  yaw_rot = glm::rotate(yaw_rot, yaw, m_up);
  pitch_rot = glm::rotate(pitch_rot, pitch, right_vec);

  m_position = glm::fvec3(yaw_rot * glm::fvec4(m_position - m_target, 0.0f)) + m_target;
  m_position = glm::fvec3(pitch_rot * glm::fvec4(m_position - m_target, 0.0f)) + m_target;
  // std::cout << "Position:\n" << "X: " << m_position.x << ", Y: " << m_position.y << ", Z: " << m_position.z << std::endl;

  update(); 
}
void Trackball::zoom_T(float dist) {
  glm::fvec3 target_to_cam = glm::normalize(m_position - m_target);
  m_position += (-target_to_cam*dist) + m_target;
  update();
}
void Trackball::pan_T(float x, float y) {
  // glm::fvec3 look = glm::normalize(target - get_camera_position());
  // glm::fvec3 world_up = glm::fvec3(0.0f, m_up, 0.0f);

  // glm::fvec3 right = glm::cross(look, world_up);
  // glm::fvec3 r_up = glm::cross(look, right);

  // target += ((right * x) + (r_up * y));
}
glm::fvec3 Trackball::get_camera_position() {
  return m_position;
}
glm::fmat4 Trackball::get_view() {
  return m_view_matrix;
}
glm::fmat4 Trackball::get_proj() {
  return m_proj_matrix;
}
glm::fvec3 Trackball::convert_to_cartesian() {
  // float x = radius * glm::sin(phi) * glm::sin(theta);
  // float y = radius * glm::sin(phi) * glm::cos(theta);
  // float z = radius * glm::cos(phi);

  // std::cout << "X: " << x << ", Y: " << y << ", -Z: " << -z << std::endl;

  // 
  return glm::fvec3(0.0f);
}
void Trackball::update() {
  // // std::cout << "CAMPOS:\n" << "X: " << get_camera_position().x << ", Y: " << get_camera_position().y << ", Z: " << get_camera_position().z << std::endl;
  // // std::cout << "TARGET:\n" << "X: " << target.x << ", Y: " << target.y << ", Z: " << target.z << std::endl;
  // // printMatrix(glm::lookAt(get_camera_position(), target, glm::fvec3(0.0f, 1.0f, 0.0f)), "LOOKAT TRACKBALL");
  m_view_matrix = glm::lookAt(m_position, m_target, m_up);
}