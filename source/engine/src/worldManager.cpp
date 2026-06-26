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

static json lightToJson(std::shared_ptr<SourceLight> light) {
    json j;

    j["className"] = "SourceLight";
    j["name"] = light->name;
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

    return j;
}

static std::shared_ptr<SourceLight> jsonToLight(const json& j) {
    auto light = std::make_shared<SourceLight>("SourceLight");

    light->name = j.value("name", "SourceLight");
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
    light->linear = j.value("linear", 0.4f);
    light->quadratic = j.value("quadratic", 0.2f);
    light->cutOff = j.value("cutOff", 12.5f);
    light->outerCutOff = j.value("outerCutOff", 15.0f);

    return light;
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
    j["name"] = structure.name;
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
    structure.name = j.value("name", "model");
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

    if (!terrain->objects.empty()) {
        json structuresJson = json::array();
        for (const auto& struct_ptr : terrain->objects) {
            if (auto obj3d = std::dynamic_pointer_cast<Object3D>(struct_ptr)) {
                json s;
                s["className"] = "Object3D";
                s["name"] = obj3d->name;
                s["modelPath"] = obj3d->modelPath;
                s["position"] = { obj3d->position.x, obj3d->position.y, obj3d->position.z };
                s["color"] = { obj3d->color[0], obj3d->color[1], obj3d->color[2] };
                s["scale"] = { obj3d->scale.x, obj3d->scale.y, obj3d->scale.z };
                s["modelScale"] = { obj3d->modelScale.x, obj3d->modelScale.y, obj3d->modelScale.z };
                s["rotation"] = { obj3d->rotation.x, obj3d->rotation.y, obj3d->rotation.z };
                s["isStatic"] = obj3d->isStatic;
                s["type"] = objectTypeToString(obj3d->type);
                structuresJson.push_back(s);
            }
            if (auto srcLight = std::dynamic_pointer_cast<SourceLight>(struct_ptr)) {
                auto light = lightToJson(srcLight);
                structuresJson.push_back(light);
            }
        }
        j["objects"] = structuresJson;
    }

    return j;
}

