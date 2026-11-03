#include "stl_loader.h"
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <limits>
#include <stdexcept>

Mesh loadBinaryStl(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) throw std::runtime_error("Khong mo duoc file: " + path);

    const auto fileSize = static_cast<std::uint64_t>(file.tellg());
    if (fileSize < 84) throw std::runtime_error("File STL qua ngan");

    file.seekg(80);
    std::uint32_t triangleCount = 0;
    file.read(reinterpret_cast<char*>(&triangleCount), sizeof(triangleCount));

    if (fileSize != 84ULL + 50ULL * triangleCount) {
        throw std::runtime_error("Khong phai STL binary hop le");
    }

    Mesh mesh;
    mesh.vertices.reserve(triangleCount * 3ULL);

    constexpr float inf = std::numeric_limits<float>::max();
    glm::vec3 minPoint(inf);
    glm::vec3 maxPoint(-inf);

    for (std::uint32_t i = 0; i < triangleCount; ++i) {
        float data[12];
        std::uint16_t attribute = 0;
        file.read(reinterpret_cast<char*>(data), sizeof(data));
        file.read(reinterpret_cast<char*>(&attribute), sizeof(attribute));

        glm::vec3 a(data[3], data[4], data[5]);
        glm::vec3 b(data[6], data[7], data[8]);
        glm::vec3 c(data[9], data[10], data[11]);

        glm::vec3 normal = glm::cross(b - a, c - a);
        if (glm::length(normal) > 0.0f) normal = glm::normalize(normal);

        for (const glm::vec3& point : {a, b, c}) {
            mesh.vertices.push_back({point, normal});
            minPoint = glm::min(minPoint, point);
            maxPoint = glm::max(maxPoint, point);
        }
    }

    if (mesh.vertices.empty()) throw std::runtime_error("File STL khong co tam giac");

    mesh.center = (minPoint + maxPoint) * 0.5f;
    mesh.radius = std::max(glm::length(maxPoint - minPoint) * 0.5f, 0.0001f);
    return mesh;
}