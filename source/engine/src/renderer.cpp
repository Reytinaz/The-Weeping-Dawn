#include "renderer.hpp"
#include "glad.h"

Renderer::Renderer(int w, int h) : width(w), height(h), shadowMap(nullptr), shader(nullptr), chunkManager(nullptr), postProcess(nullptr) {}

Renderer::~Renderer() {
    delete shadowMap;
    delete shader;
    delete postProcess;
    if (depthFBO) glDeleteFramebuffers(1, &depthFBO);
    if (depthTexture) glDeleteTextures(1, &depthTexture);
}

bool Renderer::init() {
    shader = new Shader();
    shadowMap = new ShadowMap(4096);
    postProcess = new PostProcess(width, height);

    std::string vertex = R"(
        #version 330 core

        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aTexCoord;
        layout (location = 3) in float aMaterial1;
        layout (location = 4) in float aMaterial2;
        layout (location = 5) in float aMaterial3;

        out vec3 FragPos;
        out vec3 Normal;
        out vec2 TexCoord;
        out vec4 FragPosLightSpace[)";
            vertex += std::to_string(MAX_CASCADES);
            vertex += R"(];
        out float ViewDepth;
        out float WorldHeight;

        out float vMaterial1;
        out float vMaterial2;
        out float vMaterial3;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        uniform mat4 lightSpaceMatrices[)";
            vertex += std::to_string(MAX_CASCADES);
            vertex += R"(];

        void main() {
            vec4 worldPos = model * vec4(aPos, 1.0);
            vec4 viewPos = view * worldPos;
    
            FragPos = worldPos.xyz;
            Normal = mat3(transpose(inverse(model))) * aNormal;
            TexCoord = aTexCoord;
    
            vMaterial1 = aMaterial1;
            vMaterial2 = aMaterial2;
            vMaterial3 = aMaterial3;
    
            ViewDepth = -viewPos.z;
            WorldHeight = worldPos.y;
    
            // ¬ычисл€ем позиции в пространстве света дл€ каждого каскада
            for (int i = 0; i < )";
            vertex += std::to_string(MAX_CASCADES);
            vertex += R"(; ++i) {
                FragPosLightSpace[i] = lightSpaceMatrices[i] * worldPos;
            }
    
            gl_Position = projection * viewPos;
        }
    )";

    std::string fragment = R"(
        #version 330 core
        out vec4 FragColor;

        in vec2 TexCoord;
        in vec3 FragPos;
        in vec3 Normal;
        in vec4 FragPosLightSpace[)";
            fragment += std::to_string(MAX_CASCADES);
            fragment += R"(];
        in float ViewDepth;
        in float WorldHeight;

        in float vMaterial1;
        in float vMaterial2;
        in float vMaterial3;

        uniform sampler2D shadowMaps[)";
            fragment += std::to_string(MAX_CASCADES);
            fragment += R"(];
        uniform mat4 lightSpaceMatrices[)";
            fragment += std::to_string(MAX_CASCADES);
            fragment += R"(];
        uniform float cascadeSplits[)";
            fragment += std::to_string(MAX_CASCADES + 1);
            fragment += R"(];
        uniform int numCascades;

        uniform sampler2D diffuseTexture;
        uniform samplerCube skybox;
        uniform float reflectivity;
        uniform vec3 viewPos;
        uniform vec3 objectColor;
        uniform float ambientStrength;
        uniform float specularStrength;
        uniform float shininess;
        uniform float bias;
        uniform float gamma;
        uniform float cascadeRadii[)";
            fragment += std::to_string(MAX_CASCADES);
            fragment += R"(];
        uniform float cascadeBias[)";
            fragment += std::to_string(MAX_CASCADES);
            fragment += R"(];

        uniform int debugShadow;

        #define MAX_LIGHTS 8
        struct Light {
            int type;
            vec3 position;
            vec3 direction;
            vec3 color;
            float intensity;
            float constant;
            float linear;
            float quadratic;
            float cutOff;
            float outerCutOff;
        };
        uniform Light lights[MAX_LIGHTS];
        uniform int numLights;

        uniform sampler2D materialTextures[16];
        uniform vec3 materialColors[16];
        uniform float materialUVScale[16];
        uniform float materialRoughness[16];
        uniform int materialCount;

        uniform float chunkSize = 16.0;
        uniform int isTerrain;

        float shadowCalculation(vec3 projCoords, int cascadeIndex) {
            if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0 || projCoords.z > 1.0)
                return 1.0; // outside shadow map -> treat as lit

            float currentDepth = projCoords.z;
            float shadow = 0.0;
            vec2 texelSize = 1.0 / textureSize(shadowMaps[cascadeIndex], 0);
            float radius = cascadeRadii[cascadeIndex];
            // moderate PCF sampling counts for stability
            int samples = (cascadeIndex == 0) ? 3 : 2;

            for (int x = -samples; x <= samples; ++x) {
                for (int y = -samples; y <= samples; ++y) {
                    vec2 offset = vec2(x, y) * texelSize * radius;
                    float closestDepth = texture(shadowMaps[cascadeIndex], projCoords.xy + offset).r;
                    float biasNew = cascadeBias[cascadeIndex];
                    shadow += currentDepth - biasNew > closestDepth ? 1.0 : 0.0;
                }
            }
            shadow /= ( (2*samples+1)*(2*samples+1) );
            return 1.0 - shadow;
        }

        vec3 calcLightContribution(Light light, vec3 norm, vec3 fragPos, vec3 viewDir, out float diffOut) {
            vec3 lightDir;
            float attenuation = 1.0;
            float intensity = light.intensity;

            if (light.type == 0) {
                // light.direction is stored as direction of rays (from light towards scene)
                // for lighting calculations we need vector from fragment to light -> invert
                lightDir = normalize(-light.direction);
            } else if (light.type == 1) {
                vec3 lightVec = light.position - fragPos;
                float dist = length(lightVec);
                lightDir = lightVec / dist;
                attenuation = 1.0 / (light.constant + light.linear * dist + light.quadratic * dist * dist);
            } else if (light.type == 2) {
                vec3 lightVec = light.position - fragPos;
                float dist = length(lightVec);
                lightDir = lightVec / dist;
                float theta = dot(lightDir, normalize(-light.direction));
                float epsilon = light.cutOff - light.outerCutOff;
                float spotFactor = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
                attenuation = spotFactor / (light.constant + light.linear * dist + light.quadratic * dist * dist);
            }

            float diff = max(dot(norm, lightDir), 0.0);
            diffOut = diff;
            vec3 diffuse = diff * light.color * intensity * attenuation;
            vec3 halfDir = normalize(lightDir + viewDir);
            float spec = pow(max(dot(norm, halfDir), 0.0), shininess);
            vec3 specular = specularStrength * spec * light.color * intensity * attenuation;
            return diffuse + specular;
         }

         vec4 getMaterialColor(out float outRoughness) {
            if (isTerrain == 0) {
                vec2 flippedUV = vec2(TexCoord.x, 1.0 - TexCoord.y);
                return texture(diffuseTexture, flippedUV);
            }

            float minMat = min(min(vMaterial1, vMaterial2), vMaterial3);
            float maxMat = max(max(vMaterial1, vMaterial2), vMaterial3);
            float avgMat = (vMaterial1 + vMaterial2 + vMaterial3) / 3.0;
    
            if (abs(maxMat - minMat) < 0.1) {
                int matIdx = int(round(avgMat));
                matIdx = clamp(matIdx, 0, materialCount - 1);
        
                // »спользуем UV scale конкретного материала
                float uvScale = materialUVScale[matIdx];
                vec2 scaledUV = TexCoord * (chunkSize / uvScale);
        
                vec4 color = texture(materialTextures[matIdx], scaledUV);
                if (color.r + color.g + color.b < 0.01) {
                    color = vec4(materialColors[matIdx], 1.0);
                }
                outRoughness = materialRoughness[matIdx];
                return color;
            }
    
            int matIdx1 = int(round(minMat));
            int matIdx2 = int(round(maxMat));
    
            float t = (avgMat - minMat) / (maxMat - minMat);
            t = clamp(t, 0.0, 1.0);
    
            matIdx1 = clamp(matIdx1, 0, materialCount - 1);
            matIdx2 = clamp(matIdx2, 0, materialCount - 1);
    
            // »спользуем UV scale дл€ каждого материала отдельно
            float uvScale1 = materialUVScale[matIdx1];
            float uvScale2 = materialUVScale[matIdx2];
    
            vec2 scaledUV1 = TexCoord * (chunkSize / uvScale1);
            vec2 scaledUV2 = TexCoord * (chunkSize / uvScale2);
    
            vec4 color1 = texture(materialTextures[matIdx1], scaledUV1);
            if (color1.r + color1.g + color1.b < 0.01) {
                color1 = vec4(materialColors[matIdx1], 1.0);
            }
    
            vec4 color2 = texture(materialTextures[matIdx2], scaledUV2);
            if (color2.r + color2.g + color2.b < 0.01) {
                color2 = vec4(materialColors[matIdx2], 1.0);
            }
            // interpolate roughness between materials
            float r1 = materialRoughness[matIdx1];
            float r2 = materialRoughness[matIdx2];
            outRoughness = mix(r1, r2, t);
            return mix(color1, color2, t);
        }

        void main() {
            vec4 materialColor = getMaterialColor();
            if (materialColor.a < 0.1) discard;

            vec3 norm = normalize(Normal);
            vec3 viewDir = normalize(viewPos - FragPos);

            int cascadeIndex = 0;
            for (int i = 0; i < numCascades - 1; ++i) {
                if (ViewDepth > cascadeSplits[i + 1]) cascadeIndex = i + 1;
            }

            vec4 fragPosLightSpace = lightSpaceMatrices[cascadeIndex] * vec4(FragPos, 1.0);
            vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
            projCoords = projCoords * 0.5 + 0.5;

            float shadowFactor = shadowCalculation(projCoords, cascadeIndex);
            if (debugShadow == 1) {
                // Visualize shadow intensity for debugging: dark = in shadow
                FragColor = vec4(vec3(shadowFactor), 1.0);
                return;
            }

            vec3 lighting = vec3(0.0);
            float sunDiff = 0.0;
            for (int i = 0; i < numLights; ++i) {
                float diffFactor;
                vec3 contrib = calcLightContribution(lights[i], norm, FragPos, viewDir, diffFactor);
                if (i == 0 && lights[i].type == 0) sunDiff = diffFactor;
                lighting += contrib;
            }

            vec3 ambient = ambientStrength * vec3(1.0);
            //vec3 diffuseColor = texColor.rgb * objectColor;
            vec3 directLight = (ambient + shadowFactor * lighting) * materialColor.rgb;

            float reflectionStrength = reflectivity * pow(sunDiff, 0.5);
            vec3 incident = normalize(FragPos - viewPos);
            vec3 reflection = reflect(incident, norm);
            vec3 skyReflection = texture(skybox, reflection).rgb;
            vec3 finalColor = mix(directLight, skyReflection, reflectionStrength);
            FragColor = vec4(finalColor, materialColor.a);

            //FragColor.rgb = pow(finalColor, vec3(1.0 / gamma));
            //UV TEST
            //FragColor = vec4(TexCoord, 0.0, 1.0);
            //CASCADES TEST
            //if (cascadeIndex == 0) FragColor = vec4(1,0,0,1);
            //else if (cascadeIndex == 1) FragColor = vec4(0,1,0,1);
            //else if (cascadeIndex == 2) FragColor = vec4(0,0,1,1);
            //else FragColor = vec4(1,1,1,1);
            //VIEW DEPTH TEST
            //float depth = ViewDepth / 100.0f; // нормируем
            //FragColor = vec4(depth, depth, depth, 1.0);
        }
    )";

    if (!shader->loadFromSource(vertex, fragment)) {
        std::cerr << "Failed to load shaders" << std::endl;
        return false;
    }
    if (!shadowMap->init()) {
        std::cerr << "Failed to init shadow map" << std::endl;
        delete shadowMap;
        shadowMap = nullptr;
        return false;
    }
    if (!postProcess->init()) {
        std::cerr << "Failed to init post process" << std::endl;
        delete postProcess;
        postProcess = nullptr;
        return false;
    }

    shader->use();
    shader->setUniform("ambientStrength", 0.3f);
    shader->setUniform("specularStrength", 0.0f);
    shader->setUniform("shininess", 32.0f);
    shader->setUniform("reflectivity", 0.0f);
    shader->setUniform("bias", 0.001f);
    shader->setUniform("gamma", 2.2f);
    shader->setUniform("skybox", 2);
    shader->setUniform("diffuseTexture", 1);
    shader->setUniform("chunkSize", 16.0f);

    //glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_DEPTH_TEST);
    glCullFace(GL_BACK);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    glGenTextures(1, &whiteTexture);
    glBindTexture(GL_TEXTURE_2D, whiteTexture);
    unsigned char whitePixel[] = { 255, 255, 255, 255 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glGenFramebuffers(1, &sunMaskFbo);
    glGenTextures(1, &sunMaskTexture);
    glBindTexture(GL_TEXTURE_2D, sunMaskTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER, sunMaskFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sunMaskTexture, 0);

    glGenFramebuffers(1, &moonMaskFbo);
    glGenTextures(1, &moonMaskTexture);
    glBindTexture(GL_TEXTURE_2D, moonMaskTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER, moonMaskFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, moonMaskTexture, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    std::cout << "Renderer initilized successfuly" << std::endl;
    return true;
}

void Renderer::setSize(int w, int h) {
    width = w; height = h;
    if (camera) camera->aspectRatio = static_cast<float>(w) / h;
    glViewport(0, 0, w, h);
}

void Renderer::setDebug(bool b) {
    debugEnabled = b;
}

void Renderer::clear() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::render(const std::vector<std::shared_ptr<Object3D>>& objects, const std::vector<std::shared_ptr<SourceLight>>& lights, float viewDistance, float gamma) {
    if (!shadowMap || !camera || !postProcess) return;

    std::shared_ptr<SourceLight> globalLight;
    bool hasGL = false;

    for (const auto& light : lights) {
        if (light->type == 0) {
            if (!hasGL) {
                globalLight = light;
                hasGL = true;
            }
        }
    }
    if (!hasGL) return;

    shadowMap->render(objects, *camera, globalLight, viewDistance * 100.0f);

    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    clear();
    shader->use();

    Matrix4 view = camera->getViewMatrix();
    Matrix4 proj = camera->getProjectionMatrix();
    shader->setUniform("view", view);
    shader->setUniform("projection", proj);
    shader->setUniform("viewPos", camera->position);
    shader->setUniform("gamma", gamma);

    const unsigned int shadowUnitStart = 5;
    shadowMap->bindCascadeTextures(shadowUnitStart);

    for (int i = 0; i < MAX_CASCADES; ++i) {
        std::string name = "shadowMaps[" + std::to_string(i) + "]";
        shader->setUniform(name, (int)(shadowUnitStart + i));
    }

    const auto& cascades = shadowMap->getCascades();
    for (int i = 0; i < MAX_CASCADES; i++) {
        std::string name = "lightSpaceMatrices[" + std::to_string(i) + "]";
        shader->setUniform(name, cascades[i].lightSpaceMatrix);
    }

    const auto& distances = shadowMap->getCascadeDistances();
    for (int i = 0; i <= MAX_CASCADES; i++) {
        shader->setUniform(("cascadeSplits[" + std::to_string(i) + "]").c_str(), distances[i]);
    }

    float cascadeRadii[4] = { 0.5f, 1.0f, 1.5f, 2.0f };
    for (int i = 0; i < MAX_CASCADES; ++i) {
        shader->setUniform(("cascadeRadii[" + std::to_string(i) + "]").c_str(), cascadeRadii[i]);
    }

    float cascadeBias[4] = { 0.005f, 0.002f, 0.001f, 0.001f };
    for (int i = 0; i < MAX_CASCADES; ++i) {
        shader->setUniform(("cascadeBias[" + std::to_string(i) + "]").c_str(), cascadeBias[i]);
    }

    shader->setUniform("numCascades", MAX_CASCADES);
    shader->setUniform("lightDir", globalLight->direction.x, globalLight->direction.y, globalLight->direction.z);
    shader->setUniform("debugShadow", 0);
    shader->setUniform("numLights", (int)lights.size());

    short lightNum = 0;
    for (auto& light : lights) {
        std::string base = "lights[" + std::to_string(lightNum) + "]";
        shader->setUniform(base + ".type", light->type);
        shader->setUniform(base + ".position", light->position);
        shader->setUniform(base + ".direction", light->direction);
        shader->setUniform(base + ".color", light->color);
        shader->setUniform(base + ".intensity", light->intensity);
        shader->setUniform(base + ".constant", light->constant);
        shader->setUniform(base + ".linear", light->linear);
        shader->setUniform(base + ".quadratic", light->quadratic);

        float cutOffRad = light->cutOff * 3.14159f / 180.0f;
        float outerCutOffRad = light->outerCutOff * 3.14159f / 180.0f;
        shader->setUniform(base + ".cutOff", cos(cutOffRad));
        shader->setUniform(base + ".outerCutOff", cos(outerCutOffRad));
        lightNum++;
    }

    shader->setUniform("isTerrain", 0);
    for (auto& obj : objects) {
        if (!obj->visible) continue;
        shader->setUniform("model", obj->getTransform());
        shader->setUniform("objectColor", obj->color[0], obj->color[1], obj->color[2]);

        glActiveTexture(GL_TEXTURE1);
        if (obj->diffuseTexture) {
            glBindTexture(GL_TEXTURE_2D, obj->diffuseTexture->getId());
        }
        else {
            glBindTexture(GL_TEXTURE_2D, whiteTexture);
        }
        shader->setUniform("diffuseTexture", 1);

        obj->render();
    }

    shader->setUniform("isTerrain", 1);
    if (chunkManager) {
        auto chunks = chunkManager->getVisibleChunks();
        shader->setUniform("model", Matrix4::identity());
        shader->setUniform("objectColor", 1.0f, 1.0f, 1.0f);

        for (const auto& chunk : chunks) {
            auto materialSet = chunk->getMaterialSet();
            if (materialSet) {
                int count = materialSet->size();
                shader->setUniform("materialCount", count);

                for (int i = 0; i < count && i < 16; ++i) {
                    const auto& mat = materialSet->materials[i];
                    if (mat->hasTexture && mat->getTextureId() == 0) {
                        mat->loadTexture();
                    }
                    std::string texUniform = "materialTextures[" + std::to_string(i) + "]";
                    std::string uvUniform = "materialUVScale[" + std::to_string(i) + "]";
                    std::string colUniform = "materialColors[" + std::to_string(i) + "]";

                    if (mat->hasTexture && mat->getTextureId() != 0) {
                        glActiveTexture(GL_TEXTURE10 + i);
                        glBindTexture(GL_TEXTURE_2D, mat->getTextureId());
                        shader->setUniform(texUniform, 10 + i);
                    }
                    else {
                        glActiveTexture(GL_TEXTURE10 + i);
                        glBindTexture(GL_TEXTURE_2D, whiteTexture);
                        shader->setUniform(texUniform, 10 + i);
                    }
                    shader->setUniform(colUniform, mat->color.x, mat->color.y, mat->color.z);
                    shader->setUniform(uvUniform, mat->uvScale);
                }
            }

            if (chunk->getDiffuseTexture()) {
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, chunk->getDiffuseTexture()->getId());
            }
            chunk->draw();
        }
    }

    if (debugEnabled) {
        debugRenderCascade(0);
    }
}

