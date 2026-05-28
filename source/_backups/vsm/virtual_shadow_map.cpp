/*
Notes:
m_pageTableTex

*/

#include "virtual_shadow_map.hpp"
#include "glad.h"

VirtualShadowMap::VirtualShadowMap(int physicalWidth, int physicalHeight)
    : physicalWidth(physicalWidth), physicalHeight(physicalHeight)
{
}

VirtualShadowMap::~VirtualShadowMap() {
    if (physicalAtlas) glDeleteTextures(1, &physicalAtlas);
    if (pageTableTex) glDeleteTextures(1, &pageTableTex);
    if (fbo) glDeleteFramebuffers(1, &fbo);
    if (freePageCounterSSBO) glDeleteBuffers(1, &freePageCounterSSBO);
    if (freePageListSSBO) glDeleteBuffers(1, &freePageListSSBO);
}

bool VirtualShadowMap::init() {
    // 1. Физический атлас
    glGenTextures(1, &physicalAtlas);
    glBindTexture(GL_TEXTURE_2D, physicalAtlas);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F,
        physicalWidth, physicalHeight, 0,
        GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 2. Таблица страниц
    glGenTextures(1, &pageTableTex);
    glBindTexture(GL_TEXTURE_2D, pageTableTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI,
        vsm::VIRTUAL_PAGES, vsm::VIRTUAL_PAGES, 0,
        GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 3. FBO для рендера в атлас
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, physicalAtlas, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "VSM FBO not complete!" << std::endl;
        return false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 4. Создание SSBO для аллокатора
    // Счётчик свободных страниц (одно целое число)
    glGenBuffers(1, &freePageCounterSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, freePageCounterSSBO);
    uint32_t initialCounter = vsm::PHYSICAL_PAGES; // начинаем со всеми свободными
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t), &initialCounter, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Список свободных страниц (массив индексов)
    glGenBuffers(1, &freePageListSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, freePageListSSBO);
    std::vector<uint32_t> freeIndices(vsm::PHYSICAL_PAGES);
    for (uint32_t i = 0; i < vsm::PHYSICAL_PAGES; ++i) freeIndices[i] = i;
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t) * vsm::PHYSICAL_PAGES,
        freeIndices.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // 5. Compute-шейдер для разметки страниц (без изменений)
    const char* markPagesCompSrc = R"(
        #version 430 core
        layout (local_size_x = 16, local_size_y = 16) in;

        layout (binding = 0) uniform sampler2D depthTexture;
        layout (binding = 1, r32ui) uniform volatile uimage2D pageTable;

        uniform mat4 lightSpaceMatrix;
        uniform mat4 view;
        uniform mat4 projection;
        uniform vec3 cameraPos;
        uniform vec3 lightDir;
        uniform ivec2 screenSize;

        const uint VIRTUAL_PAGES = 128;
        const uint PAGE_SIZE = 128;
        const uint VIRTUAL_SIZE = VIRTUAL_PAGES * PAGE_SIZE;

        const uint PAGE_VISIBLE   = 1u << 0;
        const uint PAGE_ALLOCATED = 1u << 1;
        const uint PAGE_DIRTY     = 1u << 2;
        const uint PAGE_REQUESTED = 1u << 3;

        void main() {
            ivec2 pixelPos = ivec2(gl_GlobalInvocationID.xy);
            if (pixelPos.x >= screenSize.x || pixelPos.y >= screenSize.y) return;

            float depth = texelFetch(depthTexture, pixelPos, 0).r;

            uint virtualX = uint(pixelPos.x) * VIRTUAL_PAGES / screenSize.x;
            uint virtualY = uint(pixelPos.y) * VIRTUAL_PAGES / screenSize.y;

            imageAtomicOr(pageTable, ivec2(virtualX, virtualY), PAGE_VISIBLE | PAGE_REQUESTED);
        }
    )";

    if (!markShader.loadComputeFromSource(markPagesCompSrc)) {
        std::cerr << "Failed to load compute shader for page marking" << std::endl;
        return false;
    }

    // 6. Compute-шейдер для выделения страниц (GPU-аллокатор)
    const char* allocPagesCompSrc = R"(
        #version 430 core
        layout (local_size_x = 8, local_size_y = 8) in; // можно подобрать размер

        layout (binding = 1, r32ui) uniform volatile uimage2D pageTable;
        layout (binding = 2, offset = 0) uniform atomic_uint freePageCounter;
        layout (binding = 3) buffer FreePageList { uint indices[]; } freeList;

        const uint PAGE_REQUESTED = 1u << 3;
        const uint PAGE_ALLOCATED = 1u << 1;

        void main() {
            ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
            if (pos.x >= 128 || pos.y >= 128) return;

            uint data = imageLoad(pageTable, pos).r;
            if ((data & PAGE_REQUESTED) != 0u && (data & PAGE_ALLOCATED) == 0u) {
                uint freeIdx = atomicCounterDecrement(freePageCounter);
                if (freeIdx > 0u) { // есть свободные страницы
                    uint physIndex = freeList.indices[freeIdx - 1]; // индексы от 0
                    data |= PAGE_ALLOCATED;
                    data |= (physIndex << 8);
                    imageStore(pageTable, pos, uvec4(data));
                }
            }
        }
    )";

    if (!allocShader.loadComputeFromSource(allocPagesCompSrc)) {
        std::cerr << "Failed to load compute shader for page allocation" << std::endl;
        return false;
    }

    // 7. Шейдер для рендера глубины объектов в атлас
    const char* depthVertexSrc = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        uniform mat4 lightSpaceMatrix;
        uniform mat4 model;
        void main() {
            gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
        }
    )";
    const char* depthFragSrc = R"(
        #version 330 core
        void main() { }
    )";
    if (!depthShader.loadFromSource(depthVertexSrc, depthFragSrc)) {
        std::cerr << "Failed to load depth shader for VSM" << std::endl;
        return false;
    }

    std::cout << "VSM initialized with GPU allocator" << std::endl;
    return true;
}

