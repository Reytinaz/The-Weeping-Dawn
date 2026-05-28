#include "worldManager.hpp"
#include "biome.hpp"

static void to_json(json& j, const Vector3& v) {
    j = json{ {"x", v.x}, {"y", v.y}, {"z", v.z} };
}

static void from_json(const json& j, Vector3& v) {
    j.at("x").get_to(v.x);
    j.at("y").get_to(v.y);
    j.at("z").get_to(v.z);
}

static std::string objectTypeToString(ObjectType type) {
    switch (type) {
    case ObjectType::CUBE: return "CUBE";
    case ObjectType::SPHERE: return "SPHERE";
    case ObjectType::PYRAMID: return "PYRAMID";
    case ObjectType::CYLINDER: return "CYLINDER";
    case ObjectType::PLANE: return "PLANE";
    case ObjectType::CUSTOM: return "CUSTOM";
    default: return "UNKNOWN";
    }
}

static ObjectType stringToObjectType(const std::string& s) {
    if (s == "CUBE") return ObjectType::CUBE;
    if (s == "SPHERE") return ObjectType::SPHERE;
    if (s == "PYRAMID") return ObjectType::PYRAMID;
    if (s == "CYLINDER") return ObjectType::CYLINDER;
    if (s == "PLANE") return ObjectType::PLANE;
    if (s == "CUSTOM") return ObjectType::CUSTOM;
    return ObjectType::CUBE;
}

static std::shared_ptr<Object3D> jsonToObject(const json& j) {
    std::string name = j.value("name", "Object");
    ObjectType type = stringToObjectType(j.value("type", "CUBE"));
    auto obj = std::make_shared<Object3D>(name, type);

    obj->position = j.at("position").get<Vector3>();
    obj->rotation = j.at("rotation").get<Vector3>();
    obj->scale = j.at("scale").get<Vector3>();
    obj->modelScale = j.at("modelScale").get<Vector3>();
    obj->mass = j.at("mass").get<float>();
    obj->restitution = j.at("restitution").get<float>();
    obj->friction = j.at("friction").get<float>();
    obj->isStatic = j.at("isStatic").get<bool>();
    obj->useGravity = j.at("useGravity").get<bool>();
    obj->visible = j.at("visible").get<bool>();
    auto& col = j.at("color");
    obj->color[0] = col.value("r", 1.0f);
    obj->color[1] = col.value("g", 1.0f);
    obj->color[2] = col.value("b", 1.0f);

    if (j.contains("tags")) {
        for (const auto& t : j["tags"]) {
            obj->addTag(t.get<std::string>());
        }
    }

    if (type == ObjectType::CUSTOM) {
        std::string modelPath = j.value("modelPath", "");
        if (!modelPath.empty()) {
            obj->loadOBJ(modelPath);
        }
    }
    obj->initOpenGL();

    if (j.contains("diffuseTexture")) {
        std::string texPath = j["diffuseTexture"];
        obj->setDiffuseTexture(texPath);
    }
    if (j.contains("normalTexture")) {
        std::string texPath = j["normalTexture"];
        obj->setNormalTexture(texPath);
    }

    return obj;
}
static json objectToJson(const std::shared_ptr<Object3D>& obj) {
    json j;
    j["name"] = obj->name;
    j["type"] = objectTypeToString(obj->type);
    j["position"] = obj->position;
    j["rotation"] = obj->rotation;
    j["scale"] = obj->scale;
    j["modelScale"] = obj->modelScale;
    j["mass"] = obj->mass;
    j["restitution"] = obj->restitution;
    j["friction"] = obj->friction;
    j["isStatic"] = obj->isStatic;
    j["useGravity"] = obj->useGravity;
    j["visible"] = obj->visible;
    j["tags"] = obj->getTags();
    j["color"] = json{ {"r", obj->color[0]}, {"g", obj->color[1]}, {"b", obj->color[2]} };

    if (obj->diffuseTexture) {
       j["diffuseTexture"] = obj->diffuseTexturePath;
    }
    if (obj->normalTexture) {
        j["normalTexture"] = obj->normalTexturePath;
    }
    if (obj->type == ObjectType::CUSTOM) {
        j["modelPath"] = obj->modelPath;
    }
    return j;
}

