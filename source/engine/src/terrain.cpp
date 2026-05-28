#include "terrain.hpp"
#include "dimensionManager.hpp"
#include "biome.hpp"
#include "glad.h"

PerlinNoise::PerlinNoise(unsigned int seed) {
    p.resize(512);
    std::vector<int> perm(256);
    std::iota(perm.begin(), perm.end(), 0);
    std::shuffle(perm.begin(), perm.end(), std::mt19937(seed));
    for (int i = 0; i < 512; ++i) p[i] = perm[i & 255];
}

static float smoothstep(float edge0, float edge1, float x) {
    float t = (x - edge0) / (edge1 - edge0);
    t = std::max(0.0f, std::min(1.0f, t));
    return t * t * (3.0f - 2.0f * t);
}

float PerlinNoise::fade(float t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

float PerlinNoise::lerp(float t, float a, float b) {
    return a + t * (b - a);
}

float PerlinNoise::grad(int hash, float x, float y) {
    int h = hash & 7;
    float u = h < 4 ? x : y;
    float v = h < 4 ? y : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

float PerlinNoise::noise(float x, float y) const {
    int X = (int)floor(x) & 255;
    int Y = (int)floor(y) & 255;
    x -= floor(x);
    y -= floor(y);
    float u = fade(x);
    float v = fade(y);

    int aa = p[p[X] + Y];
    int ab = p[p[X] + Y + 1];
    int ba = p[p[X + 1] + Y];
    int bb = p[p[X + 1] + Y + 1];

    float res = lerp(v,
        lerp(u, grad(aa, x, y), grad(ba, x - 1, y)),
        lerp(u, grad(ab, x, y - 1), grad(bb, x - 1, y - 1))
    );
    return res;
}

float PerlinNoise::ridgedNoise(float x, float y, float roughness) const {
    float n = noise(x, y);
    n = 1.0f - abs(n);
    return pow(n, roughness);
}

float PerlinNoise::billowNoise(float x, float y) const {
    float n = noise(x, y);
    return abs(n) * 2.0f - 1.0f;
}

float PerlinNoise::clampedNoise(float x, float y, float low, float high) const {
    float n = noise(x, y);
    n = (n + 1.0f) * 0.5f;
    return low + n * (high - low);
}
/*
void Terrain::generateStructures(unsigned int seed) {
    clearStructures();

    std::mt19937 rng(seed ^ (gridX * 73856093) ^ (gridZ * 19349663));
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);

    float spacing = 8.0f;
    int stepsX = static_cast<int>((m_width - 1) * m_gridSpacing / spacing);
    int stepsZ = static_cast<int>((m_depth - 1) * m_gridSpacing / spacing);

    for (int iz = 0; iz < stepsZ; ++iz) {
        for (int ix = 0; ix < stepsX; ++ix) {
            float localX = (ix + 0.5f) * spacing;
            float localZ = (iz + 0.5f) * spacing;

            if (localX > (m_width - 1) * m_gridSpacing) continue;
            if (localZ > (m_depth - 1) * m_gridSpacing) continue;

            float worldX = m_originX + localX;
            float worldZ = m_originZ + localZ;
            float height = getHeightAt(worldX, worldZ);

            float prob = 0.4f;
            float noiseVal = noise->noise(worldX * 0.1f, worldZ * 0.1f) * 0.5f + 0.5f;
            prob *= noiseVal;

            if (dist01(rng) < prob) {
                int type = static_cast<int>(dist01(rng) * 3);
                std::string modelPath;
                std::string name;
                Vector3 scale(1, 1, 1);
                Vector3 modelScale(1, 1, 1);
                if (type == 0) {
                    modelPath = "assets/models/tree1.obj";
                    scale = Vector3(2, 2, 2);
                }
                else if (type == 1) {
                    modelPath = "assets/models/tree2.obj";
                    scale = Vector3(1.5, 1.5, 1.5);
                }
                else {
                    modelPath = "assets/models/rock.obj";
                    scale = Vector3(1, 1, 1);
                }

                modelPath = "assets/models/tree.obj";
                name = "Tree";
                scale = Vector3(1.5, 5, 1.5);
                modelScale = Vector3(3.5, 1, 3.5);

                auto obj = std::make_shared<Structure>();
                obj->position = Vector3(worldX, height + scale.y / 2, worldZ);
                obj->scale = scale;
                obj->modelScale = modelScale;
                obj->rotation.y = dist01(rng) * 2 * 3.14159f;
                obj->isStatic = true;
                obj->type = 1;
                obj->modelPath = modelPath;
                obj->structName = name;

                structures.push_back(obj);
            }
        }
    }
}
*/

Terrain::Terrain(int gridX, int gridZ, int width, int depth, float gridSpacing, float heightScale)
    : m_width(width), m_depth(depth), m_gridSpacing(gridSpacing), m_heightScale(heightScale),
    m_VAO(0), m_VBO(0), m_EBO(0), m_indexCount(0),
    gridX(gridX), gridZ(gridZ), m_materialSet(nullptr)
{
    if (width < 2 || depth < 2) {
        throw std::runtime_error("Chunk size must be at least 2");
    }
    m_heights.resize(width * depth, 0.0f);
    m_normals.resize(width * depth, Vector3(0, 1, 0));
    id = (static_cast<int64_t>(gridX) << 32) | (static_cast<uint32_t>(gridZ));
    m_materialWeights.resize(width * depth);
    m_originX = gridX * (width - 1) * gridSpacing;
    m_originZ = gridZ * (depth - 1) * gridSpacing;
}

Terrain::~Terrain() {
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_VBO);
    glDeleteBuffers(1, &m_EBO);
}

float Terrain::calculateBiomeHeight(float worldX, float worldZ, std::shared_ptr<Biome> biome) const {
    if (!biome || !noise) return 0;

    const auto& params = biome->noiseParams;

    float nx = worldX * params.baseFreq;
    float nz = worldZ * params.baseFreq;

    float y = 0.0f;
    float amp = 1.0f;
    float freq = 1.0f;
    float maxAmp = 0.0f;

    for (int o = 0; o < params.octaves; ++o) {
        float n = noise->noise(nx * freq, nz * freq);
        y += n * amp;
        maxAmp += amp;
        amp *= params.persistence;
        freq *= 2.0f;
    }

    if (maxAmp > 0) y /= maxAmp;

    float t = (y + 1.0f) * 0.5f;
    t = pow(t, params.redistribution);
    y = t * 2.0f - 1.0f;

    return y * params.amplitude;
}

void Terrain::setDiffuseTexture(const std::string& filepath) {
    auto tex = std::make_shared<Texture>();
    if (tex->loadFromFile(filepath, true)) {
        this->diffuseTexture = tex;
        this->texturePath = filepath;
    }
    else {
        std::cerr << "Failed to load diffuse texture for terrain: " << filepath << std::endl;
    }
}

std::shared_ptr<Texture> Terrain::getDiffuseTexture() {
    return diffuseTexture;
}

void Terrain::setMaterialSet(std::shared_ptr<MaterialSet> materialSet) {
    m_materialSet = materialSet;
}

void Terrain::smoothHeights(int iterations) {
    std::vector<float> newHeights = m_heights;

    for (int iter = 0; iter < iterations; ++iter) {
        for (int z = 1; z < m_depth - 1; ++z) {
            for (int x = 1; x < m_width - 1; ++x) {
                int idx = z * m_width + x;

                float sum = 0;
                sum += m_heights[(z - 1) * m_width + x];     // верх
                sum += m_heights[(z + 1) * m_width + x];     // низ
                sum += m_heights[z * m_width + (x - 1)];     // лево
                sum += m_heights[z * m_width + (x + 1)];     // право
                sum += m_heights[(z - 1) * m_width + (x - 1)]; // верх-лево
                sum += m_heights[(z - 1) * m_width + (x + 1)]; // верх-право
                sum += m_heights[(z + 1) * m_width + (x - 1)]; // низ-лево
                sum += m_heights[(z + 1) * m_width + (x + 1)]; // низ-право

                float avg = sum / 8.0f;

                newHeights[idx] = m_heights[idx] * 0.5f + avg * 0.5f;
            }
        }
        m_heights = newHeights;
    }
}

void Terrain::smoothEdges(std::shared_ptr<Terrain> leftNeighbor,
    std::shared_ptr<Terrain> rightNeighbor,
    std::shared_ptr<Terrain> topNeighbor,
    std::shared_ptr<Terrain> bottomNeighbor,
    int blendRadius) {

    if (!leftNeighbor && !rightNeighbor && !topNeighbor && !bottomNeighbor) return;
    std::vector<float> newHeights = m_heights;

    if (leftNeighbor && leftNeighbor->m_heights.size() == m_heights.size()) {
        for (int z = 0; z < m_depth; ++z) {
            for (int x = 0; x < blendRadius; ++x) {
                int idx = z * m_width + x;
                float t = (float)x / blendRadius;
                float weight = 1.0f - t * t * (3.0f - 2.0f * t);

                int rightX = (m_width - 1) - (blendRadius - x);
                float neighborHeight = leftNeighbor->m_heights[z * m_width + rightX];

                newHeights[idx] = m_heights[idx] * weight + neighborHeight * (1.0f - weight);
            }
        }
    }

    if (rightNeighbor && rightNeighbor->m_heights.size() == m_heights.size()) {
        for (int z = 0; z < m_depth; ++z) {
            for (int x = m_width - blendRadius; x < m_width; ++x) {
                int idx = z * m_width + x;
                int dist = x - (m_width - blendRadius);
                float t = (float)dist / blendRadius;
                float weight = 1.0f - t * t * (3.0f - 2.0f * t);

                int leftX = blendRadius - dist - 1;
                if (leftX >= 0 && leftX < m_width) {
                    float neighborHeight = rightNeighbor->m_heights[z * m_width + leftX];
                    newHeights[idx] = m_heights[idx] * weight + neighborHeight * (1.0f - weight);
                }
            }
        }
    }

    if (topNeighbor && topNeighbor->m_heights.size() == m_heights.size()) {
        for (int x = 0; x < m_width; ++x) {
            for (int z = 0; z < blendRadius; ++z) {
                int idx = z * m_width + x;
                float t = (float)z / blendRadius;
                float weight = 1.0f - t * t * (3.0f - 2.0f * t);

                int bottomZ = (m_depth - 1) - (blendRadius - z);
                if (bottomZ >= 0 && bottomZ < m_depth) {
                    float neighborHeight = topNeighbor->m_heights[bottomZ * m_width + x];
                    newHeights[idx] = newHeights[idx] * weight + neighborHeight * (1.0f - weight);
                }
            }
        }
    }

    if (bottomNeighbor && bottomNeighbor->m_heights.size() == m_heights.size()) {
        for (int x = 0; x < m_width; ++x) {
            for (int z = m_depth - blendRadius; z < m_depth; ++z) {
                int idx = z * m_width + x;
                int dist = z - (m_depth - blendRadius);
                float t = (float)dist / blendRadius;
                float weight = 1.0f - t * t * (3.0f - 2.0f * t);

                int topZ = blendRadius - dist - 1;
                if (topZ >= 0 && topZ < m_depth) {
                    float neighborHeight = bottomNeighbor->m_heights[topZ * m_width + x];
                    newHeights[idx] = newHeights[idx] * weight + neighborHeight * (1.0f - weight);
                }
            }
        }
    }

    m_heights = newHeights;
    computeNormalsWithNeighbors(leftNeighbor, rightNeighbor, topNeighbor, bottomNeighbor);
}

void Terrain::computeNormalsWithNeighbors(std::shared_ptr<Terrain> leftNeighbor,
    std::shared_ptr<Terrain> rightNeighbor,
    std::shared_ptr<Terrain> topNeighbor,
    std::shared_ptr<Terrain> bottomNeighbor) {

    std::fill(m_normals.begin(), m_normals.end(), Vector3(0, 0, 0));

    auto getHeight = [&](int x, int z) -> float {
        if (x >= 0 && x < m_width && z >= 0 && z < m_depth) {
            return m_heights[z * m_width + x];
        }

        if (x < 0 && leftNeighbor) {
            int nx = m_width + x;
            if (nx >= 0 && nx < m_width && z >= 0 && z < m_depth) {
                return leftNeighbor->m_heights[z * m_width + nx];
            }
        }
        if (x >= m_width && rightNeighbor) {
            int nx = x - m_width;
            if (nx >= 0 && nx < m_width && z >= 0 && z < m_depth) {
                return rightNeighbor->m_heights[z * m_width + nx];
            }
        }
        if (z < 0 && topNeighbor) {
            int nz = m_depth + z;
            if (x >= 0 && x < m_width && nz >= 0 && nz < m_depth) {
                return topNeighbor->m_heights[nz * m_width + x];
            }
        }
        if (z >= m_depth && bottomNeighbor) {
            int nz = z - m_depth;
            if (x >= 0 && x < m_width && nz >= 0 && nz < m_depth) {
                return bottomNeighbor->m_heights[nz * m_width + x];
            }
        }

        if (x < 0 && z < 0 && leftNeighbor && topNeighbor) {
            int nx = m_width + x;
            int nz = m_depth + z;
            if (nx >= 0 && nx < m_width && nz >= 0 && nz < m_depth) {
                return leftNeighbor->m_heights[nz * m_width + nx];
            }
        }
        if (x >= m_width && z < 0 && rightNeighbor && topNeighbor) {
            int nx = x - m_width;
            int nz = m_depth + z;
            if (nx >= 0 && nx < m_width && nz >= 0 && nz < m_depth) {
                return rightNeighbor->m_heights[nz * m_width + nx];
            }
        }
        if (x < 0 && z >= m_depth && leftNeighbor && bottomNeighbor) {
            int nx = m_width + x;
            int nz = z - m_depth;
            if (nx >= 0 && nx < m_width && nz >= 0 && nz < m_depth) {
                return leftNeighbor->m_heights[nz * m_width + nx];
            }
        }
        if (x >= m_width && z >= m_depth && rightNeighbor && bottomNeighbor) {
            int nx = x - m_width;
            int nz = z - m_depth;
            if (nx >= 0 && nx < m_width && nz >= 0 && nz < m_depth) {
                return rightNeighbor->m_heights[nz * m_width + nx];
            }
        }

        return 0;
    };

    for (int z = 0; z < m_depth; ++z) {
        for (int x = 0; x < m_width; ++x) {
            Vector3 normal(0, 0, 0);

            float h = getHeight(x, z);
            float hx1 = getHeight(x + 1, z);
            float hx_1 = getHeight(x - 1, z);
            float hz1 = getHeight(x, z + 1);
            float hz_1 = getHeight(x, z - 1);

            float dx = (hx1 - hx_1) / (2.0f * m_gridSpacing);
            float dz = (hz1 - hz_1) / (2.0f * m_gridSpacing);

            normal = Vector3(-dx, 1.0f, -dz).normalized();

            m_normals[z * m_width + x] = normal;
        }
    }
}

void Terrain::generateMaterials(std::shared_ptr<PerlinNoise> noise, float baseFreq) {
    std::unordered_map<std::string, int> materialKeyToIndex;
    if (dimension && dimension->mergedMaterialSet) {
        m_materialSet = dimension->mergedMaterialSet;
        materialKeyToIndex = dimension->mergedMaterialIndexMap;
    }

    if (!m_materialSet) {
        if (m_biome && m_biome->materialSet) m_materialSet = m_biome->materialSet;
        if (!m_materialSet) return;
    }

    float worldOriginX = gridX * (m_width - 1) * m_gridSpacing;
    float worldOriginZ = gridZ * (m_depth - 1) * m_gridSpacing;

    for (int z = 0; z < m_depth; ++z) {
        for (int x = 0; x < m_width; ++x) {
            int idx = z * m_width + x;
            float worldX = worldOriginX + x * m_gridSpacing;
            float worldZ = worldOriginZ + z * m_gridSpacing;
            float height = m_heights[idx];

            std::vector<BiomeBlend> biomes;
            if (dimension) {
                biomes = dimension->getBiomesAt(worldX, worldZ, noise, 16.0f);
            }
            else {
                biomes.push_back({ m_biome, 1.0f });
            }

            if (biomes.size() <= 1) {
                std::shared_ptr<Biome> singleBiome = nullptr;
                if (!biomes.empty()) singleBiome = biomes[0].biome;
                if (!singleBiome) singleBiome = m_biome;

                if (singleBiome && singleBiome->materialSet) {
                    int localMatIdx = singleBiome->materialSet->getMaterialIndexAtHeight(height);
                    if (localMatIdx >= 0 && localMatIdx < static_cast<int>(singleBiome->materialSet->materials.size())) {
                        auto matPtr = singleBiome->materialSet->materials[localMatIdx];
                        std::string key = singleBiome->name + "|" + matPtr->name + "|" + matPtr->texturePath;
                        auto it = materialKeyToIndex.find(key);
                        if (it != materialKeyToIndex.end()) {
                            int mappedIdx = it->second;
                            // store primary material for smoothing later
                            // (we'll assign after smoothing pass)
                            // Temporarily store in m_materialWeights as integer
                            m_materialWeights[idx].matBL = static_cast<float>(mappedIdx);
                            m_materialWeights[idx].matBR = static_cast<float>(mappedIdx);
                            m_materialWeights[idx].matTL = static_cast<float>(mappedIdx);
                            m_materialWeights[idx].matTR = static_cast<float>(mappedIdx);
                            continue;
                        }
                    }
                }

                int matIdx = m_materialSet->getMaterialIndexAtHeight(height);
                m_materialWeights[idx].matBL = static_cast<float>(matIdx);
                m_materialWeights[idx].matBR = static_cast<float>(matIdx);
                m_materialWeights[idx].matTL = static_cast<float>(matIdx);
                m_materialWeights[idx].matTR = static_cast<float>(matIdx);
                continue;
            }

            std::map<int, float> materialWeights;
            float totalWeight = 0;

            for (const auto& [biome, weight] : biomes) {
                if (!biome || !biome->materialSet) continue;

                int matIdx = biome->materialSet->getMaterialIndexAtHeight(height);
                if (matIdx < 0 || matIdx >= static_cast<int>(biome->materialSet->materials.size())) continue;
                auto& matPtr = biome->materialSet->materials[matIdx];
                std::string key = biome->name + "|" + matPtr->name + "|" + matPtr->texturePath;
                auto it = materialKeyToIndex.find(key);
                if (it == materialKeyToIndex.end()) {
                    auto copy = std::make_shared<Material>(*matPtr);
                    m_materialSet->materials.push_back(copy);
                    int newIdx = static_cast<int>(m_materialSet->materials.size() - 1);
                    materialKeyToIndex[key] = newIdx;
                    materialWeights[newIdx] += weight;
                    if (dimension) {
                        dimension->mergedMaterialIndexMap[key] = newIdx;
                    }
                }
                else {
                    materialWeights[it->second] += weight;
                }
                totalWeight += weight;
            }

            if (totalWeight <= 0) continue;

            std::vector<std::pair<int, float>> sortedWeights;
            for (auto& [matIdx, w] : materialWeights) {
                sortedWeights.push_back({ matIdx, w / totalWeight });
            }

            std::sort(sortedWeights.begin(), sortedWeights.end(),
                [](const auto& a, const auto& b) { return a.second > b.second; });

            int mainMat = sortedWeights[0].first;
            int secondMat = mainMat;
            float blendWeight = 0.0f;

            if (sortedWeights.size() > 1) {
                secondMat = sortedWeights[1].first;
                blendWeight = sortedWeights[1].second;

                const float blendThreshold = 0.05f;
                if (blendWeight > blendThreshold) {
                    blendWeight = (blendWeight - blendThreshold) / (1.0f - blendThreshold);
                }
                else {
                    blendWeight = 0.0f;
                }
            }

            m_materialWeights[idx].matBL = static_cast<float>(mainMat);
            m_materialWeights[idx].matBR = static_cast<float>(secondMat);
            m_materialWeights[idx].matTL = blendWeight;
        }
    }

    std::vector<int> primary(m_width * m_depth, 0);
    std::vector<int> secondary(m_width * m_depth, 0);
    std::vector<float> blend(m_width * m_depth, 0.0f);

    for (int i = 0; i < m_width * m_depth; ++i) {
        primary[i] = static_cast<int>(m_materialWeights[i].matBL);
        secondary[i] = static_cast<int>(m_materialWeights[i].matBR);
        blend[i] = m_materialWeights[i].matTL;
    }

    std::vector<int> blurred(primary.size(), 0);
    int r = 1; // radius for smoothing, can be tuned
    for (int z = 0; z < m_depth; ++z) {
        for (int x = 0; x < m_width; ++x) {
            int idx = z * m_width + x;
            float sum = 0.0f;
            int count = 0;
            for (int dz = -r; dz <= r; ++dz) {
                for (int dx = -r; dx <= r; ++dx) {
                    int nx = x + dx;
                    int nz = z + dz;
                    if (nx < 0 || nx >= m_width || nz < 0 || nz >= m_depth) continue;
                    sum += static_cast<float>(primary[nz * m_width + nx]);
                    count++;
                }
            }
            if (count > 0) {
                float avg = sum / count;
                blurred[idx] = static_cast<int>(std::round(avg));
            }
            else blurred[idx] = primary[idx];
        }
    }

    // Reassign final material weights per corner using blurred primary and stored secondary/blend
    for (int z = 0; z < m_depth; ++z) {
        for (int x = 0; x < m_width; ++x) {
            int idx = z * m_width + x;
            int mainMat = blurred[idx];
            int secMat = secondary[idx];
            float bw = blend[idx];

            if (bw > 0.01f && secMat != mainMat) {
                // alternate corner assignment for simple cross-blend
                m_materialWeights[idx].matBL = static_cast<float>(mainMat);
                m_materialWeights[idx].matBR = static_cast<float>(secMat);
                m_materialWeights[idx].matTL = static_cast<float>(mainMat);
                m_materialWeights[idx].matTR = static_cast<float>(secMat);
            } else {
                m_materialWeights[idx].matBL = static_cast<float>(mainMat);
                m_materialWeights[idx].matBR = static_cast<float>(mainMat);
                m_materialWeights[idx].matTL = static_cast<float>(mainMat);
                m_materialWeights[idx].matTR = static_cast<float>(mainMat);
            }
        }
    }
}

void Terrain::generate(float baseFreq, int octaves, float persistence, bool buildBuffersNow) {
    float worldOriginX = gridX * (m_width - 1) * m_gridSpacing;
    float worldOriginZ = gridZ * (m_depth - 1) * m_gridSpacing;

    for (int z = 0; z < m_depth; ++z) {
        for (int x = 0; x < m_width; ++x) {
            float worldX = worldOriginX + x * m_gridSpacing;
            float worldZ = worldOriginZ + z * m_gridSpacing;

            float nx = worldX / (m_width * m_gridSpacing) * baseFreq;
            float nz = worldZ / (m_depth * m_gridSpacing) * baseFreq;

            float y = 0.0f;
            float amp = 1.0f;
            float freq = 1.0f;
            for (int o = 0; o < octaves; ++o) {
                y += noise->noise(nx * freq, nz * freq) * amp;
                amp *= persistence;
                freq *= 2.0f;
            }
            m_heights[z * m_width + x] = y * m_heightScale;
        }
    }
    computeNormals();
    computeBounds();

    if (buildBuffersNow) buildBuffers();
}

void Terrain::generate(const std::vector<std::vector<float>>& heightmap, bool buildBuffersNow) {
    for (int z = 0; z < m_depth; ++z)
        for (int x = 0; x < m_width; ++x)
            m_heights[z * m_width + x] = heightmap[z][x] * m_heightScale;
    computeNormals();
    computeBounds();
    if (buildBuffersNow) buildBuffers();
}

void Terrain::setBiome(std::shared_ptr<Biome> biome) {
    m_biome = biome;
    if (biome && biome->materialSet) {
        setMaterialSet(biome->materialSet);
    }
}

void Terrain::generateWithBiome(std::shared_ptr<Biome> biome, int seed) {
    if (!biome) return;

    m_biome = biome;

    float worldOriginX = gridX * (m_width - 1) * m_gridSpacing;
    float worldOriginZ = gridZ * (m_depth - 1) * m_gridSpacing;

    if (biome->materialSet) {
        setMaterialSet(biome->materialSet);
    }

    for (int z = 0; z < m_depth; ++z) {
        for (int x = 0; x < m_width; ++x) {
            float worldX = worldOriginX + x * m_gridSpacing;
            float worldZ = worldOriginZ + z * m_gridSpacing;

            std::vector<BiomeBlend> biomes;
            if (dimension) {
                biomes = dimension->getBiomesAt(worldX, worldZ, noise, 64.0f);
            }
            else {
                biomes.push_back({ biome, 1.0f });
            }

            float totalHeight = 0.0f;
            float totalWeight = 0.0f;

            // apply a small smoothing to weights to avoid single-sample spikes
            for (const auto& [b, weight] : biomes) {
                if (!b) continue;
                float w = std::max(0.0f, weight);
                // small exponent to favor stronger biomes but keep contribution
                w = pow(w, 1.1f);
                float h = calculateBiomeHeight(worldX, worldZ, b);
                totalHeight += h * w;
                totalWeight += w;
            }

            if (totalWeight > 0.0f) {
                m_heights[z * m_width + x] = totalHeight / totalWeight;
            }
        }
    }

    applyGaussianBlur(2);
    computeNormals();
    computeBounds();

    if (biome->materialSet) {
        setMaterialSet(biome->materialSet);
    }
}

void Terrain::generateStructures(unsigned int seed) {
    clearStructures();

    if (!m_biome || m_biome->structures.empty()) return;

    std::mt19937 rng(seed ^ (gridX * 73856093) ^ (gridZ * 19349663));
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);

    for (const auto& structDef : m_biome->structures) {
        float spacing = structDef.spacing;
        int stepsX = static_cast<int>((m_width - 1) * m_gridSpacing / spacing);
        int stepsZ = static_cast<int>((m_depth - 1) * m_gridSpacing / spacing);

        if (stepsX <= 0) stepsX = 1;
        if (stepsZ <= 0) stepsZ = 1;

        for (int iz = 0; iz < stepsZ; ++iz) {
            for (int ix = 0; ix < stepsX; ++ix) {
                float offsetX = (dist01(rng) - 0.5f) * spacing * 0.3f;
                float offsetZ = (dist01(rng) - 0.5f) * spacing * 0.3f;

                float localX = (ix + 0.5f) * spacing + offsetX;
                float localZ = (iz + 0.5f) * spacing + offsetZ;

                if (localX < 0 || localX >(m_width - 1) * m_gridSpacing) continue;
                if (localZ < 0 || localZ >(m_depth - 1) * m_gridSpacing) continue;

                float worldX = m_originX + localX;
                float worldZ = m_originZ + localZ;
                float height = getHeightAt(worldX, worldZ);

                if (height < structDef.minHeight || height > structDef.maxHeight) continue;

                float slope = 0.0f;
                float hx = getHeightAt(worldX + 1.0f, worldZ);
                float hz = getHeightAt(worldX, worldZ + 1.0f);
                slope = atan(sqrt(pow(hx - height, 2.0f) + pow(hz - height, 2.0f))) * 180.0f / 3.14159f;

                if (slope < structDef.minSlope || slope > structDef.maxSlope) continue;

                float noiseVal = noise->noise(worldX * 0.1f, worldZ * 0.1f) * 0.5f + 0.5f;
                float prob = structDef.probability * structDef.density * noiseVal;

                bool tooClose = false;
                for (const auto& existing : structures) {
                    float dx = existing->position.x - worldX;
                    float dz = existing->position.z - worldZ;
                    float dist = sqrt(dx * dx + dz * dz);
                    if (dist < spacing * 0.6f) {
                        tooClose = true;
                        break;
                    }
                }

                if (!tooClose && dist01(rng) < prob) {
                    auto obj = std::make_shared<Structure>();
                    obj->modelPath = structDef.modelPath;
                    obj->structName = m_biome->name + "_" +
                        (structDef.modelPath.find("tree") != std::string::npos ? "tree" : "rock");

                    obj->scale = structDef.scale;
                    obj->modelScale = structDef.modelScale;
                    obj->position = Vector3(worldX, height + structDef.scale.y * 0.5f, worldZ);
                    obj->rotation.y = dist01(rng) * 2 * 3.14159f;
                    obj->isStatic = true;
                    obj->type = 1;

                    structures.push_back(obj);
                }
            }
        }
    }
}

