#include "skybox.hpp"
#include "glad.h"
#include "SFML/Graphics.hpp"
#include "renderer.hpp"
#include "dimensionManager.hpp"

static float smoothstep(float edge0, float edge1, float x) {
    float t = (x - edge0) / (edge1 - edge0);
    t = std::max(0.0f, std::min(1.0f, t));
    return t * t * (3.0f - 2.0f * t);
}

Skybox::Skybox() : VAO(0), VBO(0), cubemapTexture(0), renderer(nullptr) {}

Skybox::~Skybox() {
    cleanup();
}

bool Skybox::load(const std::vector<std::string>& faces) {
    if (faces.size() != 6) {
        std::cerr << "Skybox requires exactly 6 texture faces" << std::endl;
        return false;
    }

    glGenTextures(1, &cubemapTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

    for (unsigned int i = 0; i < faces.size(); i++) {
        sf::Image image;
        if (!image.loadFromFile(faces[i])) {
            std::cerr << "Failed to load skybox texture: " << faces[i] << std::endl;
            return false;
        }

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA,
            image.getSize().x, image.getSize().y, 0, GL_RGBA,
            GL_UNSIGNED_BYTE, image.getPixelsPtr());
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    const char* skyboxVertexSrc = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        out vec3 WorldPos;
        uniform mat4 projection;
        uniform mat4 view;
        void main() {
            WorldPos = aPos;
            vec4 pos = projection * view * vec4(aPos, 1.0);
            gl_Position = pos.xyww;
        }
    )";

    const char* skyboxFragmentSrc = R"(
        #version 330 core
        out vec4 FragColor;

        in vec3 WorldPos;

        uniform samplerCube skyboxTexture; // ваша кубическая карта
        uniform sampler2D moonTexture;
        uniform float timeOfDay;
        uniform vec3 sunDirection;
        uniform vec3 moonDirection;
        uniform float sunSize;
        uniform float moonSize;

        float sunHardness = 10.0;
        float moonHardness = 20.0;

        float drawCircle(vec3 dir, vec3 center, float radius, float hardness) {
            float cosAngle = dot(normalize(dir), normalize(center));
            float angle = acos(clamp(cosAngle, -1.0, 1.0));
            float circle = 1.0 - smoothstep(0.0, radius, angle);
            return pow(circle, hardness);
        }
        void main() {
            vec4 skyColor = texture(skyboxTexture, WorldPos);
    
            float sunIntensity = clamp(dot(sunDirection, vec3(0, 1, 0)) * 2.0, 0.0, 1.0);
            float sunMask = drawCircle(WorldPos, sunDirection, sunSize, sunHardness);
            vec3 sunColor = vec3(2.0, 1.9, 1.7) * sunIntensity;

            // Луна (текстура)
            // Создаём локальную систему координат для луны
            vec3 moonUp = vec3(0, 1, 0);
            vec3 moonRight = normalize(cross(moonDirection, moonUp));
            vec3 moonForward = moonDirection;
            moonUp = normalize(cross(moonRight, moonForward));
    
            // Проецируем мировые координаты на плоскость луны
            vec3 localPos = WorldPos - moonDirection * dot(WorldPos, moonDirection);
            float x = dot(localPos, moonRight);
            float y = dot(localPos, moonUp);
    
            // Текстурные координаты
            vec2 texCoord = vec2(
                x / (moonSize * 2.0) + 0.5,
                y / (moonSize * 2.0) + 0.5
            );
            float moonMask = drawCircle(WorldPos, moonDirection, moonSize, 20.0);
    
            if (moonMask > 0.0) {
                vec4 moonTexColor = texture(moonTexture, texCoord);
        
                // Фазы луны
                float moonPhase = dot(sunDirection, moonDirection);
                moonPhase = (moonPhase + 1.0) * 0.5; // 0..1
        
                // Освещение луны солнцем
                float illumination = dot(WorldPos, sunDirection) > 0 ? moonPhase : 0.0;
        
                vec3 finalMoonColor = moonTexColor.rgb * illumination * moonTexColor.a;
                vec3 finalColor = mix(skyColor.rgb, finalMoonColor, moonMask);
                FragColor = vec4(finalColor, 1.0);
            } else {
                vec3 finalColor = skyColor.rgb;
                finalColor = mix(finalColor, sunColor, sunMask);
                FragColor = vec4(finalColor, 1.0);
            }
        }
    )";

    if (!skyboxShader.loadFromSource(skyboxVertexSrc, skyboxFragmentSrc)) {
        std::cerr << "Failed to load skybox shader" << std::endl;
        return false;
    }

    float vertices[] = {
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,

         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,

         -1.0f,  1.0f, -1.0f,
          1.0f,  1.0f, -1.0f,
          1.0f,  1.0f,  1.0f,
          1.0f,  1.0f,  1.0f,
         -1.0f,  1.0f,  1.0f,
         -1.0f,  1.0f, -1.0f,

         -1.0f, -1.0f, -1.0f,
         -1.0f, -1.0f,  1.0f,
          1.0f, -1.0f, -1.0f,
          1.0f, -1.0f, -1.0f,
         -1.0f, -1.0f,  1.0f,
          1.0f, -1.0f,  1.0f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    sf::Image moonImg;
    if (!moonImg.loadFromFile("assets/images/skybox/overworld/moon.png")) {
        std::cerr << "Failed to load moon texture" << std::endl;
        return false;
    }

    glGenTextures(1, &moonTexture);
    glBindTexture(GL_TEXTURE_2D, moonTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
        moonImg.getSize().x, moonImg.getSize().y, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, moonImg.getPixelsPtr());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    std::cout << "Skybox initiliazed" << std::endl;
    return true;
}

