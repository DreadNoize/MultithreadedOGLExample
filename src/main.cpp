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

#define  FREEIMAGE_LIB
#include <FreeImage.h>

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

#include "trackball.hpp"

using namespace gl;

// "Members"
// trackball
Trackball trackball;
// window specs
int width = 1280;
int height = 1024;

// windows/contexts
GLFWwindow *window;
GLFWwindow *offscreen_context;

// vao
GLuint quad_vao;
GLuint model_vao;
GLuint model_uv_vao;
GLuint grid_vao;
GLuint grid_uv_vao;
// vbo
GLuint quad_vbo;
GLuint model_vbo;
GLuint grid_vbo;
// index buffer
GLuint model_ibo;
GLuint grid_ibo;
// shader programs
GLuint default_program;
GLuint quad_program;
GLuint point_program;


// fbo, texture and renderbuffer handles
GLuint quad_handle;
GLuint fbo_handle;
GLuint rb_handle;
GLuint tex_handle;
GLuint depth_handle;

// mutex
std::mutex gl_mutex;

// time for fps calculation
double m_last_second_time;
unsigned m_frames_per_second;

// matrices
// glm::fmat4 view_transform = glm::translate(glm::fmat4{1.0f}, glm::vec3{0.0f,0.0f,5.0f});
glm::fmat4 view_transform = trackball.get_view();
glm::fmat4 second_view_rot = glm::rotate(glm::fmat4{1.0f}, -0.1f, glm::vec3(0, 1.0, 0));
glm::fmat4 view_projection = glm::fmat4{1.0f};
glm::fmat4 model_matrix = glm::fmat4{1.0f};
glm::fmat4 second_view = glm::translate(glm::fmat4{1.0f}, glm::vec3{0.0f,0.0f,5.0f});


// timers
GLuint64 start_time;
GLuint64 stop_time;

GLuint query_handle[4];
GLuint query_handle_th[4];

GLint stopTimerAvailable = 0;

bool rmb_clicked = false;
bool lmb_clicked = false;
double x_clicked; // x coordinate of mouse, when clicked.
double y_clicked; // y coordinate of mouse, when clicked.


GLsizei n_elements;

// quad
float vertices_quad[] = {
    -1.0, -1.0, 0.0,
    0.0, 0.0,
    1.0, -1.0, 0.0,
    1.0, 0.0,
    -1.0, 1.0, 0.0,
    0.0, 1.0,
    1.0, 1.0, 0.0,
    1.0, 1.0
};

std::vector<glm::vec3> g_vertices;
std::vector<glm::uvec3> g_indices;
std::vector<glm::vec2> g_uvs;

std::vector<glm::vec3> m_vertices;
std::vector<glm::vec2> m_uvs;
std::vector<glm::vec3> m_normals;
std::vector<uint32_t> m_indices;

