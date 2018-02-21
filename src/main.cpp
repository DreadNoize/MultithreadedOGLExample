#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
#include "tiny_obj_loader.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glbinding/gl/gl.h>
#include <glbinding/Binding.h>
#include <glbinding/Meta.h>

// #include "model_loader.hpp"
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_TGA
#define STBI_NO_LINEAR
// get extensive error on failure
#define STBI_FAILURE_USERMSG
// create implementation here
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <limits>
#include <chrono>
//#include <unistd.h>

#include "vertex.hpp"

using namespace gl;

// "Members"
// window specs
int height = 1024;
int width = 1280;

// windows/contexts
GLFWwindow *window;
GLFWwindow *offscreen_context;

// vao
GLuint quad_vertex_array;
GLuint model_vertex_array;
GLuint model_uv_buffer;
    // vbo
    GLuint quad_vertex_buffer;
GLuint model_vertex_buffer;
// index buffer
GLuint model_index_buffer;
// shader programs
GLuint default_program;
GLuint quad_program;
GLuint point_program;


// fbo, texture and renderbuffer handles
GLuint quad_handle;
GLuint fbo_handle;
GLuint rb_handle;
GLuint tex_handle;

// mutex
std::mutex gl_mutex;

// time for fps calculation
double m_last_second_time;
unsigned m_frames_per_second;

// matrices
glm::fmat4 view_transform = glm::fmat4{1.0f};
glm::fmat4 view_projection = glm::fmat4{1.0f};
glm::fmat4 model_matrix = glm::fmat4{1.0f};


GLsizei n_elements;

// quad
float vertices[] = {
    -1.0, -1.0, 0.0,
    0.0, 0.0,
    1.0, -1.0, 0.0,
    1.0, 0.0,
    -1.0, 1.0, 0.0,
    0.0, 1.0,
    1.0, 1.0, 0.0,
    1.0, 1.0
};

std::vector<glm::vec3> m_vertices;
std::vector<glm::vec2> m_uvs;
std::vector<glm::vec3> m_normals;
std::vector<uint32_t> m_indices;

void printMatrix(glm::fmat4 const& matrix, std::string const& name);

// UNIFORM FUNCTIONS
void updateView() {
  glm::fmat4 view_matrix = glm::inverse(view_transform);
  GLuint location = glGetUniformLocation(default_program, "ViewMatrix");  
  glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(view_matrix));
  printMatrix(view_matrix, "View");
}
void updateProjection() {
  GLuint location = glGetUniformLocation(default_program, "ProjectionMatrix");
  glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(view_projection));
  printMatrix(view_projection, "Projection");
}
void setProjection(GLFWwindow* win, int w, int h) {
  glViewport(0, 0, w, h);

  float aspect_ratio = float(w)/float(h);
  float fov = glm::radians(45.0f);
  std::cout << "Aspect: " << aspect_ratio	<< " FOV: " << fov << std::endl;

  glm::fmat4 cam_projection = glm::perspective(fov, aspect_ratio, 0.1f, 100.0f);
  printMatrix(cam_projection, "Cam");
  view_projection = cam_projection;
  updateProjection();
}
void updateUniforms() {
  glUseProgram(default_program);
  int w_width, w_height;
  glfwGetFramebufferSize(window, &w_width, &w_height);
  std::cout << "W: " << w_width << " H: " << w_height << std::endl;
  setProjection(window, width, height);

  view_transform = glm::translate(view_transform, glm::vec3{0.0f,0.0f,10.0f});
  updateView();
}

