#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <iostream>
#include "stl_loader.h"
#include <cstddef>
#include <exception>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <nfd.h>
#include <string>
#include <utility>
#include <filesystem>
#include "threemf_loader.h"
#include <algorithm>
#include <cctype>

const char* vertexSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
uniform mat4 uModel;
uniform mat4 uViewProjection;
out vec3 vNormal;
void main() {
    vNormal = mat3(uModel) * aNormal;
    gl_Position = uViewProjection * uModel * vec4(aPos, 1.0);
}
)";

const char* fragmentSource = R"(
#version 330 core
in vec3 vNormal;
out vec4 FragColor;
void main() {
    vec3 lightDirection = normalize(vec3(0.4, 0.7, 1.0));
    float diffuse = max(dot(normalize(vNormal), lightDirection), 0.0);
    vec3 color = vec3(0.30, 0.65, 0.95) * (0.25 + 0.75 * diffuse);
    FragColor = vec4(color, 1.0);
}
)";

GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << "Loi shader:\n" << log << "\n";
    }
    return shader;
}

GLuint createProgram() {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::cerr << "Loi link program:\n" << log << "\n";
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

struct Camera {
    float yaw = 0.6f;
    float pitch = 0.4f;
    float distance = 3.0f;
    bool dragging = false;
    double lastX = 0.0;
    double lastY = 0.0;
    bool panning = false;
    float panX = 0.0f;
    float panY = 0.0f;
};

void mouseButtonCallback(GLFWwindow* window, int button, int action, int) {
    auto* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));
    if (action == GLFW_PRESS && ImGui::GetIO().WantCaptureMouse) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        camera->dragging = (action == GLFW_PRESS);
    } else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        camera->panning = (action == GLFW_PRESS);
    }

    glfwGetCursorPos(window, &camera->lastX, &camera->lastY);
}

void cursorPositionCallback(GLFWwindow* window, double x, double y) {
    auto* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));

    float dx = static_cast<float>(x - camera->lastX);
    float dy = static_cast<float>(y - camera->lastY);
    camera->lastX = x;
    camera->lastY = y;

    if (camera->dragging) {
        constexpr float sensitivity = 0.01f;
        camera->yaw += dx * sensitivity;
        camera->pitch += dy * sensitivity;
        camera->pitch = glm::clamp(camera->pitch, -1.5f, 1.5f);
    }

    if (camera->panning) {
        int width = 0;
        int height = 0;
        glfwGetWindowSize(window, &width, &height);
        if (height <= 0) return;

        float worldPerPixel = 2.0f * camera->distance *
                              glm::tan(glm::radians(45.0f) * 0.5f) / height;
        camera->panX += dx * worldPerPixel;
        camera->panY -= dy * worldPerPixel;
    }
}

void scrollCallback(GLFWwindow* window, double, double yOffset) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    auto* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));
    camera->distance *= (yOffset > 0.0) ? 0.9f : 1.1f;
    camera->distance = glm::clamp(camera->distance, 1.2f, 10.0f);
}

std::string chooseStlFile() {
    nfdu8char_t* outPath = nullptr;
    nfdu8filteritem_t filters[1] = {{"3D models", "stl,STL,3mf,3MF"}};

    std::string path;
    if (NFD_OpenDialogU8(&outPath, filters, 1, nullptr) == NFD_OKAY) {
        path = outPath;
        NFD_FreePathU8(outPath);
    }
    return path;
}

void applyUiStyle() {
    ImGuiIO& io = ImGui::GetIO();
    const char* fontPath = "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf";
    if (std::filesystem::exists(fontPath)) {
        io.Fonts->AddFontFromFileTTF(fontPath, 18.0f);
    }

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.PopupRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.WindowBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;
    style.FramePadding = ImVec2(10, 7);
    style.ItemSpacing = ImVec2(10, 8);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.09f, 0.10f, 0.12f, 1.0f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.13f, 0.14f, 0.17f, 0.98f);
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.42f, 0.70f, 0.70f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.24f, 0.50f, 0.82f, 0.90f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.24f, 0.50f, 0.82f, 1.0f);
}

Mesh loadModelFile(const std::string& path) {
    std::string extension = std::filesystem::path(path).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (extension == ".3mf") return load3mf(path);
    return loadBinaryStl(path);
}

