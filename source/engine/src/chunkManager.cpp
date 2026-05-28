#include "targetArch.hpp"
#include "chunkManager.hpp"
#include "Engine.hpp"
#include "dimensionManager.hpp"

ChunkManager::ChunkManager(int chunkSize, float gridSpacing, float heightScale, float viewDistance)
    : m_chunkSize(chunkSize), m_gridSpacing(gridSpacing), m_heightScale(heightScale),
    m_viewDistance(static_cast<int>(viewDistance * 125)), noise(nullptr), seed(0), engine(nullptr)
{
}

void ChunkManager::setNoiseParams(float baseFreq, int octaves, float persistence) {
    m_baseFreq = baseFreq;
    m_octaves = octaves;
    m_persistence = persistence;
}

void ChunkManager::clear() const {
    thisDim->chunks.clear();
}

void ChunkManager::update(const Vector3& cameraPos, float viewDistance) {
    if (!noise || !engine) return;

    // Ensure merged material cache exists to avoid rebuilding it per-chunk
    if (thisDim) thisDim->ensureMergedMaterialCache();

    float chunkWorldSize = (m_chunkSize - 1) * m_gridSpacing;
    int viewDistChunks = static_cast<int>(std::ceil(viewDistance * 125 / chunkWorldSize));

    int camChunkX = static_cast<int>(std::floor(cameraPos.x / chunkWorldSize));
    int camChunkZ = static_cast<int>(std::floor(cameraPos.z / chunkWorldSize));

    std::unordered_set<int64_t> requiredIDs;
    for (int z = camChunkZ - viewDistChunks; z <= camChunkZ + viewDistChunks; ++z) {
        for (int x = camChunkX - viewDistChunks; x <= camChunkX + viewDistChunks; ++x) {
            requiredIDs.insert(makeID(x, z));
        }
    }

    for (auto it = thisDim->chunks.begin(); it != thisDim->chunks.end(); ) {
        if (requiredIDs.find(it->first) == requiredIDs.end()) {
            for (auto& obj : it->second->objects) {
                engine->currentDimension->physics->removeObject(obj);
                engine->scene.workspace->removeChild(obj);
            }
            it->second->objects.clear();
            it = thisDim->chunks.erase(it);
        }
        else {
            ++it;
        }
    }

    for (int64_t id : requiredIDs) {
        if (thisDim->chunks.find(id) != thisDim->chunks.end()) continue;
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (pendingIDs.find(id) != pendingIDs.end()) continue;
            pendingIDs.insert(id);
        }

        int gridX = static_cast<int>(id >> 32);
        int gridZ = static_cast<int>(id & 0xFFFFFFFF);

        engine->createThread([this, gridX, gridZ](Engine& engine) {
            auto loadedChunk = engine.worldManagment.loadChunk(engine.currentWorldPath, thisDim->name, noise, gridX, gridZ, thisDim->noiseParams, *this);
            if (loadedChunk) {
                std::lock_guard<std::mutex> lock(queueMutex);
                float chunkCenterX = (gridX + 0.5f) * (m_chunkSize - 1) * m_gridSpacing;
                float chunkCenterZ = (gridZ + 0.5f) * (m_chunkSize - 1) * m_gridSpacing;
                auto biome = thisDim->getBiomeAt(chunkCenterX, chunkCenterZ, noise);
                loadedChunk->setDimension(thisDim);
                loadedChunk->setBiome(biome);
                if (biome) {
                    loadedChunk->generateWithBiome(biome, seed);
                    if (biome->materialSet) {
                        auto materialSetCopy = std::make_shared<MaterialSet>(*biome->materialSet);
                        loadedChunk->setMaterialSet(materialSetCopy);
                    }
                }
                loadedChunk->generateMaterials(noise, 0.2f);
                loadedChunk->generateStructures(seed);
                readyChunks.push(loadedChunk);
                return;
            }

            auto chunk = std::make_shared<Terrain>(gridX, gridZ, m_chunkSize, m_chunkSize, m_gridSpacing, m_heightScale);
            chunk->noise = noise;
            chunk->setDimension(thisDim);

            float chunkCenterX = (gridX + 0.5f) * (m_chunkSize - 1) * m_gridSpacing;
            float chunkCenterZ = (gridZ + 0.5f) * (m_chunkSize - 1) * m_gridSpacing;
            auto biome = thisDim->getBiomeAt(chunkCenterX, chunkCenterZ, noise);

            chunk->setBiome(biome);
            if (biome) {
                chunk->generateWithBiome(biome, seed);
                if (biome->materialSet) {
                    auto materialSetCopy = std::make_shared<MaterialSet>(*biome->materialSet);
                    chunk->setMaterialSet(materialSetCopy);
                }
            }

            chunk->generateMaterials(noise, 0.2f);
            chunk->generateStructures(seed);
            std::lock_guard<std::mutex> lock(queueMutex);
            readyChunks.push(chunk);
        });
    }

    std::queue<std::shared_ptr<Terrain>> localReady;
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        std::swap(localReady, readyChunks);
    }

    size_t processedThisFrame = 0;
    std::queue<std::shared_ptr<Terrain>> remainingChunks;

    while (!localReady.empty()) {
        auto chunk = localReady.front();
        localReady.pop();

        if (processedThisFrame < maxChunksPerFrame) {
            chunk->buildBuffers();

            if (!chunk->getDiffuseTexture()) {
                if (chunk->getTexturePath().empty()) chunk->setDiffuseTexture("assets/images/terrain/grass.jpg");
                else chunk->setDiffuseTexture(chunk->getTexturePath());
            }

            for (auto& structure : chunk->structures) {
                auto templateObj = engine->getModel(structure->modelPath);
                if (!templateObj) {
                    std::cerr << "Failed to load model: " << structure->modelPath << std::endl;
                    continue;
                }
                auto obj = engine->scene.workspace->addChild<Object3D>(structure->structName, ObjectType::CUSTOM);
                obj->vertices = templateObj->vertices;
                obj->normals = templateObj->normals;
                obj->texCoords = templateObj->texCoords;
                obj->indices = templateObj->indices;
                obj->diffuseTexture = templateObj->diffuseTexture;

                obj->scale = structure->scale;
                obj->position = structure->position;
                obj->rotation = structure->rotation;
                obj->isStatic = structure->isStatic;
                obj->color[0] = 1.0f; obj->color[1] = 1.0f; obj->color[2] = 1.0f;
                obj->modelScale = structure->modelScale;

                engine->pushObj3D(obj);
                chunk->objects.push_back(obj);
            }

            thisDim->chunks[chunk->id] = chunk;
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                pendingIDs.erase(chunk->id);
            }
            engine->worldManagment.saveChunk(engine->currentWorldPath, thisDim->name, chunk);

            processedThisFrame++;
        }
        else {
            remainingChunks.push(chunk);
        }
    }

    if (!remainingChunks.empty()) {
        std::lock_guard<std::mutex> lock(queueMutex);
        while (!remainingChunks.empty()) {
            readyChunks.push(remainingChunks.front());
            remainingChunks.pop();
        }
    }
}

