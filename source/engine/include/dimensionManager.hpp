#ifndef DIMENSION_MANAGEMENT
#define DIMENSION_MANAGEMENT

#include "chunkManager.hpp"
#include "object3d.hpp"
#include "skybox.hpp"
#include "physics.hpp"
#include "unordered_map"
#include "map"
#include "biome.hpp"

struct CelestialBody {
    std::string name;
    std::string texturePath;
    std::shared_ptr<SourceLight> sourceLight;
    unsigned int textureId;
    float size;
    float softness;
};

class Dimension {
public:
    Dimension(const std::string& name);
    ~Dimension();

    std::string name;
    bool active = false;
    Physics* physics;
    ChunkManager* chunkManager;
    std::vector<std::shared_ptr<CelestialBody>> celestialBodies;
    std::shared_ptr<Skybox> skybox;
    std::vector<std::string> skyboxFaces;
    Vector3 spawnPoint;
    float noiseParams[3];
    std::map<int64_t, std::shared_ptr<Terrain>> chunks;

    std::vector<std::shared_ptr<Biome>> biomes;
    float biomeScale = 0.002f;

    mutable std::shared_ptr<MaterialSet> mergedMaterialSet;
    mutable std::unordered_map<std::string, int> mergedMaterialIndexMap;
    mutable std::mutex mergedMutex;

    float fogStart = 0.5f;
    float timeOfDay = 0.5f;
    float daySpeed = 0.02f;
    bool dayNightCycle = true;
    bool isDay = true;

    bool hasClouds = true;
    float cloudsDensity = 1.0f;
    float cloudsCover = 0.5f;
    float cloudsScale = 0.00003f;
    Vector3 cloudsWind{ 0.01f, 0.0f, 0.005f };

    float sunGlowIntensity = 1.0f;
    Vector3 dawnSkyColor = Vector3(1.0f, 0.5f, 0.2f);
    Vector3 skyColor = Vector3(1.2f, 1.2f, 1.2f);
    Vector3 nightSkyColor = Vector3(0.01f, 0.01f, 0.04f);
    std::string mainLightSource = "Sun";

    std::shared_ptr<Biome> getBiomeAt(float worldX, float worldZ, std::shared_ptr<PerlinNoise> noise) {
        if (biomes.empty()) return nullptr;

        float biomeNoise = noise->noise(worldX * biomeScale, worldZ * biomeScale);
        float t = (biomeNoise + 1.0f) * 0.5f;

        int index = static_cast<int>(t * biomes.size());
        index = std::min(index, static_cast<int>(biomes.size()) - 1);

        return biomes[index];
    }
    void ensureMergedMaterialCache() const {
        std::lock_guard<std::mutex> lock(mergedMutex);
        if (mergedMaterialSet) return;
        mergedMaterialSet = std::make_shared<MaterialSet>();
        mergedMaterialSet->name = "merged_biomes";
        mergedMaterialIndexMap.clear();
        for (const auto& b : biomes) {
            if (!b || !b->materialSet) continue;
            for (const auto& mat : b->materialSet->materials) {
                std::string key = b->name + "|" + mat->name + "|" + mat->texturePath;
                if (mergedMaterialIndexMap.find(key) == mergedMaterialIndexMap.end()) {
                    auto copy = std::make_shared<Material>(*mat);
                    mergedMaterialSet->materials.push_back(copy);
                    int idx = static_cast<int>(mergedMaterialSet->materials.size() - 1);
                    mergedMaterialIndexMap[key] = idx;
                }
            }
        }
    }
    std::vector<BiomeBlend> getBiomesAt(float worldX, float worldZ,
        std::shared_ptr<PerlinNoise> noise,
        float blendRadius) {
        std::vector<BiomeBlend> result;

        if (biomes.empty()) {
            return result;
        }
        std::map<std::shared_ptr<Biome>, float> weights;

        float actualRadius = std::max(1.0f, blendRadius);
        int samples = 7;
        float step = actualRadius * 2.0f / samples;

        for (int i = 0; i <= samples; ++i) {
            for (int j = 0; j <= samples; ++j) {
                float x = worldX - actualRadius + i * step;
                float z = worldZ - actualRadius + j * step;

                auto biome = getBiomeAt(x, z, noise);
                if (biome) {
                    float dx = x - worldX;
                    float dz = z - worldZ;
                    float dist = sqrt(dx * dx + dz * dz);
                    if (dist >= actualRadius) continue;
                    float t = dist / actualRadius;
                    float smooth = 1.0f - (t * t * (3.0f - 2.0f * t));
                    if (smooth > 0.001f) {
                        weights[biome] += smooth;
                    }
                }
            }
        }

        if (weights.empty()) {
            auto currentBiome = getBiomeAt(worldX, worldZ, noise);
            if (currentBiome) weights[currentBiome] = 1.0f;
        }

        float totalWeight = 0;
        for (auto& [biome, weight] : weights) {
            totalWeight += weight;
        }

        if (totalWeight > 0) {
            for (auto& [biome, weight] : weights) {
                result.push_back({ biome, weight / totalWeight });
            }
        }

        return result;
    }

    std::shared_ptr<Terrain> getChunk(int64_t id) {
        auto it = chunks.find(id);
        return it != chunks.end() ? it->second : nullptr;
    }

    void addChunk(std::shared_ptr<Terrain> chunk) {
        chunks[chunk->id] = chunk;
    }

    void removeChunk(int64_t id) {
        chunks.erase(id);
    }

    void renderSkybox(std::shared_ptr<Camera> camera) const;
};

#endif