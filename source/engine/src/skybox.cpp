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

bool Skybox::load(const std::vector<std::string>& faces, std::vector<std::shared_ptr<CelestialBody>>& bodies) {
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
        uniform samplerCube skyboxTexture;
        #define MAX_CELESTIAL_BODIES 4

        uniform sampler2D celestialTextures[MAX_CELESTIAL_BODIES];
        uniform vec3 celestialDirections[MAX_CELESTIAL_BODIES];
        uniform float celestialSizes[MAX_CELESTIAL_BODIES];
        uniform float celestialSoftness[MAX_CELESTIAL_BODIES];
        uniform int numCelestialBodies;

        uniform vec3 sunColor;
        uniform vec3 skyColor;

        uniform bool cloudsEnabled;
        uniform sampler3D noiseTexture;
        uniform float cloudWindX, cloudWindY, cloudWindZ;
        uniform float cloudTime;
        uniform float cloudScale;
        uniform float cloudBottom, cloudTop, cloudDensity, cloudCover;
        uniform vec3  cameraWorldPos;

        float sampleCloudDensity(vec3 pos) {
            vec3 p = pos * cloudScale + vec3(cloudWindX, cloudWindY, cloudWindZ) * cloudTime;
            float noise = texture(noiseTexture, p).r;
            float detail = texture(noiseTexture, p * 2.5 + vec3(0.5)).r * 0.3;
            return noise + detail;
        }

        uniform float sunGlowIntensity; 

        void main() {
            vec3 viewDir = normalize(WorldPos);
            vec3 result = texture(skyboxTexture, viewDir).rgb * skyColor;

            for (int i = 0; i < numCelestialBodies; ++i) {
                vec3 bodyDir = normalize(celestialDirections[i]);
                float bodySize = celestialSizes[i];

                vec3 right = normalize(cross(bodyDir, vec3(0.0, 1.0, 0.0)));
                if (length(right) < 0.001) right = vec3(1.0, 0.0, 0.0);
                vec3 up = normalize(cross(right, bodyDir));

                float cosAngle = dot(viewDir, bodyDir);
                float cosSize  = cos(radians(bodySize));

                if (cosAngle > cosSize) {
                    float dx = dot(viewDir, right);
                    float dy = dot(viewDir, up);
                    float maxOffset = sin(radians(bodySize));

                    vec2 uv = vec2(0.5);
                    if (maxOffset > 0.0001) {
                        uv = vec2(0.5) + 0.5 * vec2(dx, dy) / maxOffset;
                    }
                    uv = clamp(uv, 0.0, 1.0);

                    vec4 tex = texture(celestialTextures[i], uv);
                    if (i == 0) tex.rgb *= sunColor;

                    float brightness = dot(tex.rgb, vec3(0.333));
                    float texAlpha = (brightness < 0.1) ? 0.0 : tex.a;

                    float dist = length(uv - 0.5) * 2.0; 
                    float edgeSoftness = celestialSoftness[i];
                    float alpha = texAlpha * (1.0 - smoothstep(1.0 - edgeSoftness, 1.0, dist));

                    result = mix(result, tex.rgb, alpha);
                }
            }

            vec3 sunDir = normalize(celestialDirections[0]);
            float sunGlowAngle = dot(viewDir, sunDir);

            float glowRadius = 25.0;
            float glowCos = cos(radians(glowRadius));
            if (sunGlowAngle > glowCos) {
                float t2 = (sunGlowAngle - glowCos) / (1.0 - glowCos);
                t2 = pow(t2, 2.0);
                vec3 glow = sunColor * t2 * 1.2 * sunGlowIntensity;
                result += glow * 0.4;
            }

            float wideGlowRadius = 40.0;
            float wideGlowCos = cos(radians(wideGlowRadius));
            if (sunGlowAngle > wideGlowCos) {
                float t3 = (sunGlowAngle - wideGlowCos) / (1.0 - wideGlowCos);
                t3 = pow(t3, 3.0);
                vec3 wideGlow = sunColor * t3 * 0.6 * min(sunGlowIntensity, 0.6);
                result += wideGlow * 0.3;
            }

            vec3 moonDir = normalize(celestialDirections[1]);
            float moonGlowAngle = dot(viewDir, moonDir);
            float moonGlowRadius = 10.0;
            float moonGlowCos = cos(radians(moonGlowRadius));
            if (moonGlowAngle > moonGlowCos) {
                float t2 = (moonGlowAngle - moonGlowCos) / (1.0 - moonGlowCos);
                t2 = pow(t2, 2.0);
                vec3 glow = vec3(0.9, 0.9, 1) * t2;
                result += glow * 0.3;
            }

            
            if (cloudsEnabled) {
                vec3 rayOrigin = cameraWorldPos;
                float totalDensity = 0.0;
                float dist = 0.0;
                float stepSize = 60.0; 
                int maxSteps = 25;
                float maxDist = 5500.0;

                bool skipClouds = false;
                if (cameraWorldPos.y > cloudTop && viewDir.y > 0.0) skipClouds = true;
                if (cameraWorldPos.y < cloudBottom && viewDir.y < 0.0) skipClouds = true;

                if (!skipClouds) {
                    if (rayOrigin.y < cloudBottom) {
                        dist = (cloudBottom - rayOrigin.y) / max(viewDir.y, 0.001);
                    }

                    for (int i = 0; i < maxSteps; ++i) {
                        if (dist > maxDist || totalDensity > 2.0) break;

                        vec3 samplePos = rayOrigin + viewDir * dist;
                        float height = samplePos.y;
                        if (height > cloudTop) break;

                        float heightFactor = smoothstep(cloudBottom, cloudBottom + 150.0, height) *
                                                (1.0 - smoothstep(cloudTop - 150.0, cloudTop, height));

                        float density = sampleCloudDensity(samplePos) * heightFactor * cloudDensity;
                        density -= (1.0 - cloudCover);
                        density = max(density, 0.0);

                        totalDensity += density * stepSize * 0.0015;
                        dist += stepSize;
                    }

                    float cloudAlpha = 1.0 - exp(-totalDensity * 1.5);
                    cloudAlpha = clamp(cloudAlpha, 0.0, 1.0);

                    vec3 sunDir = normalize(celestialDirections[0]);
                    float sunHeight = sunDir.y;
                    float sunsetFactor = 1.0 - smoothstep(0.1, 0.5, sunHeight);
                    vec3 sunsetCloudColor = sunColor * 2.0;
                    vec3 cloudColor = mix(vec3(1.0), sunsetCloudColor, sunsetFactor);

                    float dayFactor = smoothstep(-0.4, 0.1, sunHeight);
                    cloudAlpha *= max(dayFactor, 0.4);
                    cloudColor *= max(dayFactor, 0.2);

                    result = mix(result, cloudColor, cloudAlpha);
                }
            }

            FragColor = vec4(result, 1.0);
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

    for (auto& body : bodies) {
        if (!loadCelestialTexture(body->texturePath, body->textureId)) {
            std::cout << "Error was occured while loading celestial body texture" << std::endl;
        }
    }
    generateNoiseTexture();
    /*if (thisDim->hasClouds) {
        if (!loadCloudTexture("assets/images/skybox/clouds.png")) {
            std::cout << "Error was occured while loading clouds texture" << std::endl;
        }
    }*/

    std::cout << "Skybox initiliazed" << std::endl;
    return true;
}

bool Skybox::loadCelestialTexture(const std::string& path, unsigned int& texID) {
    sf::Image image;
    if (!image.loadFromFile(path)) return false;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.getSize().x, image.getSize().y, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, image.getPixelsPtr());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return true;
}

