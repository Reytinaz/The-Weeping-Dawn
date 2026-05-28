#include "postProcessing.hpp"
#include "glad.h"

PostProcess::PostProcess(int w, int h) : width(w), height(h) {
    glGenFramebuffers(1, &fbo);
    glGenFramebuffers(1, &tempFbo);
}

PostProcess::~PostProcess() {
    glDeleteFramebuffers(1, &fbo);
    glDeleteFramebuffers(1, &tempFbo);
    glDeleteTextures(1, &colorTexture);
    glDeleteTextures(1, &tempTexture);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
}

bool PostProcess::init() {
    // Основная цветовая текстура
    glGenTextures(1, &colorTexture);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Временная текстура для лучей
    glGenTextures(1, &tempTexture);
    glBindTexture(GL_TEXTURE_2D, tempTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Настройка основного FBO
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "PostProcess FBO error!" << std::endl;
        return false;
    }

    // Настройка временного FBO
    glBindFramebuffer(GL_FRAMEBUFFER, tempFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tempTexture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Temp FBO error!" << std::endl;
        return false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Экранный квад
    float quadVertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f,  1.0f, 1.0f, 1.0f
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Шейдер лучей
    const char* godRaysVS = R"(
        #version 330 core
        layout(location = 0) in vec2 aPos;
        layout(location = 1) in vec2 aTexCoords;
        out vec2 TexCoords;
        void main() {
            TexCoords = aTexCoords;
            gl_Position = vec4(aPos, 0.0, 1.0);
        }
    )";

    const char* godRaysFS = R"(
        #version 330 core
        out vec4 FragColor;
        in vec2 TexCoords;
        uniform sampler2D sourceTexture;
        uniform vec2 lightPos;
        uniform vec3 lightColor;
        uniform float exposure;
        uniform float decay;
        uniform float density;
        uniform float weight;
        const int NUM_SAMPLES = 100;

        void main() {
            vec2 deltaTexCoord = TexCoords - lightPos;
            vec2 texCoord = TexCoords;
            deltaTexCoord *= 1.0 / float(NUM_SAMPLES) * density;
            
            vec3 color = texture(sourceTexture, TexCoords).rgb;
            float illuminationDecay = 1.0;
            
            for (int i = 0; i < NUM_SAMPLES; i++) {
                texCoord -= deltaTexCoord;
                vec3 sample = texture(sourceTexture, texCoord).rgb;
                sample *= illuminationDecay * weight;
                color += sample;
                illuminationDecay *= decay;
            }
            
            FragColor = vec4(color * exposure * lightColor, 1.0);
        }
    )";

    // Шейдер смешивания
    const char* blendFS = R"(
        #version 330 core
        out vec4 FragColor;
        in vec2 TexCoords;
        uniform sampler2D sceneTexture;
        uniform sampler2D raysTexture;
        void main() {
            vec3 scene = texture(sceneTexture, TexCoords).rgb;
            vec3 rays = texture(raysTexture, TexCoords).rgb;
            FragColor = vec4(scene + rays, 1.0);
        }
    )";

    if (!godRaysShader.loadFromSource(godRaysVS, godRaysFS)) {
        std::cerr << "Failed to compile god rays shader!" << std::endl;
        return false;
    }

    if (!blendShader.loadFromSource(godRaysVS, blendFS)) {
        std::cerr << "Failed to compile blend shader!" << std::endl;
        return false;
    }

    std::cout << "Post Processing initiliazed" << std::endl;
    return true;
}

void PostProcess::beginCapture() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, width, height);
}

void PostProcess::endCapture() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcess::renderGodRays(unsigned int sourceTexture, const GodRayParams& params) {
    glBindFramebuffer(GL_FRAMEBUFFER, tempFbo);
    glClear(GL_COLOR_BUFFER_BIT);

    godRaysShader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sourceTexture);
    godRaysShader.setUniform("sourceTexture", 0);
    godRaysShader.setUniform("lightPos", params.screenPos.u, params.screenPos.v);
    godRaysShader.setUniform("lightColor", params.color.x, params.color.y, params.color.z);
    godRaysShader.setUniform("exposure", params.exposure);
    godRaysShader.setUniform("decay", params.decay);
    godRaysShader.setUniform("density", params.density);
    godRaysShader.setUniform("weight", params.weight);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void PostProcess::renderGodRaysMulti(const std::vector<GodRayParams>& params) {
    if (params.empty()) {
        glBindFramebuffer(GL_FRAMEBUFFER, tempFbo);
        glClear(GL_COLOR_BUFFER_BIT);
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, tempFbo);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    godRaysShader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    godRaysShader.setUniform("sourceTexture", 0);

    // Проверка uniform'ов до цикла
    GLint lightPosLoc = glGetUniformLocation(godRaysShader.getProgramID(), "lightPos");
    GLint lightColorLoc = glGetUniformLocation(godRaysShader.getProgramID(), "lightColor");
    if (lightPosLoc == -1) std::cerr << "lightPos uniform not found!" << std::endl;
    if (lightColorLoc == -1) std::cerr << "lightColor uniform not found!" << std::endl;

    for (const auto& p : params) {
        godRaysShader.setUniform("lightPos", p.screenPos.u, p.screenPos.v);
        godRaysShader.setUniform("lightColor", p.color.x, p.color.y, p.color.z);
        godRaysShader.setUniform("exposure", p.exposure);
        godRaysShader.setUniform("decay", p.decay);
        godRaysShader.setUniform("density", p.density);
        godRaysShader.setUniform("weight", p.weight);

        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    glDisable(GL_BLEND);
}

void PostProcess::createQuad() {
    if (quadVAO != 0) {
        glDeleteVertexArrays(1, &quadVAO);
        glDeleteBuffers(1, &quadVBO);
    }

    float quadVertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f,  1.0f, 1.0f, 1.0f
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void PostProcess::renderToScreen() {
    if (blendShader.getProgramID() == 0) {
        std::cerr << "Blend shader not initialized!" << std::endl;
        return;
    }
    if (quadVAO == 0) {
        createQuad();
    }
    glBindVertexArray(quadVAO); // РАСКОММЕНТИРУЙ!
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}