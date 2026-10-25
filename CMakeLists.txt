cmake_minimum_required(VERSION 3.24)
project(OpenGL)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)


# GLFW
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.4
        GIT_SHALLOW    TRUE
        FIND_PACKAGE_ARGS 3.3 CONFIG
)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)


# OpenGL
set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)


# GLM
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG        1.0.3
        GIT_SHALLOW    TRUE
        FIND_PACKAGE_ARGS 1.0 CONFIG
)
FetchContent_MakeAvailable(glm)


# Assimp
FetchContent_Declare(
        assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG        v5.4.3
        GIT_SHALLOW    TRUE
        FIND_PACKAGE_ARGS 5.4 CONFIG
)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(assimp)

target_compile_options(assimp PRIVATE -Wno-error)


# GLAD
FetchContent_Declare(
        glad
        GIT_REPOSITORY https://github.com/Dav1dde/glad.git
        GIT_TAG        v2.0.6
        GIT_SHALLOW    TRUE
        SOURCE_SUBDIR  cmake
)
FetchContent_MakeAvailable(glad)

glad_add_library(glad_gl_core_33 STATIC REPRODUCIBLE LOADER API gl:core=3.3)


# stb_image
FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG        master
        GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(stb)

add_library(stb_image STATIC src/stb_image_impl.cpp)
target_include_directories(stb_image PUBLIC ${stb_SOURCE_DIR})


# nlohmann_json
FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG        v3.11.3
        GIT_SHALLOW    TRUE
        FIND_PACKAGE_ARGS 3.12 CONFIG
)
FetchContent_MakeAvailable(nlohmann_json)


# Dear ImGui
FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        v1.92.6
        GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
        ${imgui_SOURCE_DIR}/misc/cpp
)
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)
target_link_libraries(imgui PUBLIC glad_gl_core_33 glfw)

# Main
file(GLOB_RECURSE OPENGL_SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_executable(${PROJECT_NAME} ${OPENGL_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE src)
target_compile_definitions(${PROJECT_NAME} PRIVATE GLFW_INCLUDE_NONE)
target_link_libraries(${PROJECT_NAME} PRIVATE
        glfw
        OpenGL::GL
        glm::glm
        assimp::assimp
        glad_gl_core_33
        stb_image
        nlohmann_json::nlohmann_json
        imgui
        ${CMAKE_DL_LIBS}
)

# Assets
add_custom_target(Assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
)
add_dependencies(${PROJECT_NAME} Assets)