void Skybox::render(std::shared_ptr<Camera> camera, std::shared_ptr<SourceLight> sunlight) {
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);

    skyboxShader.use();

    Matrix4 view = camera->getViewMatrix();
    view.data[12] = 0;
    view.data[13] = 0;
    view.data[14] = 0;
    Matrix4 proj = camera->getProjectionMatrix();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    skyboxShader.setUniform("skyboxTexture", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, moonTexture);
    skyboxShader.setUniform("moonTexture", 1);

    skyboxShader.setUniform("projection", proj);
    skyboxShader.setUniform("view", view);
    skyboxShader.setUniform("sunDirection", Vector3(-sunlight->direction.x, sunlight->direction.y, sunlight->direction.z));
    skyboxShader.setUniform("moonDirection", Vector3(sunlight->direction.x, -sunlight->direction.y, -sunlight->direction.z));
    skyboxShader.setUniform("timeOfDay", thisDim->timeOfDay);
    skyboxShader.setUniform("sunSize", sunSize);
    skyboxShader.setUniform("moonSize", moonSize);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
}

void Skybox::cleanup() const {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (cubemapTexture) glDeleteTextures(1, &cubemapTexture);
}

void Skybox::updateDayNightCycle(float deltaTime, std::shared_ptr<SourceLight> sunlight) {
    thisDim->timeOfDay += deltaTime * thisDim->daySpeed;
    if (thisDim->timeOfDay >= 1.0f) thisDim->timeOfDay -= 1.0f;

    int numKeyframes = sizeof(keyframes) / sizeof(keyframes[0]);
    int idx1 = 0, idx2 = 0;

    for (int i = 0; i < numKeyframes - 1; i++) {
        if (thisDim->timeOfDay >= keyframes[i].time && thisDim->timeOfDay <= keyframes[i + 1].time) {
            idx1 = i;
            idx2 = i + 1;
            break;
        }
    }

    float t = (thisDim->timeOfDay - keyframes[idx1].time) /
        (keyframes[idx2].time - keyframes[idx1].time);

    float sunHeight = keyframes[idx1].height * (1.0f - t) + keyframes[idx2].height * t;
    float intensity = keyframes[idx1].intensity * (1.0f - t) + keyframes[idx2].intensity * t;
    Vector3 color = keyframes[idx1].color * (1.0f - t) + keyframes[idx2].color * t;

    float angle = thisDim->timeOfDay * 2.0f * 3.14159f;

    Vector3 sunPos = Vector3(
        sin(angle) * sunOrbitRadius,
        sunHeight,
        cos(angle) * sunOrbitRadius
    ).normalized();
    Vector3 lightDir = Vector3(-sunPos.x, -sunPos.y, -sunPos.z).normalized();

    float sunCurve = smoothstep( -0.1f, 0.6f, sunHeight );
    float sunIntensity = intensity * sunCurve;

    sunlight->direction = lightDir;
    sunlight->color = color;
    sunlight->intensity = sunIntensity;
    sunlight->enableGodRays = sunIntensity > 0.35f;

    float ambientFactor = 0.05f + 0.5f * sunCurve;
    renderer->shader->use();
    renderer->shader->setUniform("ambientStrength", ambientFactor);
    float specular = 0.2f + 0.8f * sunCurve;
    renderer->shader->setUniform("specularStrength", specular);
    renderer->shader->setUniform("shininess", 64.0f);
}