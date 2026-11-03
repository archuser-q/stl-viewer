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
    auto* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));
    camera->distance *= (yOffset > 0.0) ? 0.9f : 1.1f;
    camera->distance = glm::clamp(camera->distance, 1.2f, 10.0f);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Cach dung: stl-viewer <file.stl>\n";
        return 1;
    }

    Mesh mesh;
    try {
        mesh = loadBinaryStl(argv[1]);
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return 1;
    }

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
    glBufferData(GL_ARRAY_BUFFER,
            mesh.vertices.size() * sizeof(Vertex),
            mesh.vertices.data(),
            GL_STATIC_DRAW);

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

    while (!glfwWindowShouldClose(window)) {
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);

        glClearColor(0.15f, 0.17f, 0.20f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float aspect = height > 0 ? static_cast<float>(width) / height : 1.0f;
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        glm::mat4 view = glm::translate(glm::mat4(1.0f),
                                        glm::vec3(camera.panX, camera.panY, -camera.distance));

        float scale = 1.0f / mesh.radius;
        glm::mat4 model(1.0f);
        model = glm::rotate(model, camera.pitch, glm::vec3(1, 0, 0));
        model = glm::rotate(model, camera.yaw, glm::vec3(0, 1, 0));
        model = glm::scale(model, glm::vec3(scale));
        model = glm::translate(model, -mesh.center);

        glm::mat4 viewProjection = projection * view;

        glUseProgram(program);
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewProjectionLocation, 1, GL_FALSE, glm::value_ptr(viewProjection));
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mesh.vertices.size()));

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteProgram(program);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}