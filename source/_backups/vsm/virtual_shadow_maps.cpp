#include "virtual_shadow_map.hpp"
#include <glad.h>
#include <iostream>
#include <vector>

VirtualShadowMap::VirtualShadowMap(int physicalWidth, int physicalHeight)
    : physicalWidth(physicalWidth), physicalHeight(physicalHeight) {
}

VirtualShadowMap::~VirtualShadowMap() {
    glDeleteTextures(1, &physicalAtlas);
    glDeleteTextures(1, &pageTableTex);
    glDeleteFramebuffers(1, &fbo);
    glDeleteBuffers(1, &freePageCounterSSBO);
    glDeleteBuffers(1, &freePageListSSBO);
    glDeleteBuffers(1, &requestBufferSSBO);
    glDeleteBuffers(1, &requestCounterSSBO);
}

bool VirtualShadowMap::init() {
    // Физический атлас
    glGenTextures(1, &physicalAtlas);
    glBindTexture(GL_TEXTURE_2D, physicalAtlas);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, physicalWidth, physicalHeight, 0,
        GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Таблица страниц
    glGenTextures(1, &pageTableTex);
    glBindTexture(GL_TEXTURE_2D, pageTableTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32UI, vsm::VIRTUAL_PAGES, vsm::VIRTUAL_PAGES, 0,
        GL_RGBA_INTEGER, GL_UNSIGNED_INT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    std::vector<uint32_t> clearData(vsm::VIRTUAL_PAGES * vsm::VIRTUAL_PAGES * 4, 0);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, vsm::VIRTUAL_PAGES, vsm::VIRTUAL_PAGES,
        GL_RGBA_INTEGER, GL_UNSIGNED_INT, clearData.data());

    // FBO
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, physicalAtlas, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        return false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Счетчик свободных страниц
    glGenBuffers(1, &freePageCounterSSBO);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, freePageCounterSSBO);
    GLuint initialCounter = vsm::PHYSICAL_PAGES;
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &initialCounter, GL_DYNAMIC_COPY);

    // Список свободных страниц
    glGenBuffers(1, &freePageListSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, freePageListSSBO);
    std::vector<GLuint> freeIndices(vsm::PHYSICAL_PAGES);
    for (int i = 0; i < vsm::PHYSICAL_PAGES; i++) freeIndices[i] = i;
    glBufferData(GL_SHADER_STORAGE_BUFFER, vsm::PHYSICAL_PAGES * sizeof(GLuint), freeIndices.data(), GL_DYNAMIC_COPY);

    // Буфер запросов
    glGenBuffers(1, &requestBufferSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, requestBufferSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, vsm::MAX_MARKED_PAGES * sizeof(uint32_t), nullptr, GL_DYNAMIC_COPY);

    // Счетчик запросов
    glGenBuffers(1, &requestCounterSSBO);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, requestCounterSSBO);
    GLuint zero = 0;
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &zero, GL_DYNAMIC_COPY);

    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Mark shader
    const char* markSrc = R"(
        #version 430 core
        #define VIRTUAL_PAGES 128
        #define VIRTUAL_SIZE 16384
        #define PAGE_SIZE 128
        #define MAX_MARKED_PAGES 2048
        
        layout(local_size_x = 16, local_size_y = 16) in;
        
        uniform sampler2D depthTex;
        uniform mat4 invProj;
        uniform mat4 invView;
        uniform mat4 lightSpaceMatrix;
        uniform ivec2 screenSize;
        
        layout(std430, binding = 0) buffer RequestBuffer { uint pages[]; };
        layout(binding = 0, offset = 0) uniform atomic_uint requestCounter;
        
        void main() {
            ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
            if (pixel.x >= screenSize.x || pixel.y >= screenSize.y) return;
            
            float depth = texelFetch(depthTex, pixel, 0).r;
            if (depth >= 1.0) return;
            
            vec4 clipPos = vec4(
                (float(pixel.x) / screenSize.x) * 2.0 - 1.0,
                (float(pixel.y) / screenSize.y) * 2.0 - 1.0,
                depth * 2.0 - 1.0,
                1.0
            );
            
            vec4 viewPos = invProj * clipPos;
            viewPos /= viewPos.w;
            vec4 worldPos = invView * viewPos;
            
            vec4 lightPos = lightSpaceMatrix * worldPos;
            vec3 projCoords = lightPos.xyz / lightPos.w;
            projCoords = projCoords * 0.5 + 0.5;
            
            if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
                projCoords.y < 0.0 || projCoords.y > 1.0 ||
                projCoords.z < 0.0 || projCoords.z > 1.0) return;
            
            uint pageX = uint(projCoords.x * VIRTUAL_SIZE / PAGE_SIZE);
            uint pageY = uint(projCoords.y * VIRTUAL_SIZE / PAGE_SIZE);
            uint virtualIndex = pageY * VIRTUAL_PAGES + pageX;
            
            uint pos = atomicCounterIncrement(requestCounter);
            if (pos < MAX_MARKED_PAGES) {
                pages[pos] = virtualIndex;
            }
        }
    )";

    if (!markShader.loadComputeFromSource(markSrc)) {
        std::cerr << "Failed to load mark shader\n";
        return false;
    }

    // Alloc shader
    const char* allocSrc = R"(
        #version 430 core
        #define VIRTUAL_PAGES 128
        #define PHYSICAL_PAGES 1024
        
        layout(local_size_x = 64) in;
        
        layout(std430, binding = 0) buffer RequestBuffer { uint pages[]; };
        layout(std430, binding = 1) buffer FreeList { uint freePages[]; };
        
        layout(binding = 0, offset = 0) uniform atomic_uint freeCounter;
        layout(binding = 1, offset = 0) uniform atomic_uint requestCounter;
        
        layout(rgba32ui, binding = 0) uniform uimage2D pageTable;
        
        void main() {
            uint count = atomicCounter(requestCounter);
            uint idx = gl_GlobalInvocationID.x;
            if (idx >= count) return;
            
            uint vpage = pages[idx];
            ivec2 pageCoord = ivec2(vpage % VIRTUAL_PAGES, vpage / VIRTUAL_PAGES);
            
            uvec4 entry = imageLoad(pageTable, pageCoord);
            if ((entry.r & 2u) != 0) return;
            
            uint freePos = atomicCounterDecrement(freeCounter);
            if (freePos == 0 || freePos > PHYSICAL_PAGES) return;
            
            uint newPhys = freePages[freePos - 1];
            imageStore(pageTable, pageCoord, uvec4(entry.r | 2u, newPhys, 0, 0));
        }
    )";

    if (!allocShader.loadComputeFromSource(allocSrc)) {
        std::cerr << "Failed to load alloc shader\n";
        return false;
    }

    // Depth shader
    const char* depthVS = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        uniform mat4 lightSpaceMatrix;
        uniform mat4 model;
        void main() {
            gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
        }
    )";
    const char* depthFS = "#version 330 core\nvoid main(){}\n";

    if (!depthShader.loadFromSource(depthVS, depthFS)) {
        std::cerr << "Failed to load depth shader\n";
        return false;
    }

    std::cout << "VSM initialized\n";
    return true;
}