void printMatrix(glm::fmat4 const& matrix, std::string const& name);
void initShaders();
// UNIFORM FUNCTIONS
void updateView(GLuint const& program) {
  glm::fmat4 view_matrix = glm::inverse(view_transform);
  GLuint location = glGetUniformLocation(program, "ViewMatrix"); 
  glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(view_matrix));
  glm::fmat4 sec_view_matrix = glm::inverse(second_view);
  location = glGetUniformLocation(program, "SecondView"); 
  glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(sec_view_matrix));
  // printMatrix(view_matrix, "View");
}
void updateProjection(GLuint const& program) {
  GLuint location = glGetUniformLocation(program, "ProjectionMatrix");
  glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(view_projection));
  // printMatrix(view_projection, "Projection");
}
void setProjection(GLFWwindow* win, int w, int h, GLuint const& program) {
  glViewport(0, 0, w, h);

  float aspect_ratio = float(w)/float(h);
  float fov = glm::radians(45.0f);
  std::cout << "Aspect: " << aspect_ratio	<< " FOV: " << fov << std::endl;

  glm::fmat4 cam_projection = glm::perspective(fov, aspect_ratio, 0.1f, 100.0f);
  printMatrix(cam_projection, "Cam");
  view_projection = cam_projection;
  updateProjection(program);
}
void updateUniforms(GLuint const& program) {
  glUseProgram(program);
  int w_width, w_height;
  glfwGetFramebufferSize(window, &w_width, &w_height);
  std::cout << "W: " << w_width << " H: " << w_height << std::endl;
  setProjection(window, width, height, program);

  updateView(program);
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
  }  else if (key == GLFW_KEY_W && (action == GLFW_REPEAT || action== GLFW_PRESS)) {
		view_transform = glm::translate(view_transform, glm::fvec3{0.0f, 0.0f, 0.1f});
	  // updateUniforms(default_program);
		updateView(quad_program);
	} else if (key == GLFW_KEY_S && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		view_transform = glm::translate(view_transform, glm::fvec3{0.0f, 0.0f, -0.1f});
	  updateUniforms(default_program);
		updateView(quad_program);
	} else if (key == GLFW_KEY_D && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		view_transform = glm::translate(view_transform, glm::fvec3{ -0.1f, 0.0f, 0.0f });
	  updateUniforms(default_program);
		updateView(quad_program);
	} else if (key == GLFW_KEY_A && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		view_transform = glm::translate(view_transform, glm::fvec3{ 0.1f, 0.0f, 0.0f });
	  updateUniforms(default_program);
		updateView(quad_program);
	} else if (key == GLFW_KEY_E && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		view_transform = glm::translate(view_transform, glm::fvec3{ 0.0f, 0.1f, 0.0f });
	  updateUniforms(default_program);
		updateView(quad_program);
	} else if (key == GLFW_KEY_Q && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		view_transform = glm::translate(view_transform, glm::fvec3{ 0.0f, -0.1f, 0.0f });
	  updateUniforms(default_program);
		updateView(quad_program);
	} else if (key == GLFW_KEY_R && action == GLFW_PRESS) {
    glDeleteProgram(default_program);
    glDeleteProgram(quad_program);
    initShaders();
	  updateUniforms(default_program);
	  updateUniforms(quad_program);
  }
}
void key_callback_thread(GLFWwindow *win, int key, int scancode, int action, int mods) {
  /* if ((key == GLFW_KEY_ESCAPE) && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, 1);
  } else  if (key == GLFW_KEY_W && (action == GLFW_REPEAT || action== GLFW_PRESS)) {
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
	}*/
  if (key == GLFW_KEY_R && action == GLFW_PRESS) {
    glDeleteProgram(default_program);
    glDeleteProgram(quad_program);
    initShaders();
    updateUniforms(default_program);
    updateUniforms(quad_program);
  }
}
void mouse_callback(GLFWwindow *win, double x, double y) {
  if(rmb_clicked) {
    // if(x - x_clicked < 0.0) {
    //   x_clicked = x;
    //   view_transform = glm::rotate(view_transform, float(x*0.000005), glm::vec3{0.0,1.0,0.0});
    //   updateView(quad_program);
    // } else if(x - x_clicked > 0.0) {
    //   x_clicked = x;
    //   view_transform = glm::rotate(view_transform, float(x*0.000005), glm::vec3{0.0,-1.0,0.0});
    //   updateView(quad_program);
    // }
    float yaw = ((float)(x_clicked - x) / 500); 
    float pitch = ((float)(y_clicked - y) / 500);
    // std::cout << "YAW: " << yaw << ", PITCH: " << pitch << std::endl;

	  trackball.rotate_T(pitch, yaw);
    view_transform = trackball.get_view();
    updateView(default_program);
    // updateView(quad_program);
    x_clicked = x;
    y_clicked = y;
  } /* else if (lmb_clicked) {
    float delta_x = (float) (x_clicked - x);
    float delta_y = (float) (y_clicked - y);

    trackball.pan_T(delta_x*0.01, delta_y*0.01);
    view_transform = trackball.get_view();
    updateView(default_program);
    x_clicked = x;
    y_clicked = y;
  } */
  // std::cout << "MouseX: " << x << "MouseY: " << y << std::endl;
}

void click_callback(GLFWwindow *win, int button, int action, int mods) {
  if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
    rmb_clicked = true;
    glfwGetCursorPos(win, &x_clicked, &y_clicked);
    // std::cout << "MouseX: " << x_clicked << "MouseY: " << y_clicked << std::endl;
  } else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
    rmb_clicked = false;
  } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    lmb_clicked = true;
    glfwGetCursorPos(win, &x_clicked, &y_clicked);
  } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
    lmb_clicked = false;
  }
}

