#ifndef TERRAIN_HPP
#define TERRAIN_HPP

#include "texture.hpp"
#include "Object3D.hpp"
#include "random"
#include "numeric"
#include "map"
#include "material.hpp"

class Biome;
class Dimension;

struct Structure {
    std::string structName;
    std::string modelPath;
    Vector3 position;
    Vector3 scale;
    Vector3 modelScale;
    Vector3 rotation;
    bool isStatic = true;
    int type = 1;
};

struct MaterialWeight {
    float matBL = 0.0f;  // материал для bottom-left угла
    float matBR = 0.0f;  // материал для bottom-right угла
    float matTL = 0.0f;  // материал для top-left угла
    float matTR = 0.0f;  // материал для top-right угла
};

class PerlinNoise {
public:
    PerlinNoise(unsigned int seed = 123456);
    float noise(float x, float y) const;

    float ridgedNoise(float x, float y, float roughness = 0.9f) const;
    float billowNoise(float x, float y) const;
    float clampedNoise(float x, float y, float low, float high) const;
private:
    std::vector<int> p;
    static float fade(float t);
    static float lerp(float t, float a, float b);
    static float grad(int hash, float x, float y);
};

class Terrain {
public:
    friend class ChunkManager;
    Terrain(int gridX, int gridZ, int width, int depth, float gridSpacing, float heightScale);
    ~Terrain();

    void generate(float baseFreq = 0.1f, int octaves = 4, float persistence = 0.5f, bool buildBuffersNow = false);
    void generate(const std::vector<std::vector<float>>& heightmap, bool buildBuffersNow = false);
    void draw() const;
    float getHeightAt(float worldX, float worldZ) const;
    float getWorldHeightAt(float worldX, float worldZ) const;
    float getHeightScale() const { return m_heightScale; }

    void setMaterialSet(std::shared_ptr<MaterialSet> materialSet);
    void smoothHeights(int iterations);
    std::shared_ptr<MaterialSet> getMaterialSet() const { return m_materialSet; }
    void generateMaterials(std::shared_ptr<PerlinNoise> noise, float baseFreq = 0.1f);
    const std::vector<int>& getMaterialIndices() const { return m_materialIndices; }
    bool hasMaterials() const { return !m_materialIndices.empty(); }
    void smoothEdges(std::shared_ptr<Terrain> leftNeighbor,
        std::shared_ptr<Terrain> rightNeighbor,
        std::shared_ptr<Terrain> topNeighbor,
        std::shared_ptr<Terrain> bottomNeighbor,
        int blendRadius = 4);

    int getWidth() const { return m_width; }
    int getDepth() const { return m_depth; }
    float getGridSpacing() const { return m_gridSpacing; }
    const std::vector<float>& getHeightData() const { return m_heights; }
    void getWorldBounds(Vector3& worldMin, Vector3& worldMax) const;
    void computeBounds();
    void buildBuffers();
    void computeNormals();

    void setDiffuseTexture(const std::string& filepath);
    std::string getTexturePath() { return texturePath; }
    void setTexturePath(const std::string& filepath) { texturePath = filepath; }
    std::shared_ptr<Texture> getDiffuseTexture();
    unsigned int getTextureId() const { return this->diffuseTexture ? this->diffuseTexture->getId() : 0; }

    void setBiome(std::shared_ptr<Biome> biome);
    std::shared_ptr<Biome> getBiome() const { return m_biome; }
    void generateWithBiome(std::shared_ptr<Biome> biome, int seed);

    void setDimension(std::shared_ptr<Dimension> dim) { dimension = dim; }
    std::shared_ptr<Dimension> getDimension() const { return dimension; }
    float calculateBiomeHeight(float worldX, float worldZ, std::shared_ptr<Biome> biome) const;

    void computeNormalsWithNeighbors(std::shared_ptr<Terrain> leftNeighbor,
        std::shared_ptr<Terrain> rightNeighbor,
        std::shared_ptr<Terrain> topNeighbor,
        std::shared_ptr<Terrain> bottomNeighbor);

    std::vector<std::shared_ptr<Structure>> structures;
    std::vector<std::shared_ptr<Object3D>> objects;

    void clearStructures() {
        for (auto& s : structures) {
            // Удалить из физики? Но физика хранит свои копии объектов? В вашей архитектуре объекты физики и объекты сцены — одни и те же.
            // Если объекты уже добавлены в physics.objects, то при очистке чанка нужно удалить их оттуда.
            // Лучше хранить их только в чанке, а при генерации добавлять в physics и в workspace.
        }
        structures.clear();
    }
    bool containsPoint(float worldX, float worldZ) const {
        float localX = worldX - m_originX;
        float localZ = worldZ - m_originZ;
        float maxX = (m_width - 1) * m_gridSpacing;
        float maxZ = (m_depth - 1) * m_gridSpacing;
        return (localX >= -0.01f && localX <= maxX + 0.01f &&
            localZ >= -0.01f && localZ <= maxZ + 0.01f);
    }
    void generateStructures(unsigned int seed);
    void applyGaussianBlur(int radius);
    void debugMaterials();

    std::shared_ptr<PerlinNoise> noise;
    int gridX, gridZ;
    int64_t id;
private:
    int m_width, m_depth;
    float m_gridSpacing;  
    float m_originX, m_originZ;
    float m_heightScale;

    std::vector<float> m_heights;  
    std::vector<Vector3> m_normals;
    std::shared_ptr<Texture> diffuseTexture;
    std::string texturePath;
    std::shared_ptr<Biome> m_biome;
    std::shared_ptr<Dimension> dimension;

    std::shared_ptr<MaterialSet> m_materialSet;
    std::vector<int> m_materialIndices;
    std::vector<MaterialWeight> m_materialWeights;

    Vector3 minBound;
    Vector3 maxBound;

    unsigned int m_VAO, m_VBO, m_EBO;
    unsigned int m_indexCount;
};

#endif