#ifndef OBJECT3D_HPP
#define OBJECT3D_HPP

#include "matrix4.hpp"
#include "instance.hpp"
#include "texture.hpp"

enum class ObjectType { CUBE, SPHERE, PYRAMID, CYLINDER, PLANE, CUSTOM };

class Object3D : public Instance, public std::enable_shared_from_this<Object3D> {
public:
    ObjectType type;

    Vector3 position;
    Vector3 rotation; // radians
    Vector3 scale;
    Vector3 velocity;
    Vector3 angularVelocity;

    float mass;
    float restitution;  // упругость
    float friction;      // трение
    bool isStatic;
    bool useGravity;
    bool canRaycast;

    std::shared_ptr<Texture> diffuseTexture; 
    std::shared_ptr<Texture> normalTexture;
    std::string diffuseTexturePath;
    std::string normalTexturePath;
    std::string modelPath;
    Vector3 modelScale;

    unsigned int VAO, VBO, EBO;
    size_t vertexCount, indexCount;
    bool initialized;

    float color[3];

    Object3D(const std::string& name = "Cube", ObjectType type = ObjectType::CUBE);
    virtual ~Object3D() override;

    Object3D(const Object3D&) = delete;
    Object3D& operator=(const Object3D&) = delete;
    Object3D(Object3D&& other) noexcept;
    Object3D& operator=(Object3D&& other) noexcept;

    void move(const Vector3& delta) { position = position + delta; }
    void rotate(const Vector3& delta) { rotation = rotation + delta; }

    Matrix4 getTransform() const;

    bool initOpenGL();
    void render() const;
    void cleanup();

    void setDiffuseTexture(const std::string& filepath);
    void setNormalTexture(const std::string& filepath);
    void setDiffuseTexture(std::shared_ptr<Texture> texture);
    void setNormalTexture(std::shared_ptr<Texture> texture);

    void getWorldBounds(Vector3& worldMin, Vector3& worldMax) const;

    void generateCubeGeometry();
    void generatePlaneGeometry();
    void generateSphereGeometry(int segments = 16);
    void generatePyramidGeometry();
    void generateCylinderGeometry(int segments = 16);

    std::vector<Vector3> vertices;       
    std::vector<Vector3> normals;         
    std::vector<Vector2> texCoords;      
    std::vector<unsigned int> indices; 

    Vector3 minBound;
    Vector3 maxBound;

    bool loadOBJ(const std::string& filename);
    void computeNormals();
    void computeBounds();
    void normalizeScale();
    void setupBuffers(bool ignore = false);
    void applyModelScale();
};

#endif