void Renderer::debugRenderCascade(int cascadeIndex) const {
    if (!shadowMap || cascadeIndex >= MAX_CASCADES) return;

    glViewport(0, 0, 200, 200);
    glDisable(GL_DEPTH_TEST);

    static Shader debugShader;
    static bool debugInit = false;
    if (!debugInit) {
        const char* vs = R"(
            #version 330 core
            layout (location = 0) in vec2 aPos;
            out vec2 TexCoords;
            void main() {
                TexCoords = aPos * 0.5 + 0.5;
                gl_Position = vec4(aPos, 0.0, 1.0);
            }
        )";
        const char* fs = R"(
            #version 330 core
            uniform sampler2D shadowMap;
            in vec2 TexCoords;
            out vec4 FragColor;
            void main() {
                float depth = texture(shadowMap, TexCoords).r;
                FragColor = vec4(vec3(depth), 1.0);
            }
        )";
        debugShader.loadFromSource(vs, fs);
        debugInit = true;
    }

    debugShader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, shadowMap->getCascades()[cascadeIndex].depthTex);
    debugShader.setUniform("shadowMap", 0);

    static float quadVertices[] = { -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f };
    static unsigned int quadVAO = 0, quadVBO = 0;
    if (quadVAO == 0) {
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);
    }

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, width, height);
}