int main(int argc, char* argv[]) {
    Mesh mesh;
    bool hasMesh = false;
    std::string errorMessage;

    if (!glfwInit()) {
        std::cerr << "Khong khoi tao duoc GLFW\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "STL Viewer", nullptr, nullptr);
    if (!window) {
        std::cerr << "Khong tao duoc cua so\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cerr << "Khong tai duoc OpenGL\n";
        glfwTerminate();
        return 1;
    }

    if (NFD_Init() != NFD_OKAY) {
        std::cerr << "Khong khoi tao duoc hop thoai file\n";
        glfwTerminate();
        return 1;
    }

    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f,
    };

    GLuint vao = 0;
    GLuint vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);

    GLuint program = createProgram();
    glEnable(GL_DEPTH_TEST);
    GLint modelLocation = glGetUniformLocation(program, "uModel");
    GLint viewProjectionLocation = glGetUniformLocation(program, "uViewProjection");

    Camera camera;
    glfwSetWindowUserPointer(window, &camera);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPositionCallback);
    glfwSetScrollCallback(window, scrollCallback);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    applyUiStyle();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    auto openFile = [&](const std::string& path) {
        try {
            Mesh loaded = loadModelFile(path);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER,
                         loaded.vertices.size() * sizeof(Vertex),
                         loaded.vertices.data(),
                         GL_STATIC_DRAW);
            mesh = std::move(loaded);
            hasMesh = true;
            camera = Camera{};
            errorMessage.clear();
        } catch (const std::exception& error) {
            errorMessage = error.what();
        }
    };

    if (argc >= 2) openFile(argv[1]);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        const ImGuiWindowFlags panelFlags = ImGuiWindowFlags_NoDecoration |
                                            ImGuiWindowFlags_NoMove |
                                            ImGuiWindowFlags_NoSavedSettings;

        bool requestOpen = false;
        ImGuiIO& io = ImGui::GetIO();

        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O, false)) requestOpen = true;
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Q, false)) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Open File...", "Ctrl+O")) requestOpen = true;

                if (ImGui::MenuItem("Close File", nullptr, false, hasMesh)) {
                    mesh = Mesh{};
                    hasMesh = false;
                    errorMessage.clear();
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Exit", "Ctrl+Q")) {
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        if (!hasMesh) {
            const char* hint = "Press Ctrl+O to open an STL or 3MF file";
            ImVec2 textSize = ImGui::CalcTextSize(hint);
            ImVec2 center = viewport->GetCenter();
            ImGui::GetBackgroundDrawList()->AddText(
                ImVec2(center.x - textSize.x * 0.5f, center.y - textSize.y * 0.5f),
                IM_COL32(150, 158, 170, 255),
                hint);
        }
        
        if (requestOpen) {
            std::string path = chooseStlFile();
            if (!path.empty()) openFile(path);
        }

        if (!errorMessage.empty()) {
            ImGui::SetNextWindowPos(ImVec2(16, viewport->Size.y - 56));
            ImGui::Begin("Error", nullptr, panelFlags | ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::TextUnformatted(errorMessage.c_str());
            ImGui::End();
        }

        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(0.11f, 0.12f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (hasMesh) {
            float aspect = height > 0 ? static_cast<float>(width) / height : 1.0f;
            glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
            glm::mat4 view = glm::translate(glm::mat4(1.0f),
                                            glm::vec3(camera.panX, camera.panY, -camera.distance));

            float scale = 1.0f / mesh.radius;
            glm::mat4 model(1.0f);
            model = glm::rotate(model, camera.pitch, glm::vec3(1, 0, 0));
            model = glm::rotate(model, camera.yaw, glm::vec3(0, 1, 0));
            model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1, 0, 0));
            model = glm::scale(model, glm::vec3(scale));
            model = glm::translate(model, -mesh.center);

            glm::mat4 viewProjection = projection * view;

            glUseProgram(program);
            glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix4fv(viewProjectionLocation, 1, GL_FALSE, glm::value_ptr(viewProjection));
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mesh.vertices.size()));
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    NFD_Quit();

    glDeleteProgram(program);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}