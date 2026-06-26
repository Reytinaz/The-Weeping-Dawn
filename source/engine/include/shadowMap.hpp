#ifndef SHADOW_MAP_HPP
#define SHADOW_MAP_HPP

#include "shader.hpp"
#include "matrix4.hpp"
#include "Object3D.hpp"
#include "chunkManager.hpp"
#include "camera.hpp"
#include "vector4.hpp"
#include "SourceLight.hpp"

#define MAX_LIGHTS 64
#define MAX_CASCADES 3

struct Cascade {
    unsigned int fbo;
    unsigned int depthTex;
    Matrix4 lightSpaceMatrix;
    float nearDist;
    float farDist;
};

class ShadowMap {
public:
    ShadowMap(int resolution);
    ~ShadowMap();

    bool init();
    void render(const std::vector<std::shared_ptr<Object3D>>& objects,
        const Camera& camera,
        std::shared_ptr<SourceLight> sunlight, float viewDistance);
    void bindCascadeTextures(unsigned int startUnit) const;
    void setChunkManager(ChunkManager* cm) { chunkManager = cm; }

    const std::vector<Cascade>& getCascades() const { return cascades; }
    const std::vector<float>& getCascadeDistances() const { return cascadeDistances; }
    const std::vector<Matrix4>& getLightMatrices() const {
        std::vector<Matrix4> matrices;
        matrices.resize(MAX_CASCADES);
        for (int i = 0; i < MAX_CASCADES; ++i)
            matrices[i] = cascades[i].lightSpaceMatrix;
        return matrices;
     }
private:
    int resolution;
    Shader depthShader;
    ChunkManager* chunkManager;

    std::vector<Cascade> cascades;
    std::vector<float> cascadeDistances;

    void computeCascadeDistances(float nearPlane, float farPlane);
    std::vector<Vector4> getFrustumCornersWorldSpace(const Matrix4& proj, const Matrix4& view) const;
    Matrix4 calculateLightMatrix(const Camera& camera, float nearDist, float farDist, const Vector3& lightDir, int cascadeIndex) const;
};

#endif