void VirtualShadowMap::render(const std::vector<std::shared_ptr<Object3D>>& objects,
    const Vector3& camPos,
    const Camera& camera,
    unsigned int depthTexture,
    const Matrix4& lightSpaceMatrix,
    const Vector3& lightDir,
    int screenWidth, int screenHeight) {

    this->lightSpaceMatrix = lightSpaceMatrix;

    // Очистка таблицы страниц
    std::vector<uint32_t> clearData(vsm::VIRTUAL_PAGES * vsm::VIRTUAL_PAGES, 0);
    glBindTexture(GL_TEXTURE_2D, pageTableTex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, vsm::VIRTUAL_PAGES, vsm::VIRTUAL_PAGES,
        GL_RED_INTEGER, GL_UNSIGNED_INT, clearData.data());
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // Сброс счётчика свободных страниц и списка (заполняем заново)
    // Можно обновлять данные в SSBO через glBufferSubData
    uint32_t initialCounter = vsm::PHYSICAL_PAGES;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, freePageCounterSSBO);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t), &initialCounter);

    std::vector<uint32_t> freeIndices(vsm::PHYSICAL_PAGES);
    for (uint32_t i = 0; i < vsm::PHYSICAL_PAGES; ++i) freeIndices[i] = i;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, freePageListSSBO);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t) * vsm::PHYSICAL_PAGES, freeIndices.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Разметка видимых страниц
    markVisiblePages(camera, depthTexture, lightSpaceMatrix, lightDir, screenWidth, screenHeight);

    // Выделение страниц на GPU
    allocatePagesGPU();

    // Рендер страниц (пока можно закомментировать для теста производительности)
    renderPages(objects);
}