static json materialToJson(std::shared_ptr<Material> material) {
    json j;
    j["name"] = material->name;
    j["minHeight"] = material->minHeight;
    j["maxHeight"] = material->maxHeight;
    j["blendStrength"] = material->blendStrength;
    j["noiseFactor"] = material->noiseFactor;
    j["uvScale"] = material->uvScale;
    j["roughness"] = material->roughness;
    j["metallic"] = material->metallic;
    j["emissive"] = material->emissive;
    j["color"] = { material->color.x, material->color.y, material->color.z };
    j["texturePath"] = material->texturePath;
    j["hasTexture"] = material->hasTexture;
    return j;
}

static std::shared_ptr<Material> jsonToMaterial(const json& j) {
    auto material = std::make_shared<Material>();

    material->name = j.value("name", "default");
    material->minHeight = j.value("minHeight", 0.0f);
    material->maxHeight = j.value("maxHeight", 100.0f);
    material->blendStrength = j.value("blendStrength", 0.2f);
    material->noiseFactor = j.value("noiseFactor", 0.3f);
    material->uvScale = j.value("uvScale", 1.0f);
    material->roughness = j.value("roughness", 0.5f);
    material->metallic = j.value("metallic", 0.0f);
    material->emissive = j.value("emissive", 0.0f);
    material->texturePath = j.value("texturePath", "");
    material->hasTexture = j.value("hasTexture", false);

    if (j.contains("color") && j["color"].is_array() && j["color"].size() >= 3) {
        material->color = Vector3(j["color"][0], j["color"][1], j["color"][2]);
    }

    return material;
}

static json materialSetToJson(std::shared_ptr<MaterialSet> materialSet) {
    json j;
    j["name"] = materialSet->name;
    j["materials"] = json::array();

    for (const auto& material : materialSet->materials) {
        j["materials"].push_back(materialToJson(material));
    }

    return j;
}

static std::shared_ptr<MaterialSet> jsonToMaterialSet(const json& j) {
    auto materialSet = std::make_shared<MaterialSet>(j.value("name", "default"));

    if (j.contains("materials") && j["materials"].is_array()) {
        for (const auto& matJson : j["materials"]) {
            auto material = jsonToMaterial(matJson);
            materialSet->addMaterial(material);
        }
    }

    return materialSet;
}

static json biomeNoiseParamsToJson(const BiomeNoiseParams& params) {
    json j;
    j["baseFreq"] = params.baseFreq;
    j["octaves"] = params.octaves;
    j["persistence"] = params.persistence;
    j["amplitude"] = params.amplitude;
    j["minHeight"] = params.minHeight;
    j["maxHeight"] = params.maxHeight;
    j["roughness"] = params.roughness;
    j["redistribution"] = params.redistribution;
    j["erosion"] = params.erosion;
    j["noiseType"] = params.noiseType;
    return j;
}

static BiomeNoiseParams jsonToBiomeNoiseParams(const json& j) {
    BiomeNoiseParams params;
    params.baseFreq = j.value("baseFreq", 0.02f);
    params.octaves = j.value("octaves", 4);
    params.persistence = j.value("persistence", 0.5f);
    params.amplitude = j.value("amplitude", 20.0f);
    params.minHeight = j.value("minHeight", -10.0f);
    params.maxHeight = j.value("maxHeight", 50.0f);
    params.roughness = j.value("roughness", 0.65f);
    params.redistribution = j.value("redistribution", 1.0f);
    params.erosion = j.value("erosion", 0.2f);
    params.noiseType = static_cast<BiomeNoiseParams::NoiseType>(j.value("noiseType", 0));
    return params;
}

static json biomeStructureToJson(const BiomeStructure& structure) {
    json j;
    j["modelPath"] = structure.modelPath;
    j["density"] = structure.density;
    j["minHeight"] = structure.minHeight;
    j["maxHeight"] = structure.maxHeight;
    j["minSlope"] = structure.minSlope;
    j["maxSlope"] = structure.maxSlope;
    j["scale"] = { structure.scale.x, structure.scale.y, structure.scale.z };
    j["modelScale"] = { structure.modelScale.x, structure.modelScale.y, structure.modelScale.z };
    j["probability"] = structure.probability;
    j["spacing"] = structure.spacing;
    return j;
}