void Terrain::applyGaussianBlur(int radius) {
    std::vector<float> blurred = m_heights;
    std::vector<float> kernel(radius * 2 + 1);

    float sigma = radius / 2.0f;
    float sum = 0;

    for (int i = -radius; i <= radius; ++i) {
        float val = exp(-(i * i) / (2 * sigma * sigma));
        kernel[i + radius] = val;
        sum += val;
    }

    for (size_t i = 0; i < kernel.size(); ++i) {
        kernel[i] /= sum;
    }

    for (int z = 0; z < m_depth; ++z) {
        for (int x = radius; x < m_width - radius; ++x) {
            float sum = 0;
            for (int dx = -radius; dx <= radius; ++dx) {
                sum += m_heights[z * m_width + (x + dx)] * kernel[dx + radius];
            }
            blurred[z * m_width + x] = sum;
        }
    }

    for (int z = radius; z < m_depth - radius; ++z) {
        for (int x = 0; x < m_width; ++x) {
            float sum = 0;
            for (int dz = -radius; dz <= radius; ++dz) {
                sum += blurred[(z + dz) * m_width + x] * kernel[dz + radius];
            }
            m_heights[z * m_width + x] = sum;
        }
    }
}

void Terrain::getWorldBounds(Vector3& worldMin, Vector3& worldMax) const {
    worldMin = Vector3(m_originX + minBound.x, minBound.y, m_originZ + minBound.z);
    worldMax = Vector3(m_originX + maxBound.x, maxBound.y, m_originZ + maxBound.z);
}