void scroll_callback(GLFWwindow *win, double x_offset, double y_offset) {
  // std::cout << "Scrolling: " << y_offset << std::endl;
  // glm::fmat4 temp = trackball.get_view();
  // printMatrix(temp, "Trackballview before Scroll");
  // glm::fvec3 cp = trackball.get_camera_position();
  // std::cout << "Camera position:\n" << "X: " << cp.x << ", Y: " << cp.y << ", Z: " << cp.z << std::endl;   
  trackball.zoom_T(y_offset * 0.1);
  // std::cout << "UP: " << trackball.get_up() << std::endl;
  // printMatrix(view_transform, "VIEW_TRANSFORM");
  // glm::fmat4 temp2 = trackball.get_view();
  // printMatrix(temp2, "Trackballview after Scroll");
  // printMatrix(glm::lookAt(glm::fvec3(0.0f,0.0f,5.0f), glm::fvec3(0.0f,0.0f,0.0f), glm::fvec3(0.0f,1.0f,0.0f)), "LOOKAT");
  view_transform = trackball.get_view();
  updateView(default_program);
  // updateView(quad_program);
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
GLuint loadProgram(std::string const &vertex_shader_path, std::string const &fragment_shader_path, std::string const geometry_shader_path = "") {
  GLuint program = glCreateProgram();

  GLuint vertex_shader = loadShader(vertex_shader_path, GL_VERTEX_SHADER);
  GLuint fragment_shader = loadShader(fragment_shader_path, GL_FRAGMENT_SHADER);
  GLuint geometry_shader;
  // attach shaders to program
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);

  if(!geometry_shader_path.empty()) {
    geometry_shader = loadShader(geometry_shader_path, GL_GEOMETRY_SHADER);
    glAttachShader(program, geometry_shader);
  }
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

  if(!geometry_shader_path.empty()) {
    glDetachShader(program, geometry_shader);
    glDeleteShader(geometry_shader);    
  }
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
void loadTextureSTB(std::vector<std::uint8_t>& pixel, 
                  int& width, 
                  int& height, 
                  GLenum& channel,
                  GLenum& channel_format) {
  stbi_set_flip_vertically_on_load(false);

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
GLubyte* loadTexture( int& width, 
                  int& height, 
                  GLenum& channel,
                  GLenum& channel_format) {
  FREE_IMAGE_FORMAT format = FreeImage_GetFileType("../../resources/pig_d.dds", 0);
  if(format == -1) {
    std::cerr << "File not found" << std::endl;
    throw std::logic_error("Texture loading");
  }

  FIBITMAP* bitmap = FreeImage_Load(format, "../../resources/pig_d.dds");

  int bpp = FreeImage_GetBPP(bitmap);

  FIBITMAP* bitmap32;
  if(bpp == 32) {
    bitmap32 = bitmap;
  } else {
    bitmap32 = FreeImage_ConvertTo32Bits(bitmap);
  }
  channel = GL_RGBA;
  channel_format = GL_UNSIGNED_BYTE;
  
  width = FreeImage_GetWidth(bitmap32);
  height = FreeImage_GetHeight(bitmap32);

  GLubyte* textureData = FreeImage_GetBits(bitmap32);

  return textureData; 
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
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, click_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // initialize glindings in this context
    glbinding::Binding::initialize();
    watch_gl_errors();
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    //initialize freeimage
    FreeImage_Initialise(true);
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

  quad_program = loadProgram("../../shader/quad.vert", "../../shader/quad.frag", "../../shader/quad.geom");
}
void initQuad() {
  glGenVertexArrays(1, &quad_vao);
  glBindVertexArray(quad_vao);
  glGenBuffers(1, &quad_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);

  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 20, &vertices_quad, GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(sizeof(float) * 3));
}
void genGrid() {
  float W = width/16;
  float H = height/16;

  std::cout << "W: " << W << "; H: " << H << std::endl;

  for(int i = 0; i < W+1; i++) {
    for(int j = 0; j < H+1; j++) {
      float x = -1.0 + 2 * (i/W);
      float y = -1.0 + 2 * (j/H);
	    float z = 0.0;

      // std::cout << "i: " << i << "; j: " << j << std::endl;
      // std::cout << "i/W: " << i/W << "; j/H: " << j/H << std::endl;

      // std::cout << "pos: " << x << "," << y << "," << z << std::endl;

      g_vertices.push_back(glm::vec3(x,y,z));

      float u = i/W;
      float v = j/H;

      // std::cout << "uv: " << u << "," << v << std::endl;

      g_uvs.push_back(glm::vec2(u,v));

    }
  }
  for(int i = 0; i < H; i++) {
    for(int j = 0; j < W; j++) {
      int r1 = j * (H+1);
      int r2 = (j+1) * (H+1);

      g_indices.push_back(glm::uvec3(r1+i, r1+i+1, r2+i+1));
      g_indices.push_back(glm::uvec3(r1+i, r2+i+1, r2+i));

      // std::cout << "Triangle1: " << r1+i << "," << r1+i+1 << "," << r2+i+1 << std::endl;      
      // std::cout << "Triangle2: " << r1+i << "," << r2+i+1 << "," << r2+i << std::endl;
    }
  }   
}
void initGrid() {
  glGenVertexArrays(1, &grid_vao);
  glBindVertexArray(grid_vao);
  glGenBuffers(1, &grid_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, grid_vbo);

  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * g_vertices.size(), g_vertices.data(), GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

  glGenBuffers(1, &grid_uv_vao);
  glBindBuffer(GL_ARRAY_BUFFER, grid_uv_vao);

  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * g_uvs.size(), g_uvs.data(), GL_STATIC_DRAW);

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

  glGenBuffers(1, &grid_ibo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, grid_ibo);

  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(glm::vec3) * g_indices.size(), g_indices.data(), GL_STATIC_DRAW);
}
void initGeometry() {
  std::cout << "Loading model ..." << std::endl;
  loadModel("../../resources/pig_c.obj");

  glGenVertexArrays(1, &model_vao);
  glBindVertexArray(model_vao);

  glGenBuffers(1, &model_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, model_vbo);
  std::cout << "Binding Buffer Data (Vertex) ..." << std::endl;
  glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * m_vertices.size(), m_vertices.data(), GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*) 0);

  glGenBuffers(1, &model_uv_vao);
  glBindBuffer(GL_ARRAY_BUFFER, model_uv_vao);
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

  glActiveTexture(GL_TEXTURE2);
  glGenTextures(1, &depth_handle);
  glBindTexture(GL_TEXTURE_2D, depth_handle);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

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

  // loadTexture(pixels, width, height, channel, channel_format);
  auto textureData = loadTexture(width, height, channel, channel_format);
  std::cout << "Width: " << width << " Height: " << height << std::endl;

  glTexImage2D(GL_TEXTURE_2D, 0, channel, width, height, 0, GL_RGBA, channel_format, textureData);
}
void initFramebufferThread() {
  glActiveTexture(GL_TEXTURE3);
  // glGenRenderbuffers(1, &rb_handle);
  // glBindRenderbuffer(GL_RENDERBUFFER, rb_handle);
  // glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);


  glGenFramebuffers(1, &fbo_handle);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_handle);
  // glBindRenderbuffer(GL_RENDERBUFFER, rb_handle);
  // glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rb_handle);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, quad_handle, 0);
  glActiveTexture(GL_TEXTURE2);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth_handle, 0);

  GLenum draw_buffer[1] = {GL_COLOR_ATTACHMENT0};
  glDrawBuffers(1, draw_buffer);

  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    std::cerr << "Error: Framebuffer object incomplete!" << std::endl;
  }
  glFlush();
}

