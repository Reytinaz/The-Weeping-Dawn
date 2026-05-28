#ifndef PHYSICS_HPP
#define PHYSICS_HPP

#include "object3d.hpp"
#include "chunkManager.hpp"

class Physics {
public:
    Physics() : chunkManager(nullptr) {};
    ~Physics() = default;

    void addObject(std::shared_ptr<Object3D> obj);
    void removeObject(std::shared_ptr<Object3D> obj);
    void clear();
    void update(float deltaTime);
    void setChunkManager(ChunkManager* chunkManage);
    std::vector<std::shared_ptr<Object3D>>& getObjects() { return objects; }
    bool raycast(const Vector3& origin, const Vector3& direction, float maxDistance, Vector3& hitPoint, std::shared_ptr<Object3D>& hitObject, std::vector<std::shared_ptr<Object3D>> ignoreList);
    bool raycast(const Vector3& origin, const Vector3& direction, float maxDistance, Vector3& hitPoint, std::shared_ptr<Object3D>& hitObject);

    Vector3 gravity = Vector3(0.0f, -9.81f, 0.0f);
private:
    std::vector<std::shared_ptr<Object3D>> objects;
    ChunkManager* chunkManager;
};

#endif