void Skybox::generateNoiseTexture() {
    const int SIZE = 64;
    std::vector<GLubyte> data(SIZE * SIZE * SIZE * 4);

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    for (int z = 0; z < SIZE; ++z) {
        for (int y = 0; y < SIZE; ++y) {
            for (int x = 0; x < SIZE; ++x) {
                int idx = (z * SIZE * SIZE + y * SIZE + x) * 4;
                float val = dist(rng);
                data[idx + 0] = static_cast<GLubyte>(val * 255);
                data[idx + 1] = static_cast<GLubyte>(val * 255);
                data[idx + 2] = static_cast<GLubyte>(val * 255);
                data[idx + 3] = 255;
            }
        }
    }

    glGenTextures(1, &noise3DTexture);
    glBindTexture(GL_TEXTURE_3D, noise3DTexture);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, SIZE, SIZE, SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
}

void Skybox::render(std::shared_ptr<Camera> camera,
    const std::vector<std::shared_ptr<CelestialBody>>& bodies)
{
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    skyboxShader.use();

    Matrix4 view = camera->getViewMatrix();
    view.data[12] = 0; view.data[13] = 0; view.data[14] = 0;
    Matrix4 proj = camera->getProjectionMatrix();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    skyboxShader.setUniform("skyboxTexture", 0);

    int count = std::min((int)bodies.size(), 4);
    skyboxShader.setUniform("numCelestialBodies", count);
    skyboxShader.setUniform("projection", proj);
    skyboxShader.setUniform("view", view);
    skyboxShader.setUniform("skyColor", thisDim->skyColor);
    skyboxShader.setUniform("sunGlowIntensity", thisDim->sunGlowIntensity);
    skyboxShader.setUniform("cloudsEnabled", thisDim->hasClouds);
    if (thisDim->hasClouds) {
        glActiveTexture(GL_TEXTURE5); // свободный юнит (убедитесь, что не занят)
        glBindTexture(GL_TEXTURE_3D, noise3DTexture);
        skyboxShader.setUniform("noiseTexture", 5);
        skyboxShader.setUniform("cloudWindX", thisDim->cloudsWind.x);
        skyboxShader.setUniform("cloudWindY", thisDim->cloudsWind.y);
        skyboxShader.setUniform("cloudWindZ", thisDim->cloudsWind.z);
        skyboxShader.setUniform("cloudTime", cloudTime);
        skyboxShader.setUniform("cloudBottom", cloudBottom);
        skyboxShader.setUniform("cloudTop", cloudTop);
        skyboxShader.setUniform("cloudDensity", thisDim->cloudsDensity);
        skyboxShader.setUniform("cloudCover", thisDim->cloudsCover);
        skyboxShader.setUniform("cloudScale", thisDim->cloudsScale);
        skyboxShader.setUniform("cameraWorldPos", camera->position);
    }

    for (int i = 0; i < count; ++i) {
        const auto& body = bodies[i];
        if (!body) continue;

        Vector3 dir = Vector3(body->sourceLight->direction.x, -body->sourceLight->direction.y, -body->sourceLight->direction.z);
        if (dir.x == 0 || dir.x == -0) dir.x = 0.01f;
        if (dir.z == 0 || dir.z == -0) dir.z = 0.01f;

        glActiveTexture(GL_TEXTURE1 + i);
        glBindTexture(GL_TEXTURE_2D, body->textureId);
        skyboxShader.setUniform("celestialTextures[" + std::to_string(i) + "]", 1 + i);
        skyboxShader.setUniform("celestialDirections[" + std::to_string(i) + "]", dir);
        skyboxShader.setUniform("celestialSizes[" + std::to_string(i) + "]", body->size);
        skyboxShader.setUniform("celestialSoftness[" + std::to_string(i) + "]", body->softness);
        if (body->name == "Sun" && body->sourceLight) {
            skyboxShader.setUniform("sunColor", body->sourceLight->color);
        }
    }

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

void Skybox::updateDayNightCycle(float deltaTime) {
    if (!thisDim || !renderer) return;

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

    float sunCurve = smoothstep(-0.1f, 0.6f, sunHeight);
    float sunIntensity = intensity * sunCurve;
    thisDim->isDay = (sunHeight > 0.0f);
	if (thisDim->isDay) thisDim->mainLightSource = "Sun";
	else thisDim->mainLightSource = "Moon";

    for (auto& body : thisDim->celestialBodies) {
        if (!body || !body->sourceLight) continue;

        if (body->name == "Sun") {
            body->sourceLight->direction = lightDir;
            body->sourceLight->color = color;
            body->sourceLight->intensity = sunIntensity;
            body->sourceLight->enabled = thisDim->isDay;
        }
        else if (body->name == "Moon") {
            body->sourceLight->direction = -lightDir;
            body->sourceLight->enabled = !thisDim->isDay;
        }
    }

    Vector3 daySkyColor = color * 0.8f + Vector3(0.2f, 0.2f, 0.3f);
    float sunHeightNorm = (sunHeight + 1.0f) * 0.5f;
    float horizonFactor = 1.0f - abs(sunHeight) * 2.0f;
    horizonFactor = std::clamp(horizonFactor, 0.0f, 1.0f);
    horizonFactor = smoothstep(0.0f, 1.0f, horizonFactor);

    float dayFactor = smoothstep(0.1f, 0.6f, sunHeight);
    float nightFactor = smoothstep(-0.2f, -0.4f, sunHeight);

    thisDim->skyColor = thisDim->nightSkyColor * nightFactor
        + thisDim->dawnSkyColor * horizonFactor * (1.0f - nightFactor)
        + daySkyColor * dayFactor * (1.0f - nightFactor - horizonFactor * 0.5f);
    thisDim->skyColor = thisDim->skyColor * (0.8f + 0.2f * horizonFactor);

    float glowIntensity = exp(-abs(sunHeight) * 3.0f) * 2.0f + 0.35f;
    thisDim->sunGlowIntensity = glowIntensity;

    float ambientFactor = std::max(0.5f * sunCurve, 0.12f);
    renderer->shader->use();
    renderer->shader->setUniform("ambientStrength", ambientFactor);
}

void Skybox::updateClouds(float deltaTime) {
    cloudTime += deltaTime;
}