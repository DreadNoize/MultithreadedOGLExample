#pragma once

#ifndef TRACKBALL_HPP
#define TRACKBALL_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>
#include <string>

class Trackball {
  public:
    Trackball() :
      m_radius{5.0f},
      m_up{glm::fvec3(0.0f,1.0f,0.0f)},
      m_target{glm::fvec3(0.0f,0.0f,0.0f)},
      m_position{glm::fvec3(0.0f,0.0f,-5.0f)},
      m_proj_matrix{glm::fmat4(1.0f)} {
        m_view_matrix = glm::lookAt(m_position, m_target, m_up);
        // printMatrix(m_view_matrix, "CTOR VIEW");
      }
    Trackball(float const& r, glm::fvec3 const& up, glm::fvec3 const& target, glm::fvec3 const& position) :
      m_radius{r},
      m_up{up},
      m_target{target},
      m_position{position},
      m_proj_matrix{glm::fmat4(1.0f)} {
        m_view_matrix = glm::lookAt(m_position, m_target, m_up);
        // printMatrix(m_view_matrix, "CTOR VIEW");  
      }
    ~Trackball() {}
	  void printMatrix(glm::fmat4 const& matrix, std::string const& name);
    void rotate_T(float rTheta, float rPhi);
    void zoom_T(float dist);
    void pan_T(float x, float y);
    glm::fvec3 get_camera_position();
    glm::fmat4 get_view();
    glm::fmat4 get_proj();
    float get_up();

    glm::fvec3 convert_to_cartesian();

    void update();
  private:
    float m_radius;

    glm::fvec3 m_up;
    glm::fvec3 m_target;
    glm::fvec3 m_position;
    glm::fmat4 m_view_matrix;
    glm::fmat4 m_proj_matrix;
};


#endif