//RENDERING
/* Render thread */
void render() {
  std::cout << "Rendering ..." << std::endl;

  // glGenQueries(4, query_handle_th);
  // glQueryCounter(query_handle_th[0], GL_TIMESTAMP);  

  glfwMakeContextCurrent(offscreen_context);
  glbinding::Binding::initialize();
  watch_gl_errors();
  glViewport(0, 0, width, height);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  // glBindTexture(GL_TEXTURE_2D, quad_handle);
  glUseProgram(default_program);
  initGeometry();

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex_handle);  

  GLuint location = glGetUniformLocation(default_program, "tex_handle");
  glUniform1i(location, 0);

  updateUniforms(default_program);
  gl_mutex.lock();
  initFramebufferThread();
  gl_mutex.unlock();

 /*  glQueryCounter(query_handle_th[1], GL_TIMESTAMP);
  stopTimerAvailable = 0;
  while (!stopTimerAvailable) {
    glGetQueryObjectiv(query_handle_th[1], GL_QUERY_RESULT_AVAILABLE, &stopTimerAvailable);
  }
  glGetQueryObjectui64v(query_handle_th[0], GL_QUERY_RESULT, &start_time);
  glGetQueryObjectui64v(query_handle_th[1], GL_QUERY_RESULT, &stop_time);
  std::cout << "Init Time (RENDER THREAD): " << (stop_time - start_time)/1000000.0 << " ms" << std::endl; */
  
  glm::fmat4 model_matrix_ = glm::fmat4{1.0f};
  
  // model_matrix_ = glm::rotate(model_matrix_, 1.57f, glm::vec3{0.0f,1.0f,0.0f});
  // 
  location = glGetUniformLocation(default_program, "ModelMatrix");
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  
  for(;;) {
    gl_mutex.lock();
    glUseProgram(default_program);    
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_handle);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindVertexArray(model_vao);

    // model_matrix_ = glm::rotate(model_matrix_, 1.57f, glm::vec3{0.0f,1.0f,0.0f});
    // std::cout << glm::sin(float(glfwGetTime())*0.00001f) << std::endl;
    model_matrix_ = glm::rotate(model_matrix_, glm::sin(float(glfwGetTime())*0.00001f), glm::vec3{0.0f,1.0f,0.0f});
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(model_matrix_));

	  glDrawArrays(GL_TRIANGLES, 0, m_vertices.size());
    // glFlush();
    glFinish();  
    gl_mutex.unlock();
  }
}

