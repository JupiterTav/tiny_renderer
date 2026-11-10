cmake_minimum_required(VERSION 4.0)

project(tiny_renderer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenGL REQUIRED)

include(FetchContent)

# ---------------------------------------------------------------------------
# Third-party dependencies
# ---------------------------------------------------------------------------

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.5.1
    GIT_SHALLOW    TRUE
)

# Dear ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.93.0
    GIT_SHALLOW    TRUE
)

# tinyobjloader
set(TINYOBJLOADER_BUILD_TEST_LOADER OFF CACHE BOOL "" FORCE)
set(TINYOBJLOADER_BUILD_OBJ_STICHER OFF CACHE BOOL "" FORCE)
set(TINYOBJLOADER_WITH_PYTHON OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    tinyobjloader
    GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
    GIT_TAG        v2.0.0rc13
    GIT_SHALLOW    TRUE
)

# stb (single-file public-domain libraries)
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        d14d46f6b44456eb88beac58cba833a4135d4c89
    GIT_SHALLOW    FALSE
)

FetchContent_MakeAvailable(glfw imgui tinyobjloader stb)

# ---------------------------------------------------------------------------
# GLAD (pre-generated OpenGL 3.3 core loader in external/glad)
# ---------------------------------------------------------------------------

add_library(glad STATIC
    external/glad/src/gl.c
)
target_include_directories(glad PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include
)
target_link_libraries(glad PUBLIC OpenGL::GL)

# ---------------------------------------------------------------------------
# Dear ImGui
# ---------------------------------------------------------------------------

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw glad)

# ---------------------------------------------------------------------------
# stb headers
# ---------------------------------------------------------------------------

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})

# ---------------------------------------------------------------------------
# Application
# ---------------------------------------------------------------------------

add_executable(tiny_renderer
    src/main.cpp
    src/Shader.cpp
    src/Mesh.cpp
    src/Model.cpp
    src/Camera.cpp
    src/Frame.cpp
    src/stb_impl.cpp
)
target_include_directories(tiny_renderer PRIVATE include)
target_link_libraries(tiny_renderer PRIVATE
    glfw
    glad
    imgui
    tinyobjloader
    stb
    OpenGL::GL
)
