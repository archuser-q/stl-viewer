#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
};

struct Mesh {
    std::vector<Vertex> vertices;
    glm::vec3 center{0.0f};
    float radius = 1.0f;
};

Mesh loadBinaryStl(const std::string& path);