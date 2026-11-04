cmake_minimum_required(VERSION 3.20)
project(StlViewer LANGUAGES C CXX)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4)
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1)
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.5)
FetchContent_Declare(nfd
    GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended.git
    GIT_TAG v1.2.1)
FetchContent_Declare(miniz
    GIT_REPOSITORY https://github.com/richgel999/miniz.git
    GIT_TAG 3.0.2)
FetchContent_Declare(pugixml
    GIT_REPOSITORY https://github.com/zeux/pugixml.git
    GIT_TAG v1.14)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_FUZZERS OFF CACHE BOOL "" FORCE)
set(INSTALL_PROJECT OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw glm imgui nfd miniz pugixml)

find_package(OpenGL REQUIRED)

add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
target_include_directories(imgui_lib PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui_lib PUBLIC glfw OpenGL::GL)

set(FONT_FILE ${PROJECT_SOURCE_DIR}/assets/font.ttf)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${FONT_FILE})
file(READ ${FONT_FILE} FONT_HEX HEX)
string(LENGTH "${FONT_HEX}" FONT_HEX_LENGTH)
math(EXPR FONT_SIZE "${FONT_HEX_LENGTH} / 2")
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," FONT_ARRAY "${FONT_HEX}")
file(WRITE ${PROJECT_BINARY_DIR}/font_data.cpp
"extern const unsigned char kFontData[];
extern const unsigned int kFontSize;
const unsigned char kFontData[] = {${FONT_ARRAY}};
const unsigned int kFontSize = ${FONT_SIZE};
")

add_executable(stl-viewer src/main.cpp src/stl_loader.cpp src/threemf_loader.cpp
    ${PROJECT_BINARY_DIR}/font_data.cpp)
target_link_libraries(stl-viewer PRIVATE glfw glm::glm OpenGL::GL imgui_lib nfd miniz pugixml::pugixml)
target_include_directories(stl-viewer PRIVATE ${glfw_SOURCE_DIR}/deps)