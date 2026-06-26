#define WORLD_MANAGER_HPP
#ifdef WORLD_MANAGER_HPP

#include "nlohmann/json.hpp"
#include "physics.hpp"
#include "Object3D.hpp"
#include "Container.hpp"
#include "fstream"
#include "dimensionManager.hpp"

using json = nlohmann::json;
class Engine;

class WorldManager {
public:
    std::vector<std::shared_ptr<Instance>>* getChunkObjetcs(const std::string& worldPath, const std::string& dimensionName, std::shared_ptr<Terrain>& chunk);
    bool saveChunk(const std::string& filename, const std::string& dimensionName, std::shared_ptr<Terrain> chunk);
    std::shared_ptr<Terrain> loadChunk(const std::string& worldPath, const std::string& dimensionName, std::shared_ptr<PerlinNoise> noise, int gridX, int gridZ, float noiseParams[3], ChunkManager& cm);
    bool save(const std::string& filename, const std::map<std::string, std::shared_ptr<Dimension>>& dimensions);
    bool load(const std::string& filename, std::map<std::string, std::shared_ptr<Dimension>>& dimensions,
        std::shared_ptr<PerlinNoise> perlinNoise, ChunkManager& cm);
    bool createNewWorld(const std::string& filename,
        const std::string& worldName,
        int seed);
    std::vector<std::shared_ptr<Dimension>> getDimensions();

    int getWorldSeed(const std::string& filepath);
    std::string getWorldName(const std::string& filepath);
};

#endif