std::vector<std::shared_ptr<Terrain>> ChunkManager::getVisibleChunks() const {
    std::vector<std::shared_ptr<Terrain>> result;
    result.reserve(thisDim->chunks.size());
    for (const auto& pair : thisDim->chunks) {
        result.push_back(pair.second);
    }
    return result;
}

float ChunkManager::getHeightAt(float worldX, float worldZ) const {
    if (!thisDim) return 0.0f;

    for (auto& [id, chunk] : thisDim->chunks) {
        if (chunk->containsPoint(worldX, worldZ)) {
            return chunk->getHeightAt(worldX, worldZ);
        }
    }

    float minDist = FLT_MAX;
    std::shared_ptr<Terrain> closestChunk = nullptr;

    for (auto& [id, chunk] : thisDim->chunks) {
        float centerX = chunk->m_originX + (chunk->getWidth() - 1) * chunk->getGridSpacing() * 0.5f;
        float centerZ = chunk->m_originZ + (chunk->getDepth() - 1) * chunk->getGridSpacing() * 0.5f;
        float dist = (worldX - centerX) * (worldX - centerX) + (worldZ - centerZ) * (worldZ - centerZ);
        if (dist < minDist) {
            minDist = dist;
            closestChunk = chunk;
        }
    }

    if (closestChunk) {
        return closestChunk->getHeightAt(worldX, worldZ);
    }

    return 0.0f;
}

void ChunkManager::getWorldBounds(Vector3& worldMin, Vector3& worldMax) const {
    if (thisDim->chunks.empty()) {
        worldMin = Vector3(-100, -100, -100);
        worldMax = Vector3(100, 100, 100);
        return;
    }
    worldMin = Vector3(INFINITY, INFINITY, INFINITY);
    worldMax = Vector3(-INFINITY, -INFINITY, -INFINITY);
    for (const auto& pair : thisDim->chunks) {
        const auto& chunk = pair.second;
        Vector3 chunkMin, chunkMax;
        chunk->getWorldBounds(chunkMin, chunkMax);
        worldMin.x = std::min(worldMin.x, chunkMin.x);
        worldMin.y = std::min(worldMin.y, chunkMin.y);
        worldMin.z = std::min(worldMin.z, chunkMin.z);
        worldMax.x = std::max(worldMax.x, chunkMax.x);
        worldMax.y = std::max(worldMax.y, chunkMax.y);
        worldMax.z = std::max(worldMax.z, chunkMax.z);
    }
}