// UTILS
/* Print matrix */
void printMatrix(glm::fmat4 const& matrix, std::string const& name) {
  std::cout << "Matrix: " << name << std::endl;
  std::cout << matrix[0][0] << "," << matrix[0][1] << "," << matrix[0][2] << "," << matrix[0][3] << std::endl; 
  std::cout << matrix[1][0] << "," << matrix[1][1] << "," << matrix[1][2] << "," << matrix[1][3] << std::endl; 
  std::cout << matrix[2][0] << "," << matrix[2][1] << "," << matrix[2][2] << "," << matrix[2][3] << std::endl; 
  std::cout << matrix[3][0] << "," << matrix[3][1] << "," << matrix[3][2] << "," << matrix[3][3] << std::endl; 
}
void watch_gl_errors() {
  glbinding::setCallbackMaskExcept(glbinding::CallbackMask::After | glbinding::CallbackMask::ParametersAndReturnValue, {"glGetError", "glBegin", "glVertex3f", "glColor3f"});
  glbinding::setAfterCallback(
      [](glbinding::FunctionCall const &call) {
        GLenum error = glGetError();
        if (error != GL_NO_ERROR)
        {
          // print name
          std::cerr << "OpenGL Error: " << call.function->name() << "(";
          // parameters
          for (unsigned i = 0; i < call.parameters.size(); ++i)
          {
            std::cerr << call.parameters[i]->asString();
            if (i < call.parameters.size() - 1)
              std::cerr << ", ";
          }
          std::cerr << ")";
          // return value
          if (call.returnValue)
          {
            std::cerr << " -> " << call.returnValue->asString();
          }
          // error
          std::cerr << " - " << glbinding::Meta::getString(error) << std::endl;
          // throw exception to allow for backtrace
          throw std::runtime_error("Execution of " + std::string(call.function->name()));
          exit(EXIT_FAILURE);
        }
      });
}
void show_fps(GLFWwindow *win) {
  ++m_frames_per_second;
  double current_time = glfwGetTime();
  if (current_time - m_last_second_time >= 1.0) {
    std::string title{"OpenGL Framework - "};
    title += std::to_string(m_frames_per_second) + " fps";

    glfwSetWindowTitle(win, title.c_str());
    m_frames_per_second = 0;
    m_last_second_time = current_time;
  }
}

// MOUSE AND KEY CALLBACK
void key_callback(GLFWwindow *win, int key, int scancode, int action, int mods) {
  if ((key == GLFW_KEY_ESCAPE /* || key == GLFW_KEY_Q */) && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(offscreen_context, 1);
    glfwDestroyWindow(offscreen_context);
    glfwSetWindowShouldClose(win, 1);
  } /* else if (key == GLFW_KEY_W && (action == GLFW_REPEAT || action== GLFW_PRESS)) {
		view_transform = glm::translate(view_transform, glm::fvec3{0.0f, 0.0f, -0.1f});
		updateView();
	} else if (key == GLFW_KEY_S && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		view_transform = glm::translate(view_transform, glm::fvec3{0.0f, 0.0f, 0.1f});
		updateView();
	} else if (key == GLFW_KEY_D && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		view_transform = glm::translate(view_transform, glm::fvec3{ -0.1f, 0.0f, 0.0f });
		updateView();
	} else if (key == GLFW_KEY_A && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		view_transform = glm::translate(view_transform, glm::fvec3{ 0.1f, 0.0f, 0.0f });
		updateView();
	} else if (key == GLFW_KEY_E && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		view_transform = glm::translate(view_transform, glm::fvec3{ 0.0f, 0.1f, 0.0f });
		updateView();
	} else if (key == GLFW_KEY_Q && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		view_transform = glm::translate(view_transform, glm::fvec3{ 0.0f, -0.1f, 0.0f });
		updateView();
	} */
}
void key_callback_thread(GLFWwindow *win, int key, int scancode, int action, int mods) {
  /* if ((key == GLFW_KEY_ESCAPE) && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, 1);
  } else */ if (key == GLFW_KEY_W && (action == GLFW_REPEAT || action== GLFW_PRESS)) {
		view_transform = glm::translate(view_transform, glm::fvec3{0.0f, 0.0f, -0.1f});
		updateView();
	} else if (key == GLFW_KEY_S && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		view_transform = glm::translate(view_transform, glm::fvec3{0.0f, 0.0f, 0.1f});
		updateView();
	} else if (key == GLFW_KEY_D && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		view_transform = glm::translate(view_transform, glm::fvec3{ -0.1f, 0.0f, 0.0f });
		updateView();
	} else if (key == GLFW_KEY_A && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		view_transform = glm::translate(view_transform, glm::fvec3{ 0.1f, 0.0f, 0.0f });
		updateView();
	} else if (key == GLFW_KEY_E && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		view_transform = glm::translate(view_transform, glm::fvec3{ 0.0f, 0.1f, 0.0f });
		updateView();
	} else if (key == GLFW_KEY_Q && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		view_transform = glm::translate(view_transform, glm::fvec3{ 0.0f, -0.1f, 0.0f });
		updateView();
	}
}
void mouse_callback(GLFWwindow *win, double x, double y) {
}