void VirtualShadowMap::markVisiblePages(const Camera& camera,
    unsigned int depthTexture,
    const Matrix4& lightSpaceMatrix,
    const Vector3& lightDir,
    int screenWidth, int screenHeight) {
    markShader.use();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glUniform1i(glGetUniformLocation(markShader.getProgramID(), "depthTexture"), 0);

    glBindImageTexture(1, pageTableTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

    markShader.setUniform("lightSpaceMatrix", lightSpaceMatrix);
    markShader.setUniform("view", camera.getViewMatrix());
    markShader.setUniform("projection", camera.getProjectionMatrix());
    markShader.setUniform("cameraPos", camera.position);
    markShader.setUniform("lightDir", lightDir);
    glUniform2i(glGetUniformLocation(markShader.getProgramID(), "screenSize"), screenWidth, screenHeight);

    int numGroupsX = (screenWidth + 15) / 16;
    int numGroupsY = (screenHeight + 15) / 16;
    glDispatchCompute(numGroupsX, numGroupsY, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void VirtualShadowMap::allocatePagesGPU() {
    allocShader.use();

    glBindImageTexture(1, pageTableTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, freePageCounterSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, freePageListSSBO);

    // Размер рабочей группы (8x8) – покрываем всю таблицу 128x128
    int groupsX = (vsm::VIRTUAL_PAGES + 7) / 8;
    int groupsY = (vsm::VIRTUAL_PAGES + 7) / 8;
    glDispatchCompute(groupsX, groupsY, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);
}
void VirtualShadowMap::renderPages(const std::vector<std::shared_ptr<Object3D>>& objects) {
    if (!fbo) return;

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);

    depthShader.use();
    depthShader.setUniform("lightSpaceMatrix", lightSpaceMatrix);

    // Чтение таблицы (пока синхронно, позже можно оптимизировать)
    std::vector<uint32_t> pageData(vsm::VIRTUAL_PAGES * vsm::VIRTUAL_PAGES);
    glBindTexture(GL_TEXTURE_2D, pageTableTex);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, pageData.data());

    for (uint32_t i = 0; i < pageData.size(); ++i) {
        uint32_t entry = pageData[i];
        if (!(entry & vsm::PAGE_ALLOCATED)) continue;

        uint32_t virtualX = i % vsm::VIRTUAL_PAGES;
        uint32_t virtualY = i / vsm::VIRTUAL_PAGES;
        uint32_t physIdx = (entry >> 8);
        uint32_t physX = (physIdx % vsm::PHYSICAL_PAGES_PER_ROW) * vsm::PAGE_SIZE;
        uint32_t physY = (physIdx / vsm::PHYSICAL_PAGES_PER_ROW) * vsm::PAGE_SIZE;

        glViewport(physX, physY, vsm::PAGE_SIZE, vsm::PAGE_SIZE);
        glEnable(GL_SCISSOR_TEST);
        glScissor(physX, physY, vsm::PAGE_SIZE, vsm::PAGE_SIZE);

        for (auto& obj : objects) {
            if (!obj->visible) continue;

            Vector3 objMin, objMax;
            obj->getWorldBounds(objMin, objMax);

            // Проверяем, пересекается ли объект с текущей страницей
            for (int c = 0; c < 8; ++c) {
                Vector3 corner;
                corner.x = (c & 1) ? objMax.x : objMin.x;
                corner.y = (c & 2) ? objMax.y : objMin.y;
                corner.z = (c & 4) ? objMax.z : objMin.z;

                Vector3 lightPos = lightSpaceMatrix.transformPoint(corner);
                float x = lightPos.x * 0.5f + 0.5f;
                float y = lightPos.y * 0.5f + 0.5f;
                if (x < 0.0f || x > 1.0f || y < 0.0f || y > 1.0f) continue;

                uint32_t pageX = uint32_t(x * vsm::VIRTUAL_PAGES);
                uint32_t pageY = uint32_t(y * vsm::VIRTUAL_PAGES);
                if (pageX == virtualX && pageY == virtualY) {
                    depthShader.setUniform("model", obj->getTransform());
                    obj->render();
                    break;
                }
            }
        }

        glDisable(GL_SCISSOR_TEST);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, physicalWidth, physicalHeight);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}
void VirtualShadowMap::bindPhysicalAtlas(unsigned int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, physicalAtlas);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void VirtualShadowMap::bindPageTable(unsigned int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, pageTableTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void VirtualShadowMap::debugRenderPageTable() {
    static Shader debugShader;
    static bool initialized = false;
    static unsigned int quadVAO = 0, quadVBO = 0;

    if (!initialized) {
        const char* vertSrc = R"(
            #version 330 core
            layout (location = 0) in vec2 aPos;
            out vec2 TexCoords;
            void main() {
                TexCoords = aPos * 0.5 + 0.5;
                gl_Position = vec4(aPos, 0.0, 1.0);
            }
        )";
        const char* fragSrc = R"(
            #version 330 core
            uniform usampler2D pageTable;
            in vec2 TexCoords;
            out vec4 FragColor;
            void main() {
                uint data = texture(pageTable, TexCoords).r;
                uint flags = data & 0xFFu;
                uint physIndex = data >> 8u;
                if ((flags & 0x02u) != 0u) { // PAGE_ALLOCATED
                    float intensity = float(physIndex) / 1023.0; // нормализуем индекс
                    FragColor = vec4(0.0, 0.0, intensity, 1.0);
                } else {
                    FragColor = vec4(0.0, 0.0, 0.0, 1.0);
                }
            }
        )";
        debugShader.loadFromSource(vertSrc, fragSrc);
        initialized = true;

        float quadVertices[] = { -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f };
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    glViewport(0, 0, 200, 200);
    glDisable(GL_DEPTH_TEST);

    debugShader.use();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pageTableTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    debugShader.setUniform("pageTable", 0);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, physicalWidth, physicalHeight);
}