void Terrain::computeBounds() {
    if (m_heights.empty()) {
        minBound = Vector3(0, 0, 0);
        maxBound = Vector3(0, 0, 0);
        return;
    }
    minBound = Vector3(INFINITY, INFINITY, INFINITY);
    maxBound = Vector3(-INFINITY, -INFINITY, -INFINITY);
    for (int z = 0; z < m_depth; ++z) {
        for (int x = 0; x < m_width; ++x) {
            float wx = x * m_gridSpacing;
            float wy = m_heights[z * m_width + x];
            float wz = z * m_gridSpacing;

            minBound.x = std::min(minBound.x, wx);
            minBound.y = std::min(minBound.y, wy);
            minBound.z = std::min(minBound.z, wz);

            maxBound.x = std::max(maxBound.x, wx);
            maxBound.y = std::max(maxBound.y, wy);
            maxBound.z = std::max(maxBound.z, wz);
        }
    }
}

void Terrain::computeNormals() {
    std::fill(m_normals.begin(), m_normals.end(), Vector3(0, 0, 0));

    for (int z = 0; z < m_depth - 1; ++z) {
        for (int x = 0; x < m_width - 1; ++x) {
            int i0 = z * m_width + x;
            int i1 = z * m_width + x + 1;
            int i2 = (z + 1) * m_width + x;
            int i3 = (z + 1) * m_width + x + 1;

            Vector3 v0(x * m_gridSpacing, m_heights[i0], z * m_gridSpacing);
            Vector3 v1((x + 1) * m_gridSpacing, m_heights[i1], z * m_gridSpacing);
            Vector3 v2(x * m_gridSpacing, m_heights[i2], (z + 1) * m_gridSpacing);
            Vector3 v3((x + 1) * m_gridSpacing, m_heights[i3], (z + 1) * m_gridSpacing);

            Vector3 e1 = v1 - v0;
            Vector3 e2 = v2 - v0;
            Vector3 faceNormal = e2.cross(e1).normalized();
            m_normals[i0] += faceNormal;
            m_normals[i1] += faceNormal;
            m_normals[i2] += faceNormal;

            e1 = v3 - v1;
            e2 = v2 - v1;
            faceNormal = e2.cross(e1).normalized();
            m_normals[i1] += faceNormal;
            m_normals[i3] += faceNormal;
            m_normals[i2] += faceNormal;
        }
    }

    for (size_t i = 0; i < m_normals.size(); ++i) {
        float len = m_normals[i].length();
        if (len > 1e-6f) m_normals[i] = m_normals[i] * (1.0f / len);
    }
}

