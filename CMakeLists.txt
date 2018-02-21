cmake_minimum_required (VERSION 3.1)
project (OpenGL-Threading)

set(CMAKE_CXX_STANDARD 11)

include_directories(SYSTEM externals/glm)

option(GLFW_BUILD_DOCS     OFF)
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS    OFF)
option(GLFW_INSTALL        OFF)
add_subdirectory(externals/glfw)

include_directories(externals/glfw/include)
add_subdirectory(externals/glbinding)

include_directories(externals/stb)

add_subdirectory(externals/tinyobjloader)

include_directories(externals/tinyobjloader)
file(GLOB TINYOBJLOADER_SOURCES "externals/tinyobjloader/*.h")


file(GLOB TRACKING_SRC
  # "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp"
)

file(GLOB SINGLE_THREAD 
  "${CMAKE_CURRENT_SOURCE_DIR}/src/singlethread.cpp"
)

add_executable(OpenGL-Threading ${TRACKING_SRC})

add_executable(Singlethread ${SINGLE_THREAD})


target_link_libraries(OpenGL-Threading glbinding glfw ${GLFW_LIBRARIES})
target_link_libraries(Singlethread glbinding glfw ${GLFW_LIBRARIES})