static BiomeStructure jsonToBiomeStructure(const json& j) {
    BiomeStructure structure;
    structure.modelPath = j.value("modelPath", "");
    structure.density = j.value("density", 0.5f);
    structure.minHeight = j.value("minHeight", 0.0f);
    structure.maxHeight = j.value("maxHeight", 20.0f);
    structure.minSlope = j.value("minSlope", 0.0f);
    structure.maxSlope = j.value("maxSlope", 30.0f);

    if (j.contains("scale") && j["scale"].is_array() && j["scale"].size() >= 3) {
        structure.scale = Vector3(j["scale"][0], j["scale"][1], j["scale"][2]);
    }
    if (j.contains("modelScale") && j["modelScale"].is_array() && j["modelScale"].size() >= 3) {
        structure.modelScale = Vector3(j["modelScale"][0], j["modelScale"][1], j["modelScale"][2]);
    }

    structure.probability = j.value("probability", 0.03f);
    structure.spacing = j.value("spacing", 6.0f);
    return structure;
}

static json biomeToJson(std::shared_ptr<Biome> biome) {
    json j;
    j["name"] = biome->name;
    j["noiseParams"] = biomeNoiseParamsToJson(biome->noiseParams);
    j["debugColor"] = { biome->debugColor.x, biome->debugColor.y, biome->debugColor.z };

    if (biome->materialSet) {
        j["materialSet"] = materialSetToJson(biome->materialSet);
    }

    j["structures"] = json::array();
    for (const auto& structure : biome->structures) {
        j["structures"].push_back(biomeStructureToJson(structure));
    }

    return j;
}

static std::shared_ptr<Biome> jsonToBiome(const json& j) {
    std::string name = j.value("name", "unknown");
    auto biome = std::make_shared<Biome>(name);

    if (j.contains("noiseParams")) {
        biome->noiseParams = jsonToBiomeNoiseParams(j["noiseParams"]);
    }

    if (j.contains("debugColor") && j["debugColor"].is_array() && j["debugColor"].size() >= 3) {
        biome->debugColor = Vector3(j["debugColor"][0], j["debugColor"][1], j["debugColor"][2]);
    }

    if (j.contains("materialSet")) {
        biome->materialSet = jsonToMaterialSet(j["materialSet"]);
    }

    if (j.contains("structures") && j["structures"].is_array()) {
        for (const auto& structJson : j["structures"]) {
            biome->addStructure(jsonToBiomeStructure(structJson));
        }
    }

    return biome;
}

static json terrainToJson(std::shared_ptr<Terrain> terrain) {
    json j;

    j["gridX"] = terrain->gridX;
    j["gridZ"] = terrain->gridZ;

    if (!terrain->structures.empty()) {
        json structuresJson = json::array();
        for (const auto& struct_ptr : terrain->structures) {
            json s;
            s["structName"] = struct_ptr->structName;
            s["modelPath"] = struct_ptr->modelPath;
            s["position"] = { struct_ptr->position.x, struct_ptr->position.y, struct_ptr->position.z };
            s["scale"] = { struct_ptr->scale.x, struct_ptr->scale.y, struct_ptr->scale.z };
            s["modelScale"] = { struct_ptr->modelScale.x, struct_ptr->modelScale.y, struct_ptr->modelScale.z };
            s["rotation"] = { struct_ptr->rotation.x, struct_ptr->rotation.y, struct_ptr->rotation.z };
            s["isStatic"] = struct_ptr->isStatic;
            s["type"] = struct_ptr->type;
            structuresJson.push_back(s);
        }
        j["structures"] = structuresJson;
    }

    return j;
}

static std::shared_ptr<Terrain> jsonToTerrain(const json& j, std::shared_ptr<PerlinNoise> perlinNoise, float noiseParams[3], ChunkManager& cm) {
    int gridX = j.value("gridX", 0);
    int gridZ = j.value("gridZ", 0);
    auto terrain = std::make_shared<Terrain>(gridX, gridZ, cm.m_chunkSize, cm.m_chunkSize, cm.m_gridSpacing, cm.m_heightScale);

    terrain->noise = perlinNoise;
    terrain->generate(noiseParams[0], static_cast<int>(noiseParams[1]), noiseParams[2], false);

    if (j.contains("structures") && j["structures"].is_array()) {
        for (const auto& sJson : j["structures"]) {
            auto structure = std::make_shared<Structure>();
            structure->structName = sJson.value("structName", "Unknown");
            structure->modelPath = sJson.value("modelPath", "Unknown");

            if (sJson.contains("position") && sJson["position"].is_array()) {
                auto& pos = sJson["position"];
                structure->position = Vector3(pos[0], pos[1], pos[2]);
            }

            if (sJson.contains("scale") && sJson["scale"].is_array()) {
                auto& scale = sJson["scale"];
                structure->scale = Vector3(scale[0], scale[1], scale[2]);
            }

            if (sJson.contains("modelScale") && sJson["modelScale"].is_array()) {
                auto& modelScale = sJson["modelScale"];
                structure->modelScale = Vector3(modelScale[0], modelScale[1], modelScale[2]);
            }
            else {
                structure->modelScale = Vector3(1, 1, 1);
            }

            if (sJson.contains("rotation") && sJson["rotation"].is_array()) {
                auto& rot = sJson["rotation"];
                structure->rotation = Vector3(rot[0], rot[1], rot[2]);
            }

            structure->isStatic = sJson.value("isStatic", true);
            structure->type = sJson.value("type", 1);

            terrain->structures.push_back(structure);
        }
    }

    return terrain;
}