void Terrain::buildBuffers() {
if (m_heights.empty() || m_normals.empty()) return;

    struct Vertex {
        float px, py, pz;
        float nx, ny, nz;
        float tu, tv;
        float material1;
        float material2;
        float material3;
    };

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    
    for (int z = 0; z < m_depth - 1; ++z) {
        for (int x = 0; x < m_width - 1; ++x) {
            int i0 = z * m_width + x;
            int i1 = z * m_width + x + 1;
            int i2 = (z + 1) * m_width + x;
            int i3 = (z + 1) * m_width + x + 1;
            
            float h00 = m_heights[i0];
            float h10 = m_heights[i1];
            float h01 = m_heights[i2];
            float h11 = m_heights[i3];
            
            float m00 = m_materialWeights[i0].matBL;
            float m10 = m_materialWeights[i1].matBR;
            float m01 = m_materialWeights[i2].matTL;
            float m11 = m_materialWeights[i3].matTR;
            
            Vertex v0, v1, v2;
            
            v0.px = m_originX + x * m_gridSpacing;
            v0.py = h00;
            v0.pz = m_originZ + z * m_gridSpacing;
            v0.nx = m_normals[i0].x;
            v0.ny = m_normals[i0].y;
            v0.nz = m_normals[i0].z;
            v0.tu = (float)x / (m_width - 1) * 60.0f;
            v0.tv = (float)z / (m_depth - 1) * 60.0f;
            v0.material1 = m00;
            v0.material2 = m10;
            v0.material3 = m01;
            
            v1.px = m_originX + (x + 1) * m_gridSpacing;
            v1.py = h10;
            v1.pz = m_originZ + z * m_gridSpacing;
            v1.nx = m_normals[i1].x;
            v1.ny = m_normals[i1].y;
            v1.nz = m_normals[i1].z;
            v1.tu = (float)(x + 1) / (m_width - 1) * 60.0f;
            v1.tv = (float)z / (m_depth - 1) * 60.0f;
            v1.material1 = m00;
            v1.material2 = m10;
            v1.material3 = m01;
            
            v2.px = m_originX + x * m_gridSpacing;
            v2.py = h01;
            v2.pz = m_originZ + (z + 1) * m_gridSpacing;
            v2.nx = m_normals[i2].x;
            v2.ny = m_normals[i2].y;
            v2.nz = m_normals[i2].z;
            v2.tu = (float)x / (m_width - 1) * 60.0f;
            v2.tv = (float)(z + 1) / (m_depth - 1) * 60.0f;
            v2.material1 = m00;
            v2.material2 = m10;
            v2.material3 = m01;
            
            vertices.push_back(v0);
            vertices.push_back(v1);
            vertices.push_back(v2);
            
            Vertex v3, v4, v5;
            
            v3 = v1;
            v3.material1 = m11;
            v3.material2 = m01;
            v3.material3 = m10;
            
            v4.px = m_originX + (x + 1) * m_gridSpacing;
            v4.py = h11;
            v4.pz = m_originZ + (z + 1) * m_gridSpacing;
            v4.nx = m_normals[i3].x;
            v4.ny = m_normals[i3].y;
            v4.nz = m_normals[i3].z;
            v4.tu = (float)(x + 1) / (m_width - 1) * 60.0f;
            v4.tv = (float)(z + 1) / (m_depth - 1) * 60.0f;
            v4.material1 = m11;
            v4.material2 = m01;
            v4.material3 = m10;
            
            v5 = v2;
            v5.material1 = m11;
            v5.material2 = m01;
            v5.material3 = m10;
            
            vertices.push_back(v3);
            vertices.push_back(v4);
            vertices.push_back(v5);
        }
    }
    
    m_indexCount = vertices.size();
    
    glGenBuffers(1, &m_EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    
    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // TexCoord
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    // material1
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);
    
    // material2
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(9 * sizeof(float)));
    glEnableVertexAttribArray(4);
    
    // material3
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(10 * sizeof(float)));
    glEnableVertexAttribArray(5);
    
    glBindVertexArray(0);
}