/* Warping thread */
void draw() {
  glActiveTexture(GL_TEXTURE3);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glUseProgram(quad_program);
  glBindVertexArray(grid_vao);
  // glBindVertexArray(quad_vao);
  GLuint location = glGetUniformLocation(quad_program, "quad_tex");
  glUniform1i(location, 3);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, depth_handle);
  location = glGetUniformLocation(quad_program, "depth_tex");
  glUniform1i(location, 2);

  // model_matrix = glm::rotate(model_matrix, float(glfwGetTime()) * 0.00001f, glm::vec3{0.0,1.0,0.0});
  location = glGetUniformLocation(quad_program, "ModelMatrix");
  glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(model_matrix));
  // printMatrix(model_matrix, "Model");

  //glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
  glDrawElements(GL_TRIANGLES, g_indices.size()*3, GL_UNSIGNED_INT, 0);
  // glDrawArrays(GL_TRIANGLE_STRIP, 0, 8);
}

int main(int argc, char **argv) {
  initWindow();

  glGenQueries(4, query_handle);
  glQueryCounter(query_handle[0], GL_TIMESTAMP);

  initShaders();
  initTexture();
  initQuad();
  genGrid();
  initGrid();
  initOffscreenWindow();
  std::cout << "Start thread" << std::endl;
  std::thread t = std::thread(render);
  glfwMakeContextCurrent(window);
  glBindTexture(GL_TEXTURE_2D, quad_handle);

  updateUniforms(quad_program );
  glQueryCounter(query_handle[1], GL_TIMESTAMP);
  stopTimerAvailable = 0;
  while (!stopTimerAvailable) {
      glGetQueryObjectiv(query_handle[1], GL_QUERY_RESULT_AVAILABLE, &stopTimerAvailable);
  }
  glGetQueryObjectui64v(query_handle[0], GL_QUERY_RESULT, &start_time);
  glGetQueryObjectui64v(query_handle[1], GL_QUERY_RESULT, &stop_time);

  std::cout << "Initialisation Time (MAIN THREAD): " << (stop_time - start_time)/1000000.0 << " ms" << std::endl;
  while (!glfwWindowShouldClose(window)) {
    // glQueryCounter(query_handle[2], GL_TIMESTAMP);    
    glfwPollEvents();
    draw();
    glfwSwapBuffers(window);
    show_fps(window);
    /* glQueryCounter(query_handle[3], GL_TIMESTAMP);
    stopTimerAvailable = 0;
    while (!stopTimerAvailable) {
      glGetQueryObjectiv(query_handle[3], GL_QUERY_RESULT_AVAILABLE, &stopTimerAvailable);
    }
    glGetQueryObjectui64v(query_handle[2], GL_QUERY_RESULT, &start_time);
    glGetQueryObjectui64v(query_handle[3], GL_QUERY_RESULT, &stop_time);

    std::cout << "Render Time (MAIN THREAD): " << (stop_time - start_time)/1000000.0 << " ms" << std::endl; */
  }
  // glfwSetWindowShouldClose(offscreen_context, 1);
  t.join();
  FreeImage_DeInitialise();
  glfwDestroyWindow(offscreen_context);
  glfwDestroyWindow(window);  
  glfwTerminate();
  return 1;
}
