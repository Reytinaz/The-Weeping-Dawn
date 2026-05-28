#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "camera.hpp"
#include "object3d.hpp"
#include "SourceLight.hpp"
#include "shader.hpp"
#include "shadowMap.hpp"
#include "skybox.hpp"
#include "postProcessing.hpp"
#include "dimensionManager.hpp"

class Renderer {
private:
    bool depthShaderLoaded = false;
    bool debugEnabled = false;
    int width, height;
    unsigned int whiteTexture = 0;
    unsigned int depthFBO;
    unsigned int depthTexture;

    unsigned int sunMaskFbo;
    unsigned int sunMaskTexture;
    unsigned int moonMaskFbo;
    unsigned int moonMaskTexture;

    Shader maskShader;
    bool maskInit = false;
    unsigned int maskVao, maskVbo;

    ChunkManager* chunkManager;
    PostProcess* postProcess;
    Dimension* currentDimension;
    std::vector<GodRayParams> rayParams;
    void renderLightMask(const SourceLight& light, unsigned int fbo);
public:
    std::shared_ptr<Camera> camera;
    Shader* shader;
    ShadowMap* shadowMap;

    Renderer(int w, int h);
    ~Renderer();
    bool init();
    void setSize(int w, int h);
    void setDebug(bool b);
    void clear();
    void render(const std::vector<std::shared_ptr<Object3D>>& objects, const std::vector<std::shared_ptr<SourceLight>>& lights, float viewDistance,  float gamma);
    void debugRenderCascade(int cascadeIndex) const;
    void setChunkManager(ChunkManager* chunkManage);
    void setCurrentDimension(Dimension* dimension);
    Vector2 getScreenPosition(const Vector3& worldPos);
};

#endif