void Terrain::draw() const {
    glBindVertexArray(m_VAO);
    glDrawArrays(GL_TRIANGLES, 0, m_indexCount);
    glBindVertexArray(0);
}

float Terrain::getHeightAt(float worldX, float worldZ) const {
    if (m_heights.empty() || m_width == 0 || m_depth == 0) {
        return 0.0f;
    }
    float localX = worldX - m_originX;
    float localZ = worldZ - m_originZ;

    float maxX = (m_width - 1) * m_gridSpacing;
    float maxZ = (m_depth - 1) * m_gridSpacing;

    if (localX < -0.01f || localX > maxX + 0.01f || localZ < -0.01f || localZ > maxZ + 0.01f) {
        return 0.0f;
    }

    localX = std::max(0.0f, std::min(localX, maxX));
    localZ = std::max(0.0f, std::min(localZ, maxZ));

    float fx = localX / m_gridSpacing;
    float fz = localZ / m_gridSpacing;

    int x0 = static_cast<int>(floor(fx));
    int z0 = static_cast<int>(floor(fz));
    int x1 = std::min(x0 + 1, m_width - 1);
    int z1 = std::min(z0 + 1, m_depth - 1);
    x0 = std::max(0, x0);
    z0 = std::max(0, z0);

    float h00 = m_heights[z0 * m_width + x0];
    float h10 = m_heights[z0 * m_width + x1];
    float h01 = m_heights[z1 * m_width + x0];
    float h11 = m_heights[z1 * m_width + x1];

    float tx = fx - static_cast<float>(x0);
    float tz = fz - static_cast<float>(z0);

    float h0 = h00 * (1.0f - tx) + h10 * tx;
    float h1 = h01 * (1.0f - tx) + h11 * tx;
    return h0 * (1.0f - tz) + h1 * tz;
}