Vector2 Renderer::getScreenPosition(const Vector3& worldPos) {
    Matrix4 view = camera->getViewMatrix();
    Matrix4 proj = camera->getProjectionMatrix();

    Vector4 clipPos = proj * view * Vector4(worldPos.x, worldPos.y, worldPos.z, 1.0f);

    if (clipPos.w == 0) return Vector2(0, 0);
    Vector3 ndc = Vector3(clipPos.x / clipPos.w, clipPos.y / clipPos.w, clipPos.z / clipPos.w);

    return Vector2(ndc.x * 0.5f + 0.5f, ndc.y * 0.5f + 0.5f);
}

void Renderer::renderLightMask(const SourceLight& light, unsigned int fbo) {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!light.enableGodRays || light.direction.y < 0.1f) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }

    Vector3 worldPos = camera->position + light.direction * 1000.0f;
    Vector2 screenPos = getScreenPosition(worldPos);

    if (screenPos.u < 0 || screenPos.u > 1 || screenPos.v < 0 || screenPos.v > 1) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }

    // —татические переменные внутри метода
    static Shader maskShader;
    static unsigned int maskVao = 0, maskVbo = 0;
    static bool maskInit = false;

    if (!maskInit) {
        const char* vs = R"(
            #version 330 core
            layout (location = 0) in vec2 aPos;
            uniform mat4 model;
            void main() {
                gl_Position = model * vec4(aPos, 0.0, 1.0);
            }
        )";
        const char* fs = R"(
            #version 330 core
            out vec4 FragColor;
            uniform vec3 lightColor;
            void main() {
                FragColor = vec4(lightColor, 1.0);
            }
        )";
        if (!maskShader.loadFromSource(vs, fs)) {
            std::cerr << "Failed to load mask shader!" << std::endl;
            return;
        }

        float quad[] = { -1,-1, 1,-1, -1,1, 1,1 };
        glGenVertexArrays(1, &maskVao);
        glGenBuffers(1, &maskVbo);

        if (maskVao == 0 || maskVbo == 0) {
            std::cerr << "Failed to generate mask VAO/VBO!" << std::endl;
            return;
        }

        glBindVertexArray(maskVao);
        glBindBuffer(GL_ARRAY_BUFFER, maskVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);
        maskInit = true;
    }

    float x = screenPos.u * 2.0f - 1.0f;
    float y = screenPos.v * 2.0f - 1.0f;
    float s = light.raySize;

    Matrix4 model = Matrix4::translation(Vector3(x, y, 0)) *
        Matrix4::scale(Vector3(s, s, 1));

    maskShader.use();
    maskShader.setUniform("model", model);
    maskShader.setUniform("lightColor", light.color.x, light.color.y, light.color.z);

    glBindVertexArray(maskVao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::setChunkManager(ChunkManager* chunkManage) {
    chunkManager = chunkManage;
    shadowMap->setChunkManager(chunkManage);
}

void Renderer::setCurrentDimension(Dimension* dimension) {
    currentDimension = dimension;
}