bool WorldManager::saveChunk(const std::string& filename,
    const std::string& dimensionName,
    std::shared_ptr<Terrain> chunk) {

    std::ifstream file(filename);
    if (!file.is_open()) return false;

    json worldJson;
    try {
        file >> worldJson;
        file.close();
    }
    catch (...) {
        file.close();
        return false;
    }

    if (!worldJson.contains("dimensions")) worldJson["dimensions"] = json::object();
    if (!worldJson["dimensions"].contains(dimensionName)) worldJson["dimensions"][dimensionName] = json::object();
    if (!worldJson["dimensions"][dimensionName].contains("chunks")) worldJson["dimensions"][dimensionName]["chunks"] = json::object();

    std::string chunkKey = std::to_string(chunk->gridX) + "_" + std::to_string(chunk->gridZ);
    worldJson["dimensions"][dimensionName]["chunks"][chunkKey] = terrainToJson(chunk);

    std::ofstream outFile(filename);
    if (!outFile.is_open()) return false;
    outFile << worldJson.dump(2);
    outFile.close();

    return true;
}
std::shared_ptr<Terrain> WorldManager::loadChunk(const std::string& worldPath,
    const std::string& dimensionName, std::shared_ptr<PerlinNoise> noise,
    int gridX, int gridZ, float noiseParams[3], ChunkManager& cm) {

    std::ifstream file(worldPath);
    if (!file.is_open()) {
        return nullptr;
    }

    json worldJson;
    try {
        file >> worldJson;
        file.close();
    }
    catch (const std::exception&) {
        return nullptr;
    }

    if (!worldJson.contains("dimensions") ||
        !worldJson["dimensions"].contains(dimensionName)) {
        return nullptr;
    }

    const auto& dimJson = worldJson["dimensions"][dimensionName];
    if (!dimJson.contains("chunks") || !dimJson["chunks"].is_object()) {
        return nullptr;
    }

    std::string chunkKey = std::to_string(gridX) + "_" + std::to_string(gridZ);
    const auto& chunksObj = dimJson["chunks"];

    if (chunksObj.contains(chunkKey)) {
        const auto& chunkJson = chunksObj[chunkKey];
        auto chunk = jsonToTerrain(chunkJson, noise, noiseParams, cm);
        if (chunk) {
            float chunkWorldSize = (cm.m_chunkSize - 1) * cm.m_gridSpacing;
            float chunkCenterX = (gridX + 0.5f) * chunkWorldSize;
            float chunkCenterZ = (gridZ + 0.5f) * chunkWorldSize;

            if (cm.thisDim) {
                auto biome = cm.thisDim->getBiomeAt(chunkCenterX, chunkCenterZ, noise);
                if (biome) {
                    chunk->setBiome(biome);
                    if (biome->materialSet) {
                        chunk->setMaterialSet(biome->materialSet);
                    }
                }
                else {
                    auto defaultBiome = BiomePresets::createPlainsBiome();
                    chunk->setBiome(defaultBiome);
                    if (defaultBiome->materialSet) {
                        chunk->setMaterialSet(defaultBiome->materialSet);
                    }
                }

                if (chunk->getMaterialSet()) {
                    chunk->generateMaterials(noise, 0.2f);
                }

                return chunk;
            }
        }

        return nullptr;
    }
}

