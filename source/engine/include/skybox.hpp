#ifndef SKYBOX_HPP
#define SKYBOX_HPP

#include "shader.hpp"
#include "camera.hpp"
#include "SourceLight.hpp"
#include "memory"

class Renderer;
class Dimension;

struct CelestialBody;
struct Keyframe {
    float time; 
    float height;
    float intensity;
    Vector3 color;
};
const static Keyframe keyframes[] = {
    {0.0f,  -0.8f, 0.1f, Vector3(0.5f, 0.5f, 0.8f)},  // полночь (луна)
    {0.2f,  -0.3f, 0.15f, Vector3(0.7f, 0.65f, 0.8f)},  // предрассветные сумерки
    {0.25f,   0.25f, 0.3f, Vector3(1.0f, 0.75f, 0.5f)},  // рассвет
    {0.3f,   0.7f, 0.7f, Vector3(1.0f, 0.9f, 0.75f)},  // утро
    {0.4f,   1.2f, 0.95f, Vector3(1.0f, 1.0f, 1.0f)},  // полдень
    {0.7f,   0.7f, 0.85f, Vector3(1.0f, 0.95f, 0.95f)},  // после полудня
    {0.8f,   0.25f, 0.5f, Vector3(1.0f, 0.7f, 0.5f)},  // закат
    {0.85f,  -0.3f, 0.2f, Vector3(0.7f, 0.65f, 0.9f)},  // сумерки
    {1.0f,  -0.8f, 0.1f, Vector3(0.5f, 0.5f, 0.8f)}   // полночь
};

class Skybox {
private:
    unsigned int VAO, VBO;
    unsigned int cubemapTexture;
    Shader skyboxShader;
public:
    Skybox();
    ~Skybox();

    bool load(const std::vector<std::string>& faces, std::vector<std::shared_ptr<CelestialBody>>& bodies);
    bool loadCelestialTexture(const std::string& path, unsigned int& texID);
    void render(std::shared_ptr<Camera> camera, const std::vector<std::shared_ptr<CelestialBody>>& bodies);
    void cleanup() const;

    Renderer* renderer;
    std::shared_ptr<Dimension> thisDim;
    float sunOrbitRadius = 0.5f;

    float cloudTime = 0.0f;
    float cloudBottom = 800.0f;
    float cloudTop = 2500.0f;

    unsigned int noise3DTexture = 0;
    void generateNoiseTexture();

    unsigned int getTextureID() const { return cubemapTexture; }
    void updateDayNightCycle(float deltaTime);
    void updateClouds(float deltaTime);
};

#endif