static std::shared_ptr<Terrain> jsonToTerrain(const json& j, std::shared_ptr<PerlinNoise> perlinNoise, float noiseParams[3], ChunkManager& cm) {
    int gridX = j.value("gridX", 0);
    int gridZ = j.value("gridZ", 0);
    auto terrain = std::make_shared<Terrain>(gridX, gridZ, cm.chunkSize, cm.chunkSize, cm.gridSpacing, cm.heightScale);

    terrain->noise = perlinNoise;
    terrain->generate(noiseParams[0], static_cast<int>(noiseParams[1]), noiseParams[2], false);

    if (j.contains("objects") && j["objects"].is_array()) {
        for (const auto& sJson : j["objects"]) {
            std::string className = sJson.value("className", "Unknown");
            if (className == "Object3D") {
                auto structure = std::make_shared<Object3D>();
                structure->name = sJson.value("structName", "Unknown");
                structure->modelPath = sJson.value("modelPath", "Unknown");

                if (sJson.contains("position") && sJson["position"].is_array()) {
                    auto& pos = sJson["position"];
                    structure->position = Vector3(pos[0], pos[1], pos[2]);
                }

                if (sJson.contains("color") && sJson["color"].is_array()) {
                    auto& col = sJson["color"];
                    structure->color[0] = col[0];
                    structure->color[1] = col[1];
                    structure->color[2] = col[2];
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
                structure->type = stringToObjectType(sJson.value("type", "CUBE"));

                terrain->objects.push_back(structure);
            }
            if (className == "SourceLight") {
				auto light = jsonToLight(sJson);
                terrain->objects.push_back(light);
            }
        }
    }

    return terrain;
}

std::vector<std::shared_ptr<Instance>>* WorldManager::getChunkObjetcs(const std::string& worldPath, const std::string& dimensionName, std::shared_ptr<Terrain>& chunk) {
    auto objs = new std::vector<std::shared_ptr<Instance>>();
    std::ifstream file(worldPath);
    if (!file.is_open()) {
        return objs;
    }

    json worldJson;
    try {
        file >> worldJson;
        file.close();
    }
    catch (const std::exception&) {
        return objs;
    }

    if (!worldJson.contains("dimensions") ||
        !worldJson["dimensions"].contains(dimensionName)) {
        return objs;
    }

    const auto& dimJson = worldJson["dimensions"][dimensionName];
    if (!dimJson.contains("chunks") || !dimJson["chunks"].is_object()) {
        return objs;
    }

    std::string chunkKey = std::to_string(chunk->gridX) + "_" + std::to_string(chunk->gridZ);
    const auto& chunksObj = dimJson["chunks"];

    if (chunksObj.contains(chunkKey)) {
        const auto& chunkJson = chunksObj[chunkKey];
        if (chunkJson.contains("objects") && chunkJson["objects"].is_array()) {
            for (const auto& sJson : chunkJson["objects"]) {
                std::string className = sJson.value("className", "Unknown");
                if (className == "Object3D") {
                    auto structure = std::make_shared<Object3D>();
                    structure->name = sJson.value("structName", "Unknown");
                    structure->modelPath = sJson.value("modelPath", "Unknown");
                    if (sJson.contains("position") && sJson["position"].is_array()) {
                        auto& pos = sJson["position"];
                        structure->position = Vector3(pos[0], pos[1], pos[2]);
                    }
                    if (sJson.contains("color") && sJson["color"].is_array()) {
                        auto& col = sJson["color"];
                        structure->color[0] = col[0];
                        structure->color[1] = col[1];
                        structure->color[2] = col[2];
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
                    structure->type = stringToObjectType(sJson.value("type", "CUBE"));
                    objs->push_back(structure);
                }
                if (className == "SourceLight") {
                    auto light = jsonToLight(sJson);
                    objs->push_back(light);
                }
            }
        }
    }
    return objs;
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
            float chunkWorldSize = (cm.chunkSize - 1) * cm.gridSpacing;
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

static json celestialBodyToJson(std::shared_ptr<CelestialBody> body) {
	json j;
	j["name"] = body->name;
	j["size"] = body->size;
	j["softness"] = body->softness;
	j["texturePath"] = body->texturePath;
    j["sourceLight"] = lightToJson(body->sourceLight);
	return j;
}

static std::shared_ptr<CelestialBody> jsonToCelestialBody(const json& j) {
	auto body = std::make_shared<CelestialBody>();
    body->name = j.value("name", "Unknown");
	body->size = j.value("size", 1.0f);
    body->softness = j.value("softness", 0.2f);
	body->texturePath = j.value("texturePath", "");
	body->sourceLight = jsonToLight(j["sourceLight"]);
	return body;
}

static json dimensionToJson(std::shared_ptr<Dimension> dim) {
    json j;
    j["name"] = dim->name;
    j["spawnPoint"] = { dim->spawnPoint.x, dim->spawnPoint.y, dim->spawnPoint.z };
    j["dayNightCycle_Enabled"] = dim->dayNightCycle;
    j["dayNightCycle_Speed"] = dim->daySpeed;
    j["timeOfDay"] = dim->timeOfDay;
	j["fogStart"] = dim->fogStart;
    j["hasClouds"] = dim->hasClouds;
	j["cloudsCover"] = dim->cloudsCover;
	j["cloudsDensity"] = dim->cloudsDensity;
	j["cloudsScale"] = dim->cloudsScale;
	j["cloudsWind"] = { dim->cloudsWind.x, dim->cloudsWind.y, dim->cloudsWind.z };
    j["noiseParams"] = { dim->noiseParams[0], dim->noiseParams[1], dim->noiseParams[2] };
	j["celestialBodies"] = json::array();
    j["biomes"] = json::array();

	for (const auto& body : dim->celestialBodies) {
		j["celestialBodies"].push_back(celestialBodyToJson(body));
	}

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
    dim->fogStart = j.value("fogStart", 0.5f);
	dim->hasClouds = j.value("hasClouds", true);
    dim->cloudsCover = j.value("cloudsCover", 0.5f);
    dim->cloudsDensity = j.value("cloudsDensity", 1.0f);
	dim->cloudsScale = j.value("cloudsScale", 0.00003f);
    if (j.contains("cloudsWind")) {
        const auto& sp = j["cloudsWind"];
        if (sp.is_array() && sp.size() >= 3) {
            dim->cloudsWind = Vector3(sp[0], sp[1], sp[2]);
        }
    }

	if (j.contains("celestialBodies")) {
		for (const auto& bodyJson : j["celestialBodies"]) {
			auto body = jsonToCelestialBody(bodyJson);
			dim->celestialBodies.push_back(body);
		}
	}

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


    // OVERWORLD:

    json dimJson;
    dimJson["spawnPoint"] = { 0, 10, 0 };
    dimJson["noiseParams"] = { 0.05f, 6.0f, 0.5f };
    dimJson["fogStart"] = 0.5f;
    dimJson["hasClouds"] = true;
    dimJson["cloudsScale"] = 0.00003f;
    dimJson["cloudsDensity"] = 0.8f;
    dimJson["cloudsCover"] = 0.4f;
    dimJson["cloudWind"] = { 0.001f, 0.0f, 0.001f };

    auto materialSet = MaterialPresets::createOverworldSet();
    dimJson["materialSet"] = materialSetToJson(materialSet);

    json biomesJson = json::array();
    auto plains = BiomePresets::createPlainsBiome();
    auto mountains = BiomePresets::createMountainsBiome();
    biomesJson.push_back(biomeToJson(plains));
    biomesJson.push_back(biomeToJson(mountains));
    dimJson["biomes"] = biomesJson;

    json sunLight;
    sunLight["name"] = "SunLight";
    sunLight["type"] = 0;
    sunLight["enabled"] = true;
    sunLight["direction"] = { 0, -1, 0 };
    sunLight["color"] = { 1.0f, 1.0f, 0.9f };
    sunLight["intensity"] = 1.2f;

    json moonLight;
    moonLight["name"] = "MoonLight";
    moonLight["type"] = 0;
    moonLight["enabled"] = false;
    moonLight["direction"] = { 0, 1, 0 };
    moonLight["color"] = { 0.9f, 0.9f, 1.0f };
    moonLight["intensity"] = 0.1f;

	json celestialBodiesJson = json::array();

    json sunBody;
	sunBody["sourceLight"] = sunLight;
	sunBody["name"] = "Sun";
	sunBody["size"] = 5.0f;
	sunBody["softness"] = 0.3f;
	sunBody["texturePath"] = "assets/images/skybox/overworld/sun.png";
    sunBody["textureId"] = 0;

    json moonBody;
    moonBody["sourceLight"] = moonLight;
    moonBody["name"] = "Moon";
    moonBody["size"] = 2.75f;
    moonBody["softness"] = 0.15f;
    moonBody["texturePath"] = "assets/images/skybox/overworld/moon.png";
    moonBody["textureId"] = 0;

    celestialBodiesJson.push_back(sunBody);
    celestialBodiesJson.push_back(moonBody);

    dimJson["celestialBodies"] = celestialBodiesJson;
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