static json getPreviousChunks(const std::string& filename, const std::string& dimensionName) {
    json result = json::array();

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return result;
    }

    json worldJson;
    try {
        file >> worldJson;
        file.close();
    }
    catch (const std::exception& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        return result;
    }
    if (!worldJson.contains("dimensions")) {
        std::cerr << "No dimensions in world file" << std::endl;
        return result;
    }
    if (!worldJson["dimensions"].contains(dimensionName)) {
        std::cerr << "Dimension '" << dimensionName << "' not found" << std::endl;
        return result;
    }
    const auto& dimJson = worldJson["dimensions"][dimensionName];

    if (!dimJson.contains("chunks")) {
        std::cout << "No chunks in dimension '" << dimensionName << "'" << std::endl;
        return result;
    }
    result = dimJson["chunks"];

    return result;
}

static json celestialBodyToJson(const CelestialBody& body) {
    json j;
    j["name"] = body.name;
    j["direction"] = { body.direction.x, body.direction.y, body.direction.z };
    j["color"] = { body.color.x, body.color.y, body.color.z };
    j["intensity"] = body.intensity;
    j["size"] = body.size;
    j["orbitRadius"] = body.orbitRadius;
    j["orbitSpeed"] = body.orbitSpeed;
    j["currentAngle"] = body.currentAngle;
    j["isSun"] = body.isSun;
    j["texture"] = body.texture;
    return j;
}

static CelestialBody jsonToCelestialBody(const json& j) {
    CelestialBody body;
    body.name = j.value("name", "Unknown");

    if (j.contains("direction") && j["direction"].is_array()) {
        auto& dir = j["direction"];
        body.direction = Vector3(dir[0], dir[1], dir[2]);
    }

    if (j.contains("color") && j["color"].is_array()) {
        auto& col = j["color"];
        body.color = Vector3(col[0], col[1], col[2]);
    }

    body.intensity = j.value("intensity", 1.0f);
    body.size = j.value("size", 0.05f);
    body.orbitRadius = j.value("orbitRadius", 1.0f);
    body.orbitSpeed = j.value("orbitSpeed", 0.1f);
    body.currentAngle = j.value("currentAngle", 0.0f);
    body.isSun = j.value("isSun", false);
    body.texture = j.value("texture", "");

    return body;
}

static json lightToJson(std::shared_ptr<SourceLight> light) {
    json j;

    j["type"] = (int)light->type;
    j["position"] = { light->position.x, light->position.y, light->position.z };
    j["direction"] = { light->direction.x, light->direction.y, light->direction.z };
    j["color"] = { light->color.x, light->color.y, light->color.z };
    j["intensity"] = light->intensity;
    j["enabled"] = light->enabled;
    j["constant"] = light->constant;
    j["linear"] = light->linear;
    j["quadratic"] = light->quadratic;
    j["cutOff"] = light->cutOff;
    j["outerCutOff"] = light->outerCutOff;

    j["enableGodRays"] = light->enableGodRays;
    j["rayExposure"] = light->rayExposure;
    j["rayDecay"] = light->rayDecay;
    j["rayDensity"] = light->rayDensity;
    j["rayWeight"] = light->rayWeight;
    j["raySize"] = light->raySize;

    return j;
}

static std::shared_ptr<SourceLight> jsonToLight(const json& j) {
    auto light = std::make_shared<SourceLight>("Sunlight");

    light->type = (LightType)j.value("type", (int)DIRECTIONAL_LIGHT);
    light->enabled = j.value("enabled", true);
    light->intensity = j.value("intensity", 1.0f);

    if (j.contains("position") && j["position"].is_array() && j["position"].size() >= 3) {
        light->position = Vector3(
            j["position"][0].get<float>(),
            j["position"][1].get<float>(),
            j["position"][2].get<float>()
        );
    }

    if (j.contains("direction") && j["direction"].is_array() && j["direction"].size() >= 3) {
        light->direction = Vector3(
            j["direction"][0].get<float>(),
            j["direction"][1].get<float>(),
            j["direction"][2].get<float>()
        );
    }
    else {
        light->direction = Vector3(0, -1, 0);
    }

    if (j.contains("color") && j["color"].is_array() && j["color"].size() >= 3) {
        light->color = Vector3(
            j["color"][0].get<float>(),
            j["color"][1].get<float>(),
            j["color"][2].get<float>()
        );
    }
    else {
        light->color = Vector3(1, 1, 1);
    }

    light->constant = j.value("constant", 1.0f);
    light->linear = j.value("linear", 0.09f);
    light->quadratic = j.value("quadratic", 0.032f);
    light->cutOff = j.value("cutOff", 12.5f);
    light->outerCutOff = j.value("outerCutOff", 15.0f);

    light->enableGodRays = j.value("enableGodRays", false);
    light->rayExposure = j.value("rayExposure", 0.3f);
    light->rayDecay = j.value("rayDecay", 0.97f);
    light->rayDensity = j.value("rayDensity", 0.9f);
    light->rayWeight = j.value("rayWeight", 0.1f);
    light->raySize = j.value("raySize", 0.05f);

    return light;
}

