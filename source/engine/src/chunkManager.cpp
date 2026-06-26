#include "targetArch.hpp"
#include "chunkManager.hpp"
#include "engine.hpp"
#include "dimensionManager.hpp"

ChunkManager::ChunkManager(int chunkSize, float gridSpacing, float heightScale, float viewDistance)
    : chunkSize(chunkSize), gridSpacing(gridSpacing), heightScale(heightScale),
    viewDistance(static_cast<int>(viewDistance * 125)), noise(nullptr), seed(0), engine(nullptr)
{
}

void ChunkManager::setNoiseParams(float baseFreq, int octaves, float persistence) {
    this->baseFreq = baseFreq;
    this->octaves = octaves;
    this->persistence = persistence;
}

void ChunkManager::clear() const {
    thisDim->chunks.clear();
}

void ChunkManager::update(const Vector3& cameraPos, float viewDistance) {
    if (!noise || !engine) return;
    if (thisDim) thisDim->ensureMergedMaterialCache();

    float chunkWorldSize = (chunkSize - 1) * gridSpacing;
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
                if (auto obj3d = std::dynamic_pointer_cast<Object3D>(obj)) {
                    engine->currentDimension->physics->removeObject(obj3d);
                }
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
                float chunkCenterX = (gridX + 0.5f) * (chunkSize - 1) * gridSpacing;
                float chunkCenterZ = (gridZ + 0.5f) * (chunkSize - 1) * gridSpacing;
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
                loadedChunk->clearStructures();
                auto objs = engine.worldManagment.getChunkObjetcs(engine.currentWorldPath, thisDim->name, loadedChunk);
                for (auto& e : *objs) {
                    if (auto obj3d = std::dynamic_pointer_cast<Object3D>(e)) {
                        auto obj = engine.scene.workspace->addChild<Object3D>(obj3d->name, obj3d->type);
                        obj->modelPath = obj3d->modelPath;
                        obj->scale = obj3d->scale;
                        obj->position = obj3d->position;
                        obj->rotation = obj3d->rotation;
                        obj->isStatic = obj3d->isStatic;
                        obj->color[0] = obj3d->color[0]; obj->color[1] = obj3d->color[1]; obj->color[2] = obj3d->color[2];
                        obj->modelScale = obj3d->modelScale;
                        loadedChunk->objects.push_back(obj);
                    } else loadedChunk->objects.push_back(e);;
                }
                readyChunks.push(loadedChunk);
                delete objs;
                return;
            }

            auto chunk = std::make_shared<Terrain>(gridX, gridZ, chunkSize, chunkSize, gridSpacing, heightScale);
            chunk->noise = noise;
            chunk->setDimension(thisDim);

            float chunkCenterX = (gridX + 0.5f) * (chunkSize - 1) * gridSpacing;
            float chunkCenterZ = (gridZ + 0.5f) * (chunkSize - 1) * gridSpacing;
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
            chunk->generateStructures(seed, engine);
            std::lock_guard<std::mutex> lock(queueMutex);
            readyChunks.push(chunk);
            engine.worldManagment.saveChunk(engine.currentWorldPath, thisDim->name, chunk);
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

            for (auto& structure : chunk->objects) {
                if (auto obj3d = std::dynamic_pointer_cast<Object3D>(structure)) {
                    if (obj3d->type == ObjectType::CUSTOM) {
                        auto templateObj = engine->getModel(obj3d->modelPath);
                        if (!templateObj) {
                            std::cerr << "Failed to load model: " << obj3d->modelPath << "(" << obj3d->name << ")" << std::endl;
                            continue;
                        }
                        obj3d->vertices = templateObj->vertices;
                        obj3d->normals = templateObj->normals;
                        obj3d->texCoords = templateObj->texCoords;
                        obj3d->indices = templateObj->indices;
                        obj3d->diffuseTexture = templateObj->diffuseTexture;
                        obj3d->normalTexture = templateObj->normalTexture;
                        obj3d->diffuseTexturePath = templateObj->diffuseTexturePath;
                        obj3d->normalTexturePath = templateObj->normalTexturePath;
                    }
                    engine->pushObj3D(obj3d);
                }
                if (auto srcLight = std::dynamic_pointer_cast<SourceLight>(structure)) {
                    auto light = engine->scene.workspace->addChild<SourceLight>(srcLight->name);
                    light->type = srcLight->type;
                    light->position = srcLight->position;
                    light->direction = srcLight->direction;
                    light->color = srcLight->color;
                    light->constant = srcLight->constant;
                    light->linear = srcLight->linear;
                    light->intensity = srcLight->intensity;
                    light->quadratic = srcLight->quadratic;
                    light->cutOff = srcLight->cutOff;
                    light->outerCutOff = srcLight->outerCutOff;
                }
            }

            thisDim->chunks[chunk->id] = chunk;
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                pendingIDs.erase(chunk->id);
            }
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

std::shared_ptr<Terrain> ChunkManager::getChunkAt(float worldX, float worldZ) {
    float chunkWorldSize = (chunkSize - 1) * gridSpacing;
    int gridX = static_cast<int>(std::floor(worldX / chunkWorldSize));
    int gridZ = static_cast<int>(std::floor(worldZ / chunkWorldSize));
    int64_t id = makeID(gridX, gridZ);

    auto it = thisDim->chunks.find(id);
    if (it != thisDim->chunks.end())
        return it->second;
    return nullptr;
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