// LOADING
/* loading shaders */
GLuint loadShader(std::string const &shader_path, GLenum type) {
  // get shader name for prettier error messages
  std::string shader_name = shader_path.substr(shader_path.find_last_of("/\\") + 1);
  //create shader
  GLuint shader = glCreateShader(type);
  // get shader code from file
  std::string shader_code;
  std::ifstream shader_stream(shader_path, std::ios::in);
  if (shader_stream.is_open()) {
    std::string line = "";
    while (getline(shader_stream, line)) {
      shader_code += "\n" + line;
    }
    shader_stream.close();
  }
  else {
    std::cerr << "File \'" << shader_path << "\' not found" << std::endl;

    throw std::invalid_argument(shader_name);
  }
  const char *shader_char = shader_code.c_str();
  glShaderSource(shader, 1, &shader_char, 0);
  // compile shader
  GLint result = 0;
  std::cout << "Compiling shader: " << shader_name << std::endl;
  glCompileShader(shader);

  // get compile status
  glGetShaderiv(shader, GL_COMPILE_STATUS, &result);

  // if compilation failed, get error log and print error
  if (result == 0) {
    GLint log_length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);

    GLchar *info_log = (GLchar *)malloc(sizeof(GLchar) * log_length);
    glGetShaderInfoLog(shader, log_length, &log_length, info_log);
    std::string error{};
    std::istringstream error_stream{info_log};
    while (std::getline(error_stream, error)) {
      std::cerr << "ERROR: " << shader_name << " - " << error << std::endl;
    }
    glDeleteShader(shader);
    throw std::logic_error("Compilation of " + shader_name);
  }

  return shader;
}
GLuint loadProgram(std::string const &vertex_shader_path, std::string const &fragment_shader_path) {
  GLuint program = glCreateProgram();

  GLuint vertex_shader = loadShader(vertex_shader_path, GL_VERTEX_SHADER);
  GLuint fragment_shader = loadShader(fragment_shader_path, GL_FRAGMENT_SHADER);

  // attach shaders to program
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);

  // link program
  glLinkProgram(program);

  // check linking
  GLint result = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &result);

  // if linking failed, get error log and print it
  if (result == 0) {
    GLint log_length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);

    GLchar *info_log = (GLchar *)malloc(sizeof(GLchar) * log_length);
    glGetProgramInfoLog(program, log_length, &log_length, info_log);
    std::string error{};
    std::istringstream error_stream{info_log};
    while (std::getline(error_stream, error)) {
      std::cerr << "ERROR: "
                << "LINKING FAILED - " << error << std::endl;
    }

    glDeleteProgram(program);
    throw std::logic_error("Linking of shaders");
  }
  glDetachShader(program, vertex_shader);
  glDetachShader(program, fragment_shader);

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  return program;
}
/* loading model */
void loadModel(std::string const &modelPath) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string err;

  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, modelPath.c_str()))  {
    throw std::runtime_error(err);
  }

  for (const auto &shape : shapes)  {
    for (const auto &index : shape.mesh.indices) {
      // Vertex vertex = {};
      m_indices.push_back(m_indices.size());
	  m_vertices.push_back(glm::vec3{
		  attrib.vertices[3 * index.vertex_index + 0],
		  attrib.vertices[3 * index.vertex_index + 1],
		  attrib.vertices[3 * index.vertex_index + 2] });

	  m_uvs.push_back(glm::vec2{
		  attrib.texcoords[2 * index.texcoord_index + 0],
		  attrib.texcoords[2 * index.texcoord_index + 1] });
    }
  }
}
/* loading textures */
void loadTexture(std::vector<std::uint8_t>& pixel, 
                  int& width, 
                  int& height, 
                  GLenum& channel,
                  GLenum& channel_format) {
  stbi_set_flip_vertically_on_load(true);

  uint8_t* data_ptr;
  int format = STBI_default;

  data_ptr = stbi_load("../../resources/chalet.jpg", &width, &height, &format, STBI_rgb_alpha);

  if (!data_ptr) {
    throw std::logic_error(std::string{"stb_image: "} + stbi_failure_reason());
  }

  // determine format of image data, internal format should be sized
  std::size_t num_components = 0;
  if (format == STBI_grey) {
    channel = GL_RED;
    num_components = 1;
  } else if (format == STBI_grey_alpha) {
    channel = GL_RG;
    num_components = 2;
  } else if (format == STBI_rgb) {
    channel = GL_RGB;
    num_components = 3;
  } else if (format == STBI_rgb_alpha) {
    channel = GL_RGBA;
    num_components = 4;
  } else {
    throw std::logic_error("stb_image: misinterpreted data, incorrect format");
  }

  channel_format = GL_UNSIGNED_BYTE;

  pixel.resize(width * height * num_components);
  std::memcpy(&pixel[0], data_ptr, pixel.size());

  stbi_image_free(data_ptr);
}