static json dimensionToJson(std::shared_ptr<Dimension> dim) {
    json j;
    j["name"] = dim->name;
    j["spawnPoint"] = { dim->spawnPoint.x, dim->spawnPoint.y, dim->spawnPoint.z };
    j["dayNightCycle_Enabled"] = dim->dayNightCycle;
    j["dayNightCycle_Speed"] = dim->daySpeed;
    j["timeOfDay"] = dim->timeOfDay;
    j["sunlight"] = lightToJson(dim->sunlight);
    j["noiseParams"] = { dim->noiseParams[0], dim->noiseParams[1], dim->noiseParams[2] };

    j["biomes"] = json::array();
    for (const auto& biome : dim->biomes) {
        j["biomes"].push_back(biomeToJson(biome));
    }

    if (!dim->skyboxFaces.empty()) {
        j["skybox"] = dim->skyboxFaces;
    }

    return j;
}

static std::shared_ptr<Dimension> jsonToDimension(const json& j, const std::string& name, std::shared_ptr<PerlinNoise> perlinNoise, ChunkManager& cm) {
    auto dim = std::make_shared<Dimension>(j.value("name", name));

    if (j.contains("spawnPoint")) {
        const auto& sp = j["spawnPoint"];
        if (sp.is_array() && sp.size() >= 3) {
            dim->spawnPoint = Vector3(sp[0], sp[1], sp[2]);
        }
    }

    if (j.contains("skybox") && j["skybox"].is_array()) {
        dim->skyboxFaces = j["skybox"].get<std::vector<std::string>>();
    }

    dim->dayNightCycle = j.value("dayNightCycle_Enabled", true);
    dim->daySpeed = j.value("dayNightCycle_Speed", 0.001f);
    dim->timeOfDay = j.value("timeOfDay", 0.5f);
    dim->sunlight = jsonToLight(j["sunlight"]);

    if (j.contains("noiseParams")) {
        const auto& sp = j["noiseParams"];
        if (sp.is_array() && sp.size() >= 3) {
            dim->noiseParams[0] = sp[0];
            dim->noiseParams[1] = sp[1];
            dim->noiseParams[2] = sp[2];
        }
    }

    if (j.contains("biomes")) {
        for (const auto& biomeJson : j["biomes"]) {
            auto biome = jsonToBiome(biomeJson);
            dim->biomes.push_back(biome);
        }
    }
    else {
        dim->biomes = {
            BiomePresets::createPlainsBiome(),
            BiomePresets::createMountainsBiome()
        };
    }

    if (j.contains("chunks")) {
        const auto& chunksJson = j["chunks"];
        for (const auto& [key, chunkJson] : chunksJson.items()) {
            auto chunk = jsonToTerrain(chunkJson, perlinNoise, dim->noiseParams, cm);
            if (chunk) dim->chunks[chunk->id] = chunk;
        }
    }

    return dim;
}
bool WorldManager::save(const std::string& filename,
    const std::map<std::string, std::shared_ptr<Dimension>>& dimensions) {

    std::ifstream inFile(filename);
    json j;

    if (inFile.is_open()) {
        try {
            inFile >> j;
            inFile.close();
        }
        catch (...) {
            j = json::object();
        }
    }
    else {
        j = json::object();
    }

    json dimsJson = json::object();
    for (const auto& [name, dim] : dimensions) {
        json oldChunks = getPreviousChunks(filename, name);
        dimsJson[name] = dimensionToJson(dim);
        dimsJson[name]["chunks"] = oldChunks;
    }
    j["dimensions"] = dimsJson;
    j["lastPlayed"] = std::time(nullptr);

    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for saving: " << filename << std::endl;
        return false;
    }
    file << j.dump(2);
    file.close();
    std::cout << "World saved to " << filename << std::endl;
    return true;
}

