#include "Object3D.hpp"
#include "glad.h"
#include "fstream"

Object3D::Object3D(const std::string& name, ObjectType type) : Instance(name, "Object3D") {
    this->type = type;
    position = Vector3(0, 0, 0);
    rotation = Vector3(0, 0, 0);
    scale = Vector3(1, 1, 1);
    modelScale = Vector3(1, 1, 1);
    mass = 1.f;
    restitution = 0.5f;
    friction = 0.5f;
    isStatic = false;
    useGravity = true;
    visible = true;
    VAO = VBO = EBO = 0;
    vertexCount = indexCount = 0;
    initialized = false;
    color[0] = color[1] = color[2] = 1.0f; 
    diffuseTexture = normalTexture = nullptr;
    canRaycast = true;
}

Object3D::~Object3D() { cleanup(); }

Object3D::Object3D(Object3D&& other) noexcept
    : type(other.type),
    position(other.position), rotation(other.rotation), scale(other.scale),
    velocity(other.velocity), mass(other.mass), restitution(other.restitution),
    friction(other.friction), isStatic(other.isStatic), useGravity(other.useGravity), VAO(other.VAO), VBO(other.VBO), EBO(other.EBO),
    vertexCount(other.vertexCount), indexCount(other.indexCount),
    initialized(other.initialized), diffuseTexture(other.diffuseTexture), normalTexture(other.normalTexture) {
    cleanup();
    for (int i = 0; i < 3; ++i) color[i] = other.color[i];
    other.VAO = other.VBO = other.EBO = 0;
    other.initialized = false;
    canRaycast = other.canRaycast;
    normalTexturePath = other.normalTexturePath;
    diffuseTexturePath = other.diffuseTexturePath;
    modelPath = other.modelPath;
}

Object3D& Object3D::operator=(Object3D&& other) noexcept {
    if (this != &other) {
        cleanup();
        type = other.type;
        position = other.position;
        rotation = other.rotation;
        scale = other.scale;
        velocity = other.velocity;
        mass = other.mass;
        restitution = other.restitution;
        friction = other.friction;
        isStatic = other.isStatic;
        useGravity = other.useGravity;
        visible = other.visible;
        for (int i = 0; i < 3; ++i) color[i] = other.color[i];
        VAO = other.VAO; VBO = other.VBO; EBO = other.EBO;
        vertexCount = other.vertexCount; indexCount = other.indexCount;
        initialized = other.initialized;
        other.VAO = other.VBO = other.EBO = 0;
        other.initialized = false;
        diffuseTexture = other.diffuseTexture; 
        normalTexture = other.normalTexture;
        canRaycast = other.canRaycast;
        normalTexturePath = other.normalTexturePath;
        diffuseTexturePath = other.diffuseTexturePath;
        modelPath = other.modelPath;
    }
    return *this;
}

Matrix4 Object3D::getTransform() const {
    return Matrix4::translation(position) *
        Matrix4::rotationY(rotation.y) *
        Matrix4::rotationX(rotation.x) *
        Matrix4::rotationZ(rotation.z) *
        Matrix4::scale(scale);
}

bool Object3D::initOpenGL() {
    if (initialized) return true;
    switch (type) {
    case ObjectType::CUBE: generateCubeGeometry(); break;
    case ObjectType::PLANE: generatePlaneGeometry(); break;
    case ObjectType::SPHERE: generateSphereGeometry(16); break;
    case ObjectType::PYRAMID: generatePyramidGeometry(); break;
    case ObjectType::CYLINDER: generateCylinderGeometry(16); break;
    default: generateCubeGeometry(); break;
    }
    initialized = true;
    return true;
}

void Object3D::render() const {
    if (!visible || !initialized) return;
    glBindVertexArray(VAO);
    if (indexCount > 0)
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    else
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    glBindVertexArray(0);
}

void Object3D::cleanup() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);
    VAO = VBO = EBO = 0;
    initialized = false;
}

void Object3D::setDiffuseTexture(const std::string& filepath) {
    auto tex = std::make_shared<Texture>();
    if (tex->loadFromFile(filepath, false)) {
        diffuseTexture = tex;
        diffuseTexturePath = filepath;
    }
    else {
        std::cerr << "Failed to load diffuse texture: " << filepath << std::endl;
    }
}
void Object3D::setNormalTexture(const std::string& filepath) {
    auto tex = std::make_shared<Texture>();
    if (tex->loadFromFile(filepath, false)) {
        normalTexture = tex;
        normalTexturePath = filepath;
    }
    else {
        std::cerr << "Failed to load normal texture: " << filepath << std::endl;
    }
}
void Object3D::setDiffuseTexture(std::shared_ptr<Texture> texture) {
    diffuseTexture = texture;
}
void Object3D::setNormalTexture(std::shared_ptr<Texture> texture) {
    normalTexture = texture;
}

