#pragma once
#include "vsm_constants.hpp"
#include "shader.hpp"
#include "camera.hpp"
#include "Object3D.hpp"
#include <vector>
#include <memory>

class VirtualShadowMap {
public:
    VirtualShadowMap(int physicalWidth, int physicalHeight);
    ~VirtualShadowMap();

    bool init();
    void render(const std::vector<std::shared_ptr<Object3D>>& objects,
        const Camera& camera,
        unsigned int depthTexture,
        const Matrix4& lightSpaceMatrix,
        const Vector3& lightDir,
        int screenWidth, int screenHeight);

    void bindPhysicalAtlas(unsigned int unit) const;
    void bindPageTable(unsigned int unit) const;
    void debugRender();

private:
    int physicalWidth, physicalHeight;
    unsigned int physicalAtlas = 0;
    unsigned int pageTableTex = 0;
    unsigned int fbo = 0;

    unsigned int freePageCounterSSBO = 0;
    unsigned int freePageListSSBO = 0;
    unsigned int requestBufferSSBO = 0;
    unsigned int requestCounterSSBO = 0;

    Shader markShader;
    Shader allocShader;
    Shader depthShader;

    Matrix4 lightSpaceMatrix;
};