void VirtualShadowMap::render(const std::vector<std::shared_ptr<Object3D>>& objects,
    const Camera& camera,
    unsigned int depthTexture,
    const Matrix4& lightSpaceMatrix,
    const Vector3& lightDir,
    int screenWidth, int screenHeight) {
    this->lightSpaceMatrix = lightSpaceMatrix;

    // Сброс счетчиков
    GLuint zero = 0;
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, requestCounterSSBO);
    glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &zero);

    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, freePageCounterSSBO);
    GLuint initialCounter = vsm::PHYSICAL_PAGES;
    glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &initialCounter);

    // Восстановление списка свободных страниц
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, freePageListSSBO);
    std::vector<GLuint> freeIndices(vsm::PHYSICAL_PAGES);
    for (int i = 0; i < vsm::PHYSICAL_PAGES; i++) freeIndices[i] = i;
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, vsm::PHYSICAL_PAGES * sizeof(GLuint), freeIndices.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

    // Mark visible pages
    {
        Matrix4 invProj = camera.getProjectionMatrix().inverse();
        Matrix4 invView = camera.getViewMatrix().inverse();

        markShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depthTexture);
        markShader.setUniform("depthTex", 0);
        markShader.setUniform("invProj", invProj);
        markShader.setUniform("invView", invView);
        markShader.setUniform("lightSpaceMatrix", lightSpaceMatrix);
        markShader.setUniform("screenSize", screenWidth, screenHeight);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, requestBufferSSBO);
        glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, requestCounterSSBO);

        int groupsX = (screenWidth + 15) / 16;
        int groupsY = (screenHeight + 15) / 16;
        glDispatchCompute(groupsX, groupsY, 1);

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);
    }

    // Allocate pages
    {
        GLuint requestCount = 0;
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, requestCounterSSBO);
        glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &requestCount);
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

        if (requestCount > 0) {
            allocShader.use();
            glBindImageTexture(0, pageTableTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32UI);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, requestBufferSSBO);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, freePageListSSBO);
            glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, freePageCounterSSBO);
            glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, requestCounterSSBO);

            int groups = (requestCount + 63) / 64;
            glDispatchCompute(groups, 1, 1);

            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
        }
    }

    // Render pages - упрощенно, рендерим все объекты во весь атлас
    {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, physicalWidth, physicalHeight);
        glClear(GL_DEPTH_BUFFER_BIT);

        depthShader.use();
        depthShader.setUniform("lightSpaceMatrix", lightSpaceMatrix);

        for (auto& obj : objects) {
            if (!obj->visible) continue;
            depthShader.setUniform("model", obj->getTransform());
            obj->render();
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_CULL_FACE);
    }
}

void VirtualShadowMap::bindPhysicalAtlas(unsigned int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, physicalAtlas);
}

void VirtualShadowMap::bindPageTable(unsigned int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, pageTableTex);
}

void VirtualShadowMap::debugRender() {
    // Проверка атласа
    std::vector<float> atlasData(physicalWidth * physicalHeight);
    glBindTexture(GL_TEXTURE_2D, physicalAtlas);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, atlasData.data());

    int filled = 0;
    for (float v : atlasData) {
        if (v < 0.99f) filled++;
    }

    // Проверка таблицы страниц
    std::vector<GLuint> pageData(vsm::VIRTUAL_PAGES * vsm::VIRTUAL_PAGES * 4);
    glBindTexture(GL_TEXTURE_2D, pageTableTex);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA_INTEGER, GL_UNSIGNED_INT, pageData.data());

    int allocated = 0;
    for (int i = 0; i < vsm::VIRTUAL_PAGES * vsm::VIRTUAL_PAGES; i++) {
        if (pageData[i * 4] & 2) allocated++;
    }

    std::cout << "VSM Debug: Atlas filled pixels: " << filled
        << ", Allocated pages: " << allocated << "\n";
}