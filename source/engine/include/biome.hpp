#ifndef BIOME_HPP
#define BIOME_HPP

#include <string>
#include <memory>
#include <vector>
#include "material.hpp"

struct BiomeNoiseParams {
    float baseFreq;      // базова€ частота (0.01 - очень плавно, 0.02 - равнины, 0.05 - холмы)
    int octaves;         // количество октав (3-4 дл€ м€гких форм, 5-6 дл€ деталей)
    float persistence;   // персистенци€ (0.3 - гладко, 0.5 - средне)
    float amplitude;     // амплитуда (высота)

    float minHeight;
    float maxHeight;

    float roughness = 0.65f;      // шероховатость (0.3 - очень гладко, 1.0 - остро)
    float redistribution = 1.0f;  // перераспределение высот (>1 - сглаживает низины, <1 - сглаживает пики)
    float erosion = 0.2f;         // сила эрозии (0-1)

    enum NoiseType {
        STANDARD,    // обычный Perlin
        RIDGED,      // хребтовый (горы)
        BILLOW,      // округлый (холмы)
        TERRACES     // террасы
    } noiseType = STANDARD;
};

struct BiomeStructure {
    std::string modelPath;
    float density;
    float minHeight;
    float maxHeight;
    float minSlope;
    float maxSlope;
    Vector3 scale;
    Vector3 modelScale;
    float probability;
    float spacing;
};

struct BiomeBlend {
    std::shared_ptr<Biome> biome;
    float weight;
};

class Biome {
public:
    std::string name;
    BiomeNoiseParams noiseParams;
    std::shared_ptr<MaterialSet> materialSet;
    std::vector<BiomeStructure> structures;

    Vector3 debugColor;

    Biome(const std::string& name) : name(name), debugColor(1, 1, 1) {}

    void addStructure(const BiomeStructure& structure) {
        structures.push_back(structure);
    }
};

namespace BiomePresets {
    inline std::shared_ptr<Biome> createPlainsBiome() {
        auto biome = std::make_shared<Biome>("plains");
        biome->noiseParams = {
            0.015f,
            3,
            0.15f,
            7.0f,
            1.2f,
            -10.0f,
            5.0f,
            BiomeNoiseParams::BILLOW
        };
        biome->debugColor = Vector3(0.2f, 0.8f, 0.2f);

        auto materialSet = std::make_shared<MaterialSet>("plains_materials");
        materialSet->createMaterial("ground", -100.0f, -15.0f, "assets/images/terrain/ground.jpg", 0.3f, 0.3f, 12.0f);
        materialSet->createMaterial("grass", -15.0f, 100.0f, "assets/images/terrain/grass.jpg", 0.3f, 0.3f, 10.0f);

        biome->materialSet = materialSet;

        BiomeStructure tree;
        tree.modelPath = "assets/models/tree.obj";
        tree.density = 0.5f;
        tree.minHeight = -15.0f;
        tree.maxHeight = 15.0f;
        tree.minSlope = 0.0f;
        tree.maxSlope = 15.0f;
        tree.scale = Vector3(1.5f, 5.0f, 1.5f);
        tree.modelScale = Vector3(3.5f, 1.0f, 3.5f);
        tree.probability = 0.3f;
        tree.spacing = 6.0f;
        biome->addStructure(tree);

        return biome;
    }

    inline std::shared_ptr<Biome> createMountainsBiome() {
        auto biome = std::make_shared<Biome>("mountains");
        biome->noiseParams = {
            0.05f,     // baseFreq
            5,          // octaves
            0.2f,      // persistence
            30.0f,      // amplitude
            1.0f,       // redistribution
            0.0f,      // minHeight
            70.0f,      // maxHeight
            BiomeNoiseParams::RIDGED
        };
        biome->debugColor = Vector3(0.5f, 0.5f, 0.5f);

        auto materialSet = std::make_shared<MaterialSet>("mountains_materials");
        materialSet->createMaterial("ground", -40.0f, -5.0f, "assets/images/terrain/ground.jpg", 0.3f, 0.3f, 12.0f);
        materialSet->createMaterial("rock", -5.0f, 10.0f, "assets/images/terrain/rock.jpg", 0.3f, 0.3f, 14.0f);
        materialSet->createMaterial("snow", 10.0f, 50.0f, "assets/images/terrain/snow.jpg", 0.3f, 0.3f, 10.0f);

        biome->materialSet = materialSet;

        BiomeStructure rock;
        rock.modelPath = "assets/models/rock.obj";
        rock.density = 0.5f;
        rock.minHeight = 0.0f;
        rock.maxHeight = 60.0f;
        rock.minSlope = 20.0f;
        rock.maxSlope = 60.0f;
        rock.scale = Vector3(1.0f, 1.0f, 1.0f);
        rock.modelScale = Vector3(3.0f, 1.25f, 3.0f);
        rock.probability = 0.25f;
        rock.spacing = 5.0f;
        biome->addStructure(rock);

        return biome;
    }
}

#endif