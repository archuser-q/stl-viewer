#include <GLFW/glfw3.h>
#include <iostream>

int main() {
    if (!glfwInit()) {
        std::cerr << "Khong khoi tao duoc GLFW\n";
        return 1;
    }

    GLFWwindow* window = glfwCreateWindow(1280, 720, "STL Viewer", nullptr, nullptr);
    if (!window) {
        std::cerr << "Khong tao duoc cua so\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Giới hạn FPS theo màn hình

    // Vòng lặp chính: chạy đến khi người dùng đóng cửa sổ
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.15f, 0.17f, 0.20f, 1.0f); // Chọn màu nền
        glClear(GL_COLOR_BUFFER_BIT);            // Tô nền

        glfwSwapBuffers(window); // Đưa khung hình lên màn hình
        glfwPollEvents();        // Nhận sự kiện chuột, bàn phím
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}