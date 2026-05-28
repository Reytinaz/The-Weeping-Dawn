#ifndef SKYBOX_HPP
#define SKYBOX_HPP

#include "shader.hpp"
#include "camera.hpp"
#include "SourceLight.hpp"

class Renderer;
class Dimension;

struct Keyframe {
    float time; 
    float height;
    float intensity;
    Vector3 color;
};
const static Keyframe keyframes[] = {
    {0.0f,  -0.8f, 0.1f, Vector3(0.5f, 0.5f, 0.8f)},  // полночь (луна)
    {0.2f,  -0.3f, 0.15f, Vector3(0.7f, 0.6f, 0.9f)},  // предрассветные сумерки
    {0.3f,   0.2f, 0.3f, Vector3(1.0f, 0.7f, 0.5f)},  // рассвет
    {0.4f,   0.6f, 0.7f, Vector3(1.0f, 0.9f, 0.7f)},  // утро
    {0.5f,   0.8f, 1.0f, Vector3(1.0f, 1.0f, 0.9f)},  // полдень
    {0.6f,   0.6f, 0.85f, Vector3(1.0f, 0.9f, 0.7f)},  // после полудня
    {0.7f,   0.2f, 0.5f, Vector3(1.0f, 0.7f, 0.5f)},  // закат
    {0.8f,  -0.3f, 0.2f, Vector3(0.7f, 0.6f, 0.9f)},  // сумерки
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

    bool load(const std::vector<std::string>& faces);
    void render(std::shared_ptr<Camera> camera, std::shared_ptr<SourceLight> sunlight);
    void cleanup() const;

    Renderer* renderer;
    std::shared_ptr<Dimension> thisDim;

    bool isDay = true;
    float sunOrbitRadius = 0.6f;

    unsigned int moonTexture;
    unsigned int sunTexture;

    float sunSize = 0.25f;
    float moonSize = 0.2f;

    unsigned int getTextureID() const { return cubemapTexture; }
    void updateDayNightCycle(float deltaTime, std::shared_ptr<SourceLight> sunlight);
};

#endif