void Object3D::getWorldBounds(Vector3& worldMin, Vector3& worldMax) const {
    if (vertices.empty()) {
        worldMin = position;
        worldMax = position;
        return;
    }
    // Предполагаем, что minBound и maxBound уже вычислены (например, в loadOBJ)
    Vector3 corners[8] = {
        Vector3(minBound.x, minBound.y, minBound.z),
        Vector3(maxBound.x, minBound.y, minBound.z),
        Vector3(minBound.x, maxBound.y, minBound.z),
        Vector3(maxBound.x, maxBound.y, minBound.z),
        Vector3(minBound.x, minBound.y, maxBound.z),
        Vector3(maxBound.x, minBound.y, maxBound.z),
        Vector3(minBound.x, maxBound.y, maxBound.z),
        Vector3(maxBound.x, maxBound.y, maxBound.z)
    };
    Matrix4 modelMat = getTransform();
    worldMin = Vector3(INFINITY, INFINITY, INFINITY);
    worldMax = Vector3(-INFINITY, -INFINITY, -INFINITY);
    for (int i = 0; i < 8; ++i) {
        Vector3 worldCorner = modelMat.transformPoint(corners[i]);
        worldMin.x = std::min(worldMin.x, worldCorner.x);
        worldMin.y = std::min(worldMin.y, worldCorner.y);
        worldMin.z = std::min(worldMin.z, worldCorner.z);
        worldMax.x = std::max(worldMax.x, worldCorner.x);
        worldMax.y = std::max(worldMax.y, worldCorner.y);
        worldMax.z = std::max(worldMax.z, worldCorner.z);
    }
}

