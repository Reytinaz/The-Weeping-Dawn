#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include <string>
#include <vector>
#include <memory>
#include "vector3.hpp"
#include "texture.hpp"

class Material {
public:
    std::string name;
    float minHeight;
    float maxHeight;
    float blendStrength;
    float noiseFactor;
    Vector3 color;
    std::string texturePath;
    std::shared_ptr<Texture> texture;
    bool hasTexture;

    float uvScale = 10.0f;
    float roughness = 0.5f;
    float metallic = 0.0f;
    float emissive = 0.0f;

    Material() :
        name("default"),
        minHeight(0),
        maxHeight(100),
        blendStrength(0.2f),
        noiseFactor(0.3f),
        color(1, 1, 1),
        hasTexture(false),
        texture(nullptr),
        uvScale(10.0f) {
    }

    Material(const std::string& name, float minH, float maxH,
        const std::string& texPath, float blend = 0.2f, float noiseF = 0.3f, float uvS = 10.0f) :
        name(name),
        minHeight(minH),
        maxHeight(maxH),
        blendStrength(blend),
        noiseFactor(noiseF),
        texturePath(texPath),
        hasTexture(true),
        texture(nullptr),
        uvScale(uvS) {
    }

    Material(const std::string& name, float minH, float maxH,
        const Vector3& col, float blend = 0.2f, float noiseF = 0.3f, float uvS = 10.0f) :
        name(name),
        minHeight(minH),
        maxHeight(maxH),
        blendStrength(blend),
        noiseFactor(noiseF),
        color(col),
        hasTexture(false),
        texture(nullptr),
        uvScale(uvS) {
    }

    Material(const Material& other) {
        name = other.name;
        minHeight = other.minHeight;
        maxHeight = other.maxHeight;
        blendStrength = other.blendStrength;
        noiseFactor = other.noiseFactor;
        color = other.color;
        texturePath = other.texturePath;
        hasTexture = other.hasTexture;
        texture = other.texture;
        uvScale = other.uvScale;
    }

    bool loadTexture() {
        if (!hasTexture || texturePath.empty()) return false;
        texture = std::make_shared<Texture>();
        if (texture->loadFromFile(texturePath, true)) {
            return true;
        }
        std::cerr << "Failed to load texture for material '" << name << "': " << texturePath << std::endl;
        hasTexture = false;
        return false;
    }

    unsigned int getTextureId() const {
        return texture ? texture->getId() : 0;
    }
};

class MaterialSet {
public:
    std::string name;
    std::vector<std::shared_ptr<Material>> materials;

    MaterialSet(const std::string& name = "default") : name(name) {}
    MaterialSet(const MaterialSet& other) {
        name = other.name + "_copy";
        for (const auto& mat : other.materials) {
            auto copy = std::make_shared<Material>(*mat);
            materials.push_back(copy);
        }
    }

    void addMaterial(std::shared_ptr<Material> material) {
        materials.push_back(material);
    }

    template<typename... Args>
    std::shared_ptr<Material> createMaterial(Args&&... args) {
        auto material = std::make_shared<Material>(std::forward<Args>(args)...);
        materials.push_back(material);
        return material;
    }

    std::shared_ptr<Material> getMaterialAtHeight(float height, float noiseVal = 0.5f) const {
        if (materials.empty()) return nullptr;

        for (const auto& mat : materials) {
            if (height > mat->minHeight && height < mat->maxHeight) {
                return mat;
            }
        }
        for (const auto& mat : materials) {
            if (height >= mat->minHeight && height <= mat->maxHeight) {
                return mat;
            }
        }
        float minDist = std::numeric_limits<float>::max();
        std::shared_ptr<Material> closest = materials[0];

        for (const auto& mat : materials) {
            float distToMin = std::abs(height - mat->minHeight);
            float distToMax = std::abs(height - mat->maxHeight);
            float dist = std::min(distToMin, distToMax);

            if (dist < minDist) {
                minDist = dist;
                closest = mat;
            }
        }

        return closest;
    }

    int getMaterialIndexAtHeight(float height, float noiseVal = 0.5f) const {
        for (size_t i = 0; i < materials.size(); ++i) {
            if (height < materials[i]->maxHeight) {
                return i;
            }
        }
        return materials.size() - 1;
    }

    void loadAllTextures() {
        for (auto& mat : materials) {
            mat->loadTexture();
        }
    }

    size_t size() const { return materials.size(); }
    void clear() { materials.clear(); }
};

namespace MaterialPresets {   
    inline std::shared_ptr<MaterialSet> createOverworldSet() {
        auto set = std::make_shared<MaterialSet>("overworld");

        set->createMaterial("rockBottom", -100.0f, -30.0f, "assets/images/terrain/rock.jpg", 0.1f, 0.2f, 15.0f);
        set->createMaterial("ground", -30.0f, -15.0f, "assets/images/terrain/ground.jpg", 0.2f, 0.3f, 12.0f);
        set->createMaterial("grass", -15.0f, 15.0f, "assets/images/terrain/grass.jpg", 0.2f, 0.3f, 14.0f);
        set->createMaterial("rockTop", 15.0f, 25.0f, "assets/images/terrain/rock.jpg", 0.2f, 0.3f, 15.0f);
        set->createMaterial("snow", 25.0f, 100.0f, "assets/images/terrain/snow.jpg", 0.2f, 0.3f, 20.0f);

        return set;
    }
}

#endif