float Terrain::getWorldHeightAt(float worldX, float worldZ) const {
    return getHeightAt(worldX, worldZ);
}

void Terrain::debugMaterials() {
    if (!m_materialSet) {
        std::cout << "Chunk (" << gridX << "," << gridZ << ") - NO MATERIAL SET!" << std::endl;
        return;
    }

    std::cout << "Chunk (" << gridX << "," << gridZ << ") - Biome: "
        << (m_biome ? m_biome->name : "NULL") << std::endl;
    std::cout << "  MaterialSet: " << m_materialSet->name << " with "
        << m_materialSet->materials.size() << " materials:" << std::endl;

    for (size_t i = 0; i < m_materialSet->materials.size(); ++i) {
        auto& mat = m_materialSet->materials[i];
        std::cout << "    [" << i << "] " << mat->name
            << " height [" << mat->minHeight << ", " << mat->maxHeight << "]" << std::endl;
    }

    // Показываем высоты нескольких точек
    std::cout << "  Sample heights:" << std::endl;
    for (int z = 0; z < m_depth; z += m_depth / 4) {
        for (int x = 0; x < m_width; x += m_width / 4) {
            int idx = z * m_width + x;
            float h = m_heights[idx];
            int matIdx = m_materialSet->getMaterialIndexAtHeight(h);
            std::cout << "    (" << x << "," << z << ") h=" << h << " -> mat[" << matIdx << "]" << std::endl;
        }
    }
}