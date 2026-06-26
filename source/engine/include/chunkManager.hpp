#ifndef CHUNK_MNG_HPP
#define CHUNK_MNG_HPP

#include "terrain.hpp"
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <mutex>

class Engine;
class Dimension;

class ChunkManager {
public:
    ChunkManager(int chunkSize, float gridSpacing, float heightScale, float viewDistance);
    ~ChunkManager() = default;

    void update(const Vector3& cameraPos, float viewDistance);
    std::vector<std::shared_ptr<Terrain>> getVisibleChunks() const;
    std::shared_ptr<Terrain> getChunkAt(float worldX, float worldZ);
    float getHeightAt(float worldX, float worldZ) const;
    void getWorldBounds(Vector3& worldMin, Vector3& worldMax) const;
    void setNoiseParams(float baseFreq, int octaves, float persistence);
    void clear() const;

    size_t maxChunksPerFrame = 1;
    std::shared_ptr<PerlinNoise> noise;
    Engine* engine;
    std::shared_ptr<Dimension> thisDim;
    unsigned int seed;

    int chunkSize;
    float gridSpacing;
    float heightScale;
    int viewDistance;
private:
    std::mutex queueMutex;
    std::queue<std::shared_ptr<Terrain>> readyChunks;
    std::unordered_set<int64_t> pendingIDs;

    float baseFreq;
    int octaves;
    float persistence;

    static int64_t makeID(int gridX, int gridZ) {
        return (static_cast<int64_t>(gridX) << 32) | (static_cast<uint32_t>(gridZ));
    }
};
#endif