bool WorldManager::load(const std::string& filename,
    std::map<std::string, std::shared_ptr<Dimension>>& dimensions, std::shared_ptr<PerlinNoise> perlinNoise, ChunkManager& cm) {

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for loading: " << filename << std::endl;
        return false;
    }

    json j;
    try {
        file >> j;
    }
    catch (const std::exception& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        return false;
    }

    int version = j.value("version", 1);
    if (version != 1) {
        std::cerr << "Unsupported world version: " << version << std::endl;
        return false;
    }

    if (j.contains("dimensions") && j["dimensions"].is_object()) {
        for (const auto& [name, dimJson] : j["dimensions"].items()) {
            float noiseParams[3] = { 0.05f, 6.0f, 0.5f };

            if (dimJson.contains("noiseParams") && dimJson["noiseParams"].is_array() && dimJson["noiseParams"].size() >= 3) {
                noiseParams[0] = dimJson["noiseParams"][0];
                noiseParams[1] = dimJson["noiseParams"][1];
                noiseParams[2] = dimJson["noiseParams"][2];
            }

            auto dim = jsonToDimension(dimJson, name, perlinNoise, cm);
            dimensions[name] = dim;
        }
    }

    std::cout << "World loaded from " << filename << std::endl;
    return true;
}

std::vector<std::shared_ptr<Dimension>> WorldManager::getDimensions() {
    return std::vector<std::shared_ptr<Dimension>>();
}

int WorldManager::getWorldSeed(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for loading: " << filepath << std::endl;
        return -1;
    }

    json j;
    try {
        file >> j;
    }
    catch (const std::exception& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        return -1;
    }
    return j.value("seed", 12345);
}

std::string WorldManager::getWorldName(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for loading: " << filepath << std::endl;
        return "None";
    }

    json j;
    try {
        file >> j;
    }
    catch (const std::exception& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        return "None";
    }
    return j.value("name", "None");
}

bool WorldManager::createNewWorld(const std::string& filename,
    const std::string& worldName,
    int seed) {

    std::cout << "Creating new world: " << worldName << std::endl;

    json worldJson;

    worldJson["version"] = 1;
    worldJson["name"] = worldName;
    worldJson["seed"] = seed;
    worldJson["created"] = std::time(nullptr);
    worldJson["lastPlayed"] = std::time(nullptr);

    json dimsJson = json::object();

    json dimJson;
    dimJson["spawnPoint"] = { 0, 10, 0 };
    dimJson["noiseParams"] = { 0.05f, 6.0f, 0.5f };

    auto materialSet = MaterialPresets::createOverworldSet();
    dimJson["materialSet"] = materialSetToJson(materialSet);

    json biomesJson = json::array();
    auto plains = BiomePresets::createPlainsBiome();
    auto mountains = BiomePresets::createMountainsBiome();
    biomesJson.push_back(biomeToJson(plains));
    biomesJson.push_back(biomeToJson(mountains));
    dimJson["biomes"] = biomesJson;

    json sunLight;
    sunLight["type"] = 0;
    sunLight["enabled"] = true;
    sunLight["direction"] = { 0, -1, 0 };
    sunLight["color"] = { 1.0f, 1.0f, 0.9f };
    sunLight["intensity"] = 1.2f;

    sunLight["enableGodRays"] = true;
    sunLight["rayExposure"] = 0.3f;
    sunLight["rayDecay"] = 0.97f;
    sunLight["rayDensity"] = 0.9f;
    sunLight["rayWeight"] = 0.1f;
    sunLight["raySize"] = 0.05f;

    dimJson["sunlight"] = sunLight;
    dimJson["dayNightCycle_Enabled"] = true;
    dimJson["dayNightCycle_Speed"] = 0.0015f;
    dimJson["timeOfDay"] = 0.35f;
    dimJson["skybox"] = {
        "assets/images/skybox/overworld/right.jpg",
        "assets/images/skybox/overworld/left.jpg",
        "assets/images/skybox/overworld/top.jpg",
        "assets/images/skybox/overworld/bottom.jpg",
        "assets/images/skybox/overworld/front.jpg",
        "assets/images/skybox/overworld/back.jpg"
    };
    dimJson["chunks"] = json::object();
    dimsJson["overworld"] = dimJson;

    worldJson["dimensions"] = dimsJson;

    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to create world file: " << filename << std::endl;
        return false;
    }

    file << worldJson.dump(2);
    file.close();

    std::cout << "New world created: " << filename << std::endl;
    return true;
}