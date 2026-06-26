#include "shadowMap.hpp"
#include "glad.h"

ShadowMap::ShadowMap(int resolution)
    : resolution(resolution), chunkManager(nullptr) {
    cascades.resize(MAX_CASCADES);
    cascadeDistances.resize(MAX_CASCADES + 1);
}

ShadowMap::~ShadowMap() {
    for (auto& cascade : cascades) {
        if (cascade.fbo) glDeleteFramebuffers(1, &cascade.fbo);
        if (cascade.depthTex) glDeleteTextures(1, &cascade.depthTex);
    }
}
bool ShadowMap::init() {
    for (int i = 0; i < MAX_CASCADES; ++i) {

        glGenTextures(1, &cascades[i].depthTex);
        glBindTexture(GL_TEXTURE_2D, cascades[i].depthTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
            resolution, resolution, 0,
            GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        glGenFramebuffers(1, &cascades[i].fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, cascades[i].fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
            GL_TEXTURE_2D, cascades[i].depthTex, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Cascade " << i << " FBO error\n";
            return false;
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    const char* vs = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        uniform mat4 lightSpaceMatrix;
        uniform mat4 model;
        void main() {
            gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
        }
    )";
    const char* fs = "#version 330 core\nvoid main(){}\n";
    if (!depthShader.loadFromSource(vs, fs)) return false;

    return true;
}

void ShadowMap::computeCascadeDistances(float nearPlane, float farPlane) {
    float lambda = 0.5f;
    if (farPlane <= nearPlane) return;
    cascadeDistances[0] = nearPlane;
    float range = farPlane - nearPlane;
    float ratio = farPlane / nearPlane;
    for (int i = 1; i <= MAX_CASCADES; ++i) {
        float f = (float)i / (float)MAX_CASCADES;
        float logSplit = nearPlane * pow(ratio, f);
        float linearSplit = nearPlane + range * f;
        float split = lambda * logSplit + (1.0f - lambda) * linearSplit;
        cascadeDistances[i] = split;
    }
    for (int i = 1; i < MAX_CASCADES; ++i) {
        float overlapDist = (cascadeDistances[i + 1] - cascadeDistances[i - 1]) * 1;
        cascadeDistances[i] = std::min(cascadeDistances[i] + overlapDist, cascadeDistances[i + 1]);
    }
}

std::vector<Vector4> ShadowMap::getFrustumCornersWorldSpace(const Matrix4& proj, const Matrix4& view) const {
    Matrix4 inv = (proj * view).inverse();
    std::vector<Vector4> corners;
    for (int x = 0; x < 2; ++x) {
        for (int y = 0; y < 2; ++y) {
            for (int z = 0; z < 2; ++z) {
                Vector4 pt = inv * Vector4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
                corners.push_back(pt / pt.w);
            }
        }
    }
    return corners;
}

Matrix4 ShadowMap::calculateLightMatrix(const Camera& camera,
    float nearDist,
    float farDist,
    const Vector3& lightDir, int cascadeIndex) const {

    Matrix4 proj = Matrix4::perspective(camera.fov, camera.aspectRatio, nearDist, farDist);
    Matrix4 view = camera.getViewMatrix();
    std::vector<Vector4> corners = getFrustumCornersWorldSpace(proj, view);

    Vector3 center(0, 0, 0);
    for (const auto& v : corners) {
        center.x += v.x; center.y += v.y; center.z += v.z;
    }
    center = center * (1.0f / 8.0f);

    Vector3 toLight = -lightDir.normalized();
    float distance = (camera.farPlane - camera.nearPlane) * 10.0f;
    Vector3 lightEye = center + toLight * distance;

    Vector3 worldUp = Vector3(0, 1, 0);
    Vector3 right = toLight.cross(worldUp).normalized();
    if (right.length() < 0.1f) right = Vector3(1, 0, 0);
    Vector3 up = right.cross(toLight).normalized();

    Matrix4 lightView = Matrix4::lookAt(lightEye, center, up);

    float minX = INFINITY, maxX = -INFINITY, minY = INFINITY, maxY = -INFINITY, minZ = INFINITY, maxZ = -INFINITY;
    for (const auto& v : corners) {
        Vector4 trf = lightView * v;
        minX = std::min(minX, trf.x); maxX = std::max(maxX, trf.x);
        minY = std::min(minY, trf.y); maxY = std::max(maxY, trf.y);
        minZ = std::min(minZ, trf.z); maxZ = std::max(maxZ, trf.z);
    }

    if (cascadeIndex == MAX_CASCADES - 1 && chunkManager) {
        Vector3 worldMin, worldMax;
        chunkManager->getWorldBounds(worldMin, worldMax);
        std::vector<Vector3> worldCorners = {
            Vector3(worldMin.x, worldMin.y, worldMin.z),
            Vector3(worldMax.x, worldMin.y, worldMin.z),
            Vector3(worldMin.x, worldMax.y, worldMin.z),
            Vector3(worldMax.x, worldMax.y, worldMin.z),
            Vector3(worldMin.x, worldMin.y, worldMax.z),
            Vector3(worldMax.x, worldMin.y, worldMax.z),
            Vector3(worldMin.x, worldMax.y, worldMax.z),
            Vector3(worldMax.x, worldMax.y, worldMax.z)
        };
        for (const auto& wc : worldCorners) {
            Vector4 trf = lightView * Vector4(wc.x, wc.y, wc.z, 1.0f);
            minX = std::min(minX, trf.x); maxX = std::max(maxX, trf.x);
            minY = std::min(minY, trf.y); maxY = std::max(maxY, trf.y);
            minZ = std::min(minZ, trf.z); maxZ = std::max(maxZ, trf.z);
        }
    }

    float xRange = maxX - minX;
    float yRange = maxY - minY;
    float zRange = maxZ - minZ;
    minX -= xRange * 0.5f;
    maxX += xRange * 0.5f;
    minY -= yRange * 0.5f;
    maxY += yRange * 0.5f;
    minZ -= zRange * 0.5f;
    maxZ += zRange * 0.5f;

    float worldUnitsPerTexelX = (maxX - minX) / (float)resolution;
    float worldUnitsPerTexelY = (maxY - minY) / (float)resolution;
    if (worldUnitsPerTexelX > 0.0f && worldUnitsPerTexelY > 0.0f) {
        float centerX = (minX + maxX) * 0.5f;
        float centerY = (minY + maxY) * 0.5f;
        float snapX = floor(centerX / worldUnitsPerTexelX) * worldUnitsPerTexelX;
        float snapY = floor(centerY / worldUnitsPerTexelY) * worldUnitsPerTexelY;
        float dx = snapX - centerX;
        float dy = snapY - centerY;
        minX += dx; maxX += dx;
        minY += dy; maxY += dy;
    }
    float near = -maxZ;
    float far = -minZ;

    Matrix4 lightProj = Matrix4::orthographic(minX, maxX, minY, maxY, near, far);
    return lightProj * lightView;
}

void ShadowMap::render(const std::vector<std::shared_ptr<Object3D>>& objects,
    const Camera& camera,
    std::shared_ptr<SourceLight> sunlight, float viewDistance) {

    computeCascadeDistances(camera.nearPlane, viewDistance);

    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);

    for (int i = 0; i < MAX_CASCADES; ++i) {
        float nearSplit = cascadeDistances[i];
        float farSplit = cascadeDistances[i + 1];
        cascades[i].lightSpaceMatrix = calculateLightMatrix(camera, nearSplit, farSplit, sunlight->direction, i);
        cascades[i].nearDist = nearSplit;
        cascades[i].farDist = farSplit;
    }

    depthShader.use();

    for (int i = 0; i < MAX_CASCADES; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, cascades[i].fbo);
        glViewport(0, 0, resolution, resolution);
        glClear(GL_DEPTH_BUFFER_BIT);

        depthShader.use();
        depthShader.setUniform("lightSpaceMatrix", cascades[i].lightSpaceMatrix);

        for (auto& obj : objects) {
            if (!obj->visible) continue;
            depthShader.setUniform("model", obj->getTransform());
            obj->render();
        }
    }

    glDisable(GL_CULL_FACE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowMap::bindCascadeTextures(unsigned int startUnit) const {
    for (int i = 0; i < MAX_CASCADES; ++i) {
        glActiveTexture(GL_TEXTURE0 + startUnit + i);
        glBindTexture(GL_TEXTURE_2D, cascades[i].depthTex);
    }
}