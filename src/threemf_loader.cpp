#include "threemf_loader.h"
#include <miniz.h>
#include <pugixml.hpp>
#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace {

struct Component {
    std::string path; 
    int objectId = 0;
    glm::mat4 transform{1.0f};
};

struct ObjectData {
    std::vector<glm::vec3> vertices;
    std::vector<std::array<std::uint32_t, 3>> triangles;
    std::vector<Component> components;
};

using ObjectMap = std::map<int, ObjectData>;

glm::mat4 parseTransform(const char* text) {
    float v[12];
    std::istringstream stream(text);
    for (float& value : v) {
        if (!(stream >> value)) return glm::mat4(1.0f);
    }

    glm::mat4 m(1.0f);
    m[0] = glm::vec4(v[0], v[1], v[2], 0.0f);
    m[1] = glm::vec4(v[3], v[4], v[5], 0.0f);
    m[2] = glm::vec4(v[6], v[7], v[8], 0.0f);
    m[3] = glm::vec4(v[9], v[10], v[11], 1.0f);
    return m;
}

ObjectMap parseObjects(const pugi::xml_node& model) {
    ObjectMap objects;

    for (pugi::xml_node node : model.child("resources").children("object")) {
        ObjectData object;
        pugi::xml_node mesh = node.child("mesh");

        for (pugi::xml_node v : mesh.child("vertices").children("vertex")) {
            object.vertices.emplace_back(v.attribute("x").as_float(),
                                         v.attribute("y").as_float(),
                                         v.attribute("z").as_float());
        }

        for (pugi::xml_node t : mesh.child("triangles").children("triangle")) {
            object.triangles.push_back({t.attribute("v1").as_uint(),
                                        t.attribute("v2").as_uint(),
                                        t.attribute("v3").as_uint()});
        }

        for (pugi::xml_node c : node.child("components").children("component")) {
            Component component;
            component.path = c.attribute("p:path").as_string();
            component.objectId = c.attribute("objectid").as_int();
            component.transform = parseTransform(c.attribute("transform").as_string());
            object.components.push_back(component);
        }

        objects[node.attribute("id").as_int()] = std::move(object);
    }
    return objects;
}

class Package {
public:
    explicit Package(const std::string& path) {
        if (!mz_zip_reader_init_file(&zip_, path.c_str(), 0)) {
            throw std::runtime_error("Khong mo duoc file 3MF: " + path);
        }
    }
    ~Package() { mz_zip_reader_end(&zip_); }
    Package(const Package&) = delete;
    Package& operator=(const Package&) = delete;

    std::string mainModelPath() {
        if (mz_zip_reader_locate_file(&zip_, "3D/3dmodel.model", nullptr, 0) >= 0) {
            return "3D/3dmodel.model";
        }
        for (mz_uint i = 0; i < mz_zip_reader_get_num_files(&zip_); ++i) {
            char name[512];
            if (!mz_zip_reader_get_filename(&zip_, i, name, sizeof(name))) continue;
            const std::string entry = name;
            if (entry.size() > 6 && entry.compare(entry.size() - 6, 6, ".model") == 0) {
                return entry;
            }
        }
        throw std::runtime_error("Khong tim thay model 3D trong file 3MF");
    }

    void readModel(const std::string& entry, pugi::xml_document& doc) {
        const int index = mz_zip_reader_locate_file(&zip_, entry.c_str(), nullptr, 0);
        if (index < 0) throw std::runtime_error("Thieu file trong 3MF: " + entry);

        std::size_t size = 0;
        void* data = mz_zip_reader_extract_to_heap(&zip_, static_cast<mz_uint>(index), &size, 0);
        if (!data) throw std::runtime_error("Khong giai nen duoc: " + entry);

        const pugi::xml_parse_result result = doc.load_buffer(data, size);
        mz_free(data);
        if (!result) throw std::runtime_error("File XML trong 3MF bi loi: " + entry);
    }

    const ObjectMap& objects(std::string entry) {
        if (!entry.empty() && entry[0] == '/') entry.erase(0, 1);

        auto it = cache_.find(entry);
        if (it == cache_.end()) {
            pugi::xml_document doc;
            readModel(entry, doc);
            it = cache_.emplace(entry, parseObjects(doc.child("model"))).first;
        }
        return it->second;
    }

private:
    mz_zip_archive zip_{};
    std::map<std::string, ObjectMap> cache_;
};

struct Builder {
    Package& package;
    Mesh mesh;

    void add(const ObjectMap& objects, int id, const glm::mat4& transform, int depth) {
        if (depth > 16) throw std::runtime_error("File 3MF long nhau qua sau");

        const auto found = objects.find(id);
        if (found == objects.end()) return;
        const ObjectData& object = found->second;

        const bool flip = glm::determinant(glm::mat3(transform)) < 0.0f;
        const std::size_t count = object.vertices.size();

        for (const auto& tri : object.triangles) {
            if (tri[0] >= count || tri[1] >= count || tri[2] >= count) {
                throw std::runtime_error("Chi so dinh trong 3MF khong hop le");
            }

            glm::vec3 a = glm::vec3(transform * glm::vec4(object.vertices[tri[0]], 1.0f));
            glm::vec3 b = glm::vec3(transform * glm::vec4(object.vertices[tri[1]], 1.0f));
            glm::vec3 c = glm::vec3(transform * glm::vec4(object.vertices[tri[2]], 1.0f));
            if (flip) std::swap(b, c);

            glm::vec3 normal = glm::cross(b - a, c - a);
            if (glm::length(normal) > 0.0f) normal = glm::normalize(normal);

            mesh.vertices.push_back({a, normal});
            mesh.vertices.push_back({b, normal});
            mesh.vertices.push_back({c, normal});
        }

        for (const Component& component : object.components) {
            const ObjectMap& target =
                component.path.empty() ? objects : package.objects(component.path);
            add(target, component.objectId, transform * component.transform, depth + 1);
        }
    }
};

}

Mesh load3mf(const std::string& path) {
    Package package(path);

    const std::string mainPath = package.mainModelPath();
    pugi::xml_document doc;
    package.readModel(mainPath, doc);

    const pugi::xml_node model = doc.child("model");
    if (!model) throw std::runtime_error("File 3MF khong co the <model>");

    const ObjectMap objects = parseObjects(model);
    Builder builder{package};

    bool hasItems = false;
    for (pugi::xml_node item : model.child("build").children("item")) {
        hasItems = true;
        builder.add(objects, item.attribute("objectid").as_int(),
                    parseTransform(item.attribute("transform").as_string()), 0);
    }
    if (!hasItems) {
        for (const auto& entry : objects) builder.add(objects, entry.first, glm::mat4(1.0f), 0);
    }

    Mesh mesh = std::move(builder.mesh);
    if (mesh.vertices.empty()) throw std::runtime_error("File 3MF khong co tam giac");

    constexpr float inf = std::numeric_limits<float>::max();
    glm::vec3 minPoint(inf);
    glm::vec3 maxPoint(-inf);
    for (const Vertex& vertex : mesh.vertices) {
        minPoint = glm::min(minPoint, vertex.position);
        maxPoint = glm::max(maxPoint, vertex.position);
    }

    mesh.center = (minPoint + maxPoint) * 0.5f;
    mesh.radius = std::max(glm::length(maxPoint - minPoint) * 0.5f, 0.0001f);
    return mesh;
}