void Object3D::generateCubeGeometry() {
    float vertices[] = {
        // positions          // normals
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f
    };

    unsigned int indices[] = {
         0,  1,  2,  2,  3,  0,
         4,  5,  6,  6,  7,  4,
         8,  9, 10, 10, 11,  8,
        12, 13, 14, 14, 15, 12,
        16, 17, 18, 18, 19, 16,
        20, 21, 22, 22, 23, 20
    };

    vertexCount = 24;
    indexCount = 36;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void Object3D::generatePlaneGeometry() {
    float vertices[] = { -0.5f,0,-0.5f, 0.5f,0,-0.5f, 0.5f,0,0.5f, -0.5f,0,0.5f };
    unsigned int indices[] = { 0,1,2, 0,2,3 };
    vertexCount = 4; indexCount = 6;
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO); glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void Object3D::generateSphereGeometry(int segments) {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    float radius = 0.5f;

    for (int i = 0; i <= segments; ++i) {
        float phi = i * 3.14159f / segments;
        float sinPhi = sin(phi);
        float cosPhi = cos(phi);
        for (int j = 0; j <= segments; ++j) {
            float theta = j * 2 * 3.14159f / segments;
            float sinTheta = sin(theta);
            float cosTheta = cos(theta);

            float x = sinPhi * cosTheta * radius;
            float y = cosPhi * radius;
            float z = sinPhi * sinTheta * radius;

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            Vector3 normal(x, y, z);
            normal = normal.normalized();
            vertices.push_back(normal.x);
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);
        }
    }

    for (int i = 0; i < segments; ++i) {
        for (int j = 0; j < segments; ++j) {
            int first = i * (segments + 1) + j;
            int second = first + segments + 1;

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);

            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }

    vertexCount = vertices.size() / 6;
    indexCount = indices.size();


    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void Object3D::generatePyramidGeometry() {
    float vertices[] = {
        0.0f, 0.5f, 0.0f,
        -0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, -0.5f,
        0.5f, -0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f
    };

    unsigned int indices[] = {
        0,1,2, 0,2,3, 0,3,4, 0,4,1,
        1,3,2, 1,4,3
    };

    vertexCount = 5;
    indexCount = 18;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void Object3D::generateCylinderGeometry(int segments) {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    float radius = 0.5f;
    float halfHeight = 0.5f;

    for (int i = 0; i <= segments; ++i) {
        float theta = i * 2 * 3.14159f / segments;
        float cosTheta = cos(theta);
        float sinTheta = sin(theta);

        vertices.push_back(radius * cosTheta);
        vertices.push_back(halfHeight);
        vertices.push_back(radius * sinTheta);

        vertices.push_back(radius * cosTheta);
        vertices.push_back(-halfHeight);
        vertices.push_back(radius * sinTheta);
    }

    int topCenter = (segments + 1) * 2;
    int bottomCenter = topCenter + 1;

    vertices.push_back(0); vertices.push_back(halfHeight); vertices.push_back(0);
    vertices.push_back(0); vertices.push_back(-halfHeight); vertices.push_back(0);

    for (int i = 0; i < segments; ++i) {
        int top1 = i * 2;
        int bottom1 = top1 + 1;
        int top2 = ((i + 1) % segments) * 2;
        int bottom2 = top2 + 1;

        indices.push_back(top1);
        indices.push_back(bottom1);
        indices.push_back(top2);

        indices.push_back(bottom1);
        indices.push_back(bottom2);
        indices.push_back(top2);
    }

    for (int i = 0; i < segments; ++i) {
        int top1 = i * 2;
        int top2 = ((i + 1) % segments) * 2;

        indices.push_back(topCenter);
        indices.push_back(top1);
        indices.push_back(top2);
    }

    for (int i = 0; i < segments; ++i) {
        int bottom1 = i * 2 + 1;
        int bottom2 = ((i + 1) % segments) * 2 + 1;

        indices.push_back(bottomCenter);
        indices.push_back(bottom2);
        indices.push_back(bottom1);
    }

    vertexCount = vertices.size() / 3;
    indexCount = indices.size();

    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void Object3D::applyModelScale() {
    if (vertices.empty()) {
        return;
    }
    for (auto& v : vertices) {
        v.x *= modelScale.x;
        v.y *= modelScale.y;
        v.z *= modelScale.z;
    }
    setupBuffers(true);
}

bool Object3D::loadOBJ(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    std::vector<Vector3> tempPositions;
    std::vector<Vector3> tempNormals;
    std::vector<Vector2> tempTexCoords;
    std::unordered_map<std::string, unsigned int> vertexMap;

    std::string mtlFilename;
    std::string baseDir = filename.substr(0, filename.find_last_of("/\\") + 1);

    std::string line;
    unsigned int lineNum = 0;

    while (std::getline(file, line)) {
        lineNum++;
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "mtllib") {
            iss >> mtlFilename;
        }
        else if (prefix == "v") {
            Vector3 v;
            iss >> v.x >> v.y >> v.z;
            tempPositions.push_back(v);
        }
        else if (prefix == "vt") {
            Vector2 vt;
            iss >> vt.u >> vt.v;
            tempTexCoords.push_back(vt);
        }
        else if (prefix == "vn") {
            Vector3 vn;
            iss >> vn.x >> vn.y >> vn.z;
            tempNormals.push_back(vn);
        }
        else if (prefix == "f") {
            std::vector<std::string> tokens;
            std::string token;
            while (iss >> token) tokens.push_back(token);

            if (tokens.size() < 3) continue;
            auto addVertex = [&](const std::string& tok) {
                auto it = vertexMap.find(tok);
                if (it == vertexMap.end()) {
                    std::vector<std::string> parts;
                    std::string s = tok;
                    size_t pos;
                    while ((pos = s.find('/')) != std::string::npos) {
                        parts.push_back(s.substr(0, pos));
                        s.erase(0, pos + 1);
                    }
                    parts.push_back(s);

                    int vIdx = 0, vtIdx = 0, vnIdx = 0;
                    if (parts.size() >= 1 && !parts[0].empty()) vIdx = std::stoi(parts[0]);
                    if (parts.size() >= 2 && !parts[1].empty()) vtIdx = std::stoi(parts[1]);
                    if (parts.size() >= 3 && !parts[2].empty()) vnIdx = std::stoi(parts[2]);

                    Vector3 vecPos = (vIdx > 0 && vIdx <= (int)tempPositions.size()) ? tempPositions[vIdx - 1] : Vector3(0, 0, 0);
                    Vector3 norm = (vnIdx > 0 && vnIdx <= (int)tempNormals.size()) ? tempNormals[vnIdx - 1] : Vector3(0, 0, 0);
                    Vector2 tex = (vtIdx > 0 && vtIdx <= (int)tempTexCoords.size()) ? tempTexCoords[vtIdx - 1] : Vector2(0, 0);

                    unsigned int newIdx = vertices.size();
                    vertices.push_back(vecPos);
                    normals.push_back(norm);
                    texCoords.push_back(tex);
                    vertexMap[tok] = newIdx;
                    indices.push_back(newIdx);
                }
                else {
                    indices.push_back(it->second);
                }
            };

            for (size_t i = 1; i < tokens.size() - 1; ++i) {
                addVertex(tokens[0]);
                addVertex(tokens[i]);
                addVertex(tokens[i + 1]);
            }
        }
    }

    file.close();

    if (vertices.empty()) {
        return false;
    }

    if (normals.empty() || std::all_of(normals.begin(), normals.end(), [](const Vector3& n) { return n.length() < 1e-6; })) {
        computeNormals();
    }

    if (!mtlFilename.empty()) {
        std::string mtlPath = baseDir + mtlFilename;
        std::ifstream mtlFile(mtlPath);
        if (mtlFile.is_open()) {
            std::string mtlLine;
            while (std::getline(mtlFile, mtlLine)) {
                if (mtlLine.empty() || mtlLine[0] == '#') continue;
                std::istringstream mtlIss(mtlLine);
                std::string mtlPrefix;
                mtlIss >> mtlPrefix;

                if (mtlPrefix == "newmtl") {

                }
                else if (mtlPrefix == "map_Kd") {
                    std::string texFile;
                    mtlIss >> texFile;
                    std::string texPath = baseDir + texFile;
                    setDiffuseTexture(texPath);
                }
                else if (mtlPrefix == "map_Bump" || mtlPrefix == "bump" || mtlPrefix == "map_Kn") {
                    std::string texFile;
                    mtlIss >> texFile;
                    std::string texPath = baseDir + texFile;
                    setNormalTexture(texPath);
                }
            }
            mtlFile.close();
        }
    }

    computeBounds();
    normalizeScale();
    setupBuffers();
    modelPath = filename;
    return true;
}

void Object3D::computeNormals() {
    normals.assign(vertices.size(), Vector3(0, 0, 0));

    for (size_t i = 0; i < indices.size(); i += 3) {
        unsigned int i1 = indices[i];
        unsigned int i2 = indices[i + 1];
        unsigned int i3 = indices[i + 2];

        Vector3 v1 = vertices[i2] - vertices[i1];
        Vector3 v2 = vertices[i3] - vertices[i1];
        Vector3 faceNormal = v1.cross(v2).normalized();

        normals[i1] += faceNormal;
        normals[i2] += faceNormal;
        normals[i3] += faceNormal;
    }

    for (auto& n : normals) {
        float len = n.length();
        if (len > 1e-6) {
            n = n * (1.0f / len);
        }
    }
}
void Object3D::computeBounds() {
    if (vertices.empty()) return;
    minBound = vertices[0];
    maxBound = vertices[0];
    for (const auto& v : vertices) {
        if (v.x < minBound.x) minBound.x = v.x;
        if (v.y < minBound.y) minBound.y = v.y;
        if (v.z < minBound.z) minBound.z = v.z;
        if (v.x > maxBound.x) maxBound.x = v.x;
        if (v.y > maxBound.y) maxBound.y = v.y;
        if (v.z > maxBound.z) maxBound.z = v.z;
    }
}
void Object3D::normalizeScale() {
    if (vertices.empty()) return;
    computeBounds();
    Vector3 size = maxBound - minBound;
    float maxSize = std::max(size.x, std::max(size.y, size.z));
    if (maxSize < 1e-6f) return;

    float invMax = 1.0f / maxSize;
    Vector3 center = (minBound + maxBound) * 0.5f;

    for (auto& v : vertices) {
        v = (v - center) * invMax;
    }

    computeNormals();
    computeBounds();
    setupBuffers();
}
void Object3D::setupBuffers(bool ignore) {
    if (!ignore) {
        if (vertices.empty() || initialized) return;
    }

    struct Vertex {
        float px, py, pz;
        float nx, ny, nz;
        float tu, tv;
    };

    std::vector<Vertex> vertexData;
    vertexData.reserve(vertices.size());

    for (size_t i = 0; i < vertices.size(); ++i) {
        Vertex v;
        v.px = vertices[i].x;
        v.py = vertices[i].y;
        v.pz = vertices[i].z;

        if (i < normals.size()) {
            v.nx = normals[i].x;
            v.ny = normals[i].y;
            v.nz = normals[i].z;
        }
        else {
            v.nx = v.ny = v.nz = 0.0f;
        }

        if (i < texCoords.size()) {
            v.tu = texCoords[i].u;
            v.tv = texCoords[i].v;
        }
        else {
            v.tu = v.tv = 0.0f;
        }

        vertexData.push_back(v);
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(Vertex), vertexData.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Атрибут 0: позиция
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, px));
    glEnableVertexAttribArray(0);

    // Атрибут 1: нормаль
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, nx));
    glEnableVertexAttribArray(1);

    // Атрибут 2: текстурные координаты
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tu));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    indexCount = static_cast<unsigned int>(indices.size());
    vertexCount = static_cast<unsigned int>(vertices.size());
    initialized = true;
}