// INITIALISATION
void initWindow() {
  //Check if glfw is working
  if (!glfwInit()) {
    std::exit(EXIT_FAILURE);
  }
  // set opengl version
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);

  // window creation
  window = glfwCreateWindow(width, height, "OpenGL Multithreading", NULL, NULL);
  if (!window) {
    glfwTerminate();
    std::exit(EXIT_FAILURE);
  }

  // make window current
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    // set mouse and key callback
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);

    // initialize glindings in this context
    glbinding::Binding::initialize();
    watch_gl_errors();
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
}
void initOffscreenWindow() {
  // Init second window for second thread
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  offscreen_context = glfwCreateWindow(width, height, "Offscreen Window", NULL, window);
  glfwSetKeyCallback(offscreen_context, key_callback_thread);  
  if(!offscreen_context) {
    glfwTerminate();
    std::cerr << "Failed to create offscreen context" << std::endl;
    std::exit(EXIT_FAILURE);
  }
}
void initShaders() {
  default_program = loadProgram("../../shader/point.vert", "../../shader/point.frag");

  quad_program = loadProgram("../../shader/quad.vert", "../../shader/quad.frag");
}
void initQuad() {
  glGenVertexArrays(1, &quad_vertex_array);
  glBindVertexArray(quad_vertex_array);
  glGenBuffers(1, &quad_vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, quad_vertex_buffer);

  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 20, &vertices, GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(sizeof(float) * 3));
}
void initGeometry() {
  std::cout << "Loading model ..." << std::endl;
  loadModel("../../resources/chalet.obj");

  glGenVertexArrays(1, &model_vertex_array);
  glBindVertexArray(model_vertex_array);

  glGenBuffers(1, &model_vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, model_vertex_buffer);
  std::cout << "Binding Buffer Data (Vertex) ..." << std::endl;
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * m_vertices.size(), m_vertices.data(), GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*) 0);

  glGenBuffers(1, &model_uv_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, model_uv_buffer);
  std::cout << "Binding Buffer Data (UV) ..." << std::endl;
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * m_uvs.size(), m_uvs.data(), GL_STATIC_DRAW);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*) 0);
}
void initTexture() {
  glActiveTexture(GL_TEXTURE3);
  glGenTextures(1, &quad_handle);
  glBindTexture(GL_TEXTURE_2D, quad_handle);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

  glActiveTexture(GL_TEXTURE0);
  glGenTextures(1, &tex_handle);
  glBindTexture(GL_TEXTURE_2D, tex_handle);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  std::vector<std::uint8_t> pixels;
  int width = 0;
  int height = 0;
  int depth = 0;

  GLenum channel = GL_NONE;
  GLenum channel_format;

  loadTexture(pixels, width, height, channel, channel_format);

  std::cout << "Width: " << width << " Height: " << height << std::endl;

  glTexImage2D(GL_TEXTURE_2D, 0, channel, width, height, 0, channel, channel_format, pixels.data());
}
void initFramebufferThread() {
  glActiveTexture(GL_TEXTURE3);
  glGenRenderbuffers(1, &rb_handle);
  glBindRenderbuffer(GL_RENDERBUFFER, rb_handle);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);


  glGenFramebuffers(1, &fbo_handle);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_handle);
  glBindRenderbuffer(GL_RENDERBUFFER, rb_handle);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rb_handle);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, quad_handle, 0);

  GLenum draw_buffer[1] = {GL_COLOR_ATTACHMENT0};
  glDrawBuffers(1, draw_buffer);

  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    std::cerr << "Error: Framebuffer object incomplete!" << std::endl;
  }
  glFlush();
}
/* void copyFrameBuffer(GLuint fbuffer) {
  glBindFramebuffer(GL_READ_FRAMEBUFFER, fbuffer);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glBlitFramebuffer(0, 0, width, height,
                    0, 0, width, height,
                    GL_COLOR_BUFFER_BIT,
                    GL_NEAREST);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}
 */
void render() {
  std::cout << "Rendering ..." << std::endl;
  glfwMakeContextCurrent(offscreen_context);
  glbinding::Binding::initialize();
  watch_gl_errors();
  glViewport(0, 0, width, height);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  // glBindTexture(GL_TEXTURE_2D, quad_handle);
  initGeometry();
  updateUniforms();
  gl_mutex.lock();
  initFramebufferThread();
  gl_mutex.unlock();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  for(;;) {
    gl_mutex.lock();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_handle);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindVertexArray(model_vertex_array);
    glUseProgram(default_program);

    glm::fmat4 model_matrix_ = glm::fmat4{1.0f};
	  model_matrix_ = glm::rotate(model_matrix_, float(glfwGetTime()) * 2.0f, glm::vec3{1.0f,1.0f,1.0f});
    GLuint location = glGetUniformLocation(default_program, "ModelMatrix");
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(model_matrix_));

	  glDrawArrays(GL_TRIANGLES, 0, m_vertices.size());
    glFlush();

    gl_mutex.unlock();
  }
}

void draw() {
  glActiveTexture(GL_TEXTURE3);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glUseProgram(quad_program);
  glBindVertexArray(quad_vertex_array);
  GLuint location = glGetUniformLocation(quad_program, "quad_tex");
  glUniform1i(location, 3);

  model_matrix = glm::rotate(model_matrix, float(glfwGetTime()) * 0.00001f, glm::vec3{0.0,0.0,1.0});
  location = glGetUniformLocation(quad_program, "ModelMatrix");
  glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(model_matrix));
  // printMatrix(model_matrix, "Model");

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 8);
}

int main(int argc, char **argv) {
  initWindow();
  initShaders();
  initTexture();
  initQuad();
  initOffscreenWindow();
  std::cout << "Start thread" << std::endl;
  std::thread t = std::thread(render);
  glfwMakeContextCurrent(window);
  glBindTexture(GL_TEXTURE_2D, quad_handle);
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    draw();
    glfwSwapBuffers(window);
    show_fps(window);
  }
  glfwSetWindowShouldClose(offscreen_context, 1);
  t.join();
  return 1;
}
