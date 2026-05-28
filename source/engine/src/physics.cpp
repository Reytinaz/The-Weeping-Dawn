#include "physics.hpp"

void Physics::addObject(std::shared_ptr<Object3D> obj) {
    if (obj) {
        objects.push_back(obj);
    }
}
void Physics::removeObject(std::shared_ptr<Object3D> obj) {
    objects.erase(std::remove(objects.begin(), objects.end(), obj), objects.end());
}

void Physics::clear() {
    objects.clear();
}

void Physics::setChunkManager(ChunkManager* terr) {
    chunkManager = terr;
}
bool Physics::raycast(const Vector3& origin, const Vector3& direction, float maxDistance, Vector3& hitPoint, std::shared_ptr<Object3D>& hitObject, std::vector<std::shared_ptr<Object3D>> ignoreList) {
    float closestDist = maxDistance;
    bool hit = false;
    for (auto& obj : objects) {
        for (auto& objIgnore : ignoreList) {
            if (obj == objIgnore) {
                continue;
            }
        }
        if (!obj->canRaycast) continue;

        Vector3 halfSize = obj->scale * 0.5f;
        Vector3 minWorld = obj->position - halfSize;
        Vector3 maxWorld = obj->position + halfSize;

        float tmin = 0.0f;
        float tmax = maxDistance;

        // X
        if (std::abs(direction.x) > 1e-6f) {
            float invDirX = 1.0f / direction.x;
            float tx1 = (minWorld.x - origin.x) * invDirX;
            float tx2 = (maxWorld.x - origin.x) * invDirX;
            if (tx1 > tx2) std::swap(tx1, tx2);
            tmin = std::max(tmin, tx1);
            tmax = std::min(tmax, tx2);
        }
        else {
            if (origin.x < minWorld.x || origin.x > maxWorld.x) continue;
        }

        // Y
        if (std::abs(direction.y) > 1e-6f) {
            float invDirY = 1.0f / direction.y;
            float ty1 = (minWorld.y - origin.y) * invDirY;
            float ty2 = (maxWorld.y - origin.y) * invDirY;
            if (ty1 > ty2) std::swap(ty1, ty2);
            tmin = std::max(tmin, ty1);
            tmax = std::min(tmax, ty2);
        }
        else {
            if (origin.y < minWorld.y || origin.y > maxWorld.y) continue;
        }

        // Z
        if (std::abs(direction.z) > 1e-6f) {
            float invDirZ = 1.0f / direction.z;
            float tz1 = (minWorld.z - origin.z) * invDirZ;
            float tz2 = (maxWorld.z - origin.z) * invDirZ;
            if (tz1 > tz2) std::swap(tz1, tz2);
            tmin = std::max(tmin, tz1);
            tmax = std::min(tmax, tz2);
        }
        else {
            if (origin.z < minWorld.z || origin.z > maxWorld.z) continue;
        }

        if (tmin <= tmax && tmin >= 0.0f && tmin < closestDist) {
            closestDist = tmin;
            hitPoint = origin + direction * tmin;
            hitObject = obj;
            hit = true;
        }
    }
    return hit;
}

bool Physics::raycast(const Vector3& origin, const Vector3& direction, float maxDistance, Vector3& hitPoint, std::shared_ptr<Object3D>& hitObject) {
    float closestDist = maxDistance;
    bool hit = false;
    for (auto& obj : objects) {
        if (!obj->canRaycast) continue;

        Vector3 halfSize = obj->scale * 0.5f;
        Vector3 minWorld = obj->position - halfSize;
        Vector3 maxWorld = obj->position + halfSize;

        float tmin = 0.0f;
        float tmax = maxDistance;

        // X
        if (std::abs(direction.x) > 1e-6f) {
            float invDirX = 1.0f / direction.x;
            float tx1 = (minWorld.x - origin.x) * invDirX;
            float tx2 = (maxWorld.x - origin.x) * invDirX;
            if (tx1 > tx2) std::swap(tx1, tx2);
            tmin = std::max(tmin, tx1);
            tmax = std::min(tmax, tx2);
        }
        else {
            if (origin.x < minWorld.x || origin.x > maxWorld.x) continue;
        }

        // Y
        if (std::abs(direction.y) > 1e-6f) {
            float invDirY = 1.0f / direction.y;
            float ty1 = (minWorld.y - origin.y) * invDirY;
            float ty2 = (maxWorld.y - origin.y) * invDirY;
            if (ty1 > ty2) std::swap(ty1, ty2);
            tmin = std::max(tmin, ty1);
            tmax = std::min(tmax, ty2);
        }
        else {
            if (origin.y < minWorld.y || origin.y > maxWorld.y) continue;
        }

        // Z
        if (std::abs(direction.z) > 1e-6f) {
            float invDirZ = 1.0f / direction.z;
            float tz1 = (minWorld.z - origin.z) * invDirZ;
            float tz2 = (maxWorld.z - origin.z) * invDirZ;
            if (tz1 > tz2) std::swap(tz1, tz2);
            tmin = std::max(tmin, tz1);
            tmax = std::min(tmax, tz2);
        }
        else {
            if (origin.z < minWorld.z || origin.z > maxWorld.z) continue;
        }

        if (tmin <= tmax && tmin >= 0.0f && tmin < closestDist) {
            closestDist = tmin;
            hitPoint = origin + direction * tmin;
            hitObject = obj;
            hit = true;
        }
    }
    return hit;
}

struct Contact {
    std::shared_ptr<Object3D> a;
    std::shared_ptr<Object3D> b;
    Vector3 normal;
    Vector3 contactPoint;
    float penetration = 0.f;
};


static void getOBB(const std::shared_ptr<Object3D>& obj, Vector3 axes[3], Vector3& center, Vector3& halfSize) {
    center = obj->position;
    halfSize = obj->scale * 0.5f;

    float yaw = obj->rotation.y;
    float pitch = obj->rotation.x;
    float roll = obj->rotation.z;

    float cy = cos(yaw);   float sy = sin(yaw);
    float cp = cos(pitch); float sp = sin(pitch);
    float cr = cos(roll);  float sr = sin(roll);

    axes[0].x = cy * cr + sy * sp * sr;
    axes[0].y = cp * sr;
    axes[0].z = -sy * cr + cy * sp * sr;

    axes[1].x = -cy * sr + sy * sp * cr;
    axes[1].y = cp * cr;
    axes[1].z = sy * sr + cy * sp * cr;

    axes[2].x = sy * cp;
    axes[2].y = -sp;
    axes[2].z = cy * cp;
}

static void projectOBB(const Vector3& axis, const Vector3& center, const Vector3 axes[3], const Vector3& halfSize, float& min, float& max) {
    float p = axis.dot(center);
    float e = 0.0f;
    for (int i = 0; i < 3; ++i) {
        float dot = axis.dot(axes[i]);
        float extent = (i == 0) ? halfSize.x : (i == 1 ? halfSize.y : halfSize.z);
        e += std::abs(dot) * extent;
    }
    min = p - e;
    max = p + e;
}

static bool checkOBBvsOBB(const std::shared_ptr<Object3D>& a, const std::shared_ptr<Object3D>& b, Contact& contact) {
    Vector3 axesA[3], axesB[3];
    Vector3 centerA, centerB, halfA, halfB;
    getOBB(a, axesA, centerA, halfA);
    getOBB(b, axesB, centerB, halfB);

    std::vector<Vector3> axes;
    for (int i = 0; i < 3; ++i) {
        axes.push_back(axesA[i]);
        axes.push_back(axesB[i]);
    }
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            Vector3 cross = axesA[i].cross(axesB[j]);
            if (cross.length() > 1e-6f) {
                axes.push_back(cross.normalized());
            }
        }
    }

    float minOverlap = std::numeric_limits<float>::max();
    Vector3 bestNormal(0, 0, 0);

    for (const Vector3& axis : axes) {
        float minA, maxA, minB, maxB;
        projectOBB(axis, centerA, axesA, halfA, minA, maxA);
        projectOBB(axis, centerB, axesB, halfB, minB, maxB);

        if (maxA < minB || maxB < minA) return false;

        float overlap = std::min(maxA, maxB) - std::max(minA, minB);
        if (overlap < minOverlap) {
            minOverlap = overlap;
            bestNormal = axis;
            if (centerB.dot(axis) < centerA.dot(axis)) bestNormal = -axis;
        }
    }

    contact.a = a;
    contact.b = b;
    contact.normal = bestNormal.normalized();
    contact.penetration = minOverlap;
    contact.contactPoint = (centerA + centerB) * 0.5f + bestNormal * (minOverlap * 0.5f);
    return true;
}

static void resolveContact(Contact& contact, float dt) {
    auto& a = contact.a;
    auto& b = contact.b;
    if (a->isStatic && b->isStatic) return;

    Vector3 n = contact.normal;

    const float slop = 0.01f;           // допуск проникновения
    const float percent = 0.1f;          // доля коррекции за итерацию
    const float maxCorrection = 0.1f;    // максимум смещения
    float depth = contact.penetration - slop;
    if (depth > 0) {
        float correctionAmount = std::min(depth * percent, maxCorrection);
        Vector3 correction = n * correctionAmount;
        if (!a->isStatic && !b->isStatic) {
            a->position -= correction * 0.5f;
            b->position += correction * 0.5f;
        }
        else if (!a->isStatic) {
            a->position -= correction;
        }
        else if (!b->isStatic) {
            b->position += correction;
        }

        if (!a->isStatic) {
            float va = a->velocity.dot(n);
            a->velocity -= n * va;
        }
        if (!b->isStatic) {
            float vb = b->velocity.dot(n);
            b->velocity -= n * vb;
        }
    }

    Vector3 ra = contact.contactPoint - a->position;
    Vector3 rb = contact.contactPoint - b->position;

    Vector3 velA = a->velocity + a->angularVelocity.cross(ra);
    Vector3 velB = b->velocity + b->angularVelocity.cross(rb);
    Vector3 vRel = velB - velA;
    float vn = vRel.dot(n);

    if (vn > 0.0f) return;

    float invMassA = a->isStatic ? 0.0f : 1.0f / a->mass;
    float invMassB = b->isStatic ? 0.0f : 1.0f / b->mass;

    float Ia = a->mass * (a->scale.x * a->scale.x + a->scale.y * a->scale.y + a->scale.z * a->scale.z) / 12.0f;
    float Ib = b->mass * (b->scale.x * b->scale.x + b->scale.y * b->scale.y + b->scale.z * b->scale.z) / 12.0f;
    float invIa = (a->isStatic || Ia < 1e-6f) ? 0.0f : 1.0f / Ia;
    float invIb = (b->isStatic || Ib < 1e-6f) ? 0.0f : 1.0f / Ib;

    Vector3 raCrossN = ra.cross(n);
    Vector3 rbCrossN = rb.cross(n);

    float angularFactorA = raCrossN.dot(raCrossN) * invIa;
    float angularFactorB = rbCrossN.dot(rbCrossN) * invIb;

    float effectiveMass = 1.0f / (invMassA + invMassB + angularFactorA + angularFactorB);
    float impulseMagnitude = -vn * effectiveMass;

    const float MAX_DELTA_V = 2.0f;
    float maxImpulseForA = (invMassA > 0.0f) ? MAX_DELTA_V / invMassA : std::numeric_limits<float>::max();
    float maxImpulseForB = (invMassB > 0.0f) ? MAX_DELTA_V / invMassB : std::numeric_limits<float>::max();
    float maxAllowedImpulse = std::min(maxImpulseForA, maxImpulseForB);
    impulseMagnitude = std::clamp(impulseMagnitude, -maxAllowedImpulse, 0.0f);

    Vector3 impulse = n * impulseMagnitude;

    if (!a->isStatic) {
        a->velocity -= impulse * invMassA;
        a->angularVelocity += raCrossN * impulseMagnitude * invIa;
    }
    if (!b->isStatic) {
        b->velocity += impulse * invMassB;
        b->angularVelocity -= rbCrossN * impulseMagnitude * invIb;
    }

    if ((b->isStatic && !a->isStatic) || (a->isStatic && !b->isStatic)) {
        auto& dyn = !a->isStatic ? a : b;

        Vector3 tangent = vRel - n * vn;
        float vt = tangent.length();
        if (vt > 0.01f) {
            tangent = tangent / vt;
            float frictionFactor = 1.0f;
            dyn->velocity -= tangent * dyn->velocity.dot(tangent) * (1.0f - frictionFactor);
        }
    }

    if (b->isStatic && std::abs(n.y) > 0.9f && !a->isStatic) {
        a->velocity.y = 0.0f;
        a->angularVelocity.x *= 0.5f;
        a->angularVelocity.z *= 0.5f;
    }
    if (a->isStatic && std::abs(n.y) > 0.9f && !b->isStatic) {
        b->velocity.y = 0.0f;
        b->angularVelocity.x *= 0.5f;
        b->angularVelocity.z *= 0.5f;
    }

    if (!a->isStatic) a->angularVelocity *= 0.9f;
    if (!b->isStatic) b->angularVelocity *= 0.9f;
}

void Physics::update(float deltaTime) {
    deltaTime = std::min(deltaTime, 0.02f);

    const float maxFallSpeed = 15.0f;
    const float linearDamping = 0.995f;

    for (auto& obj : objects) {
        if (!obj->isStatic) {
            if (obj->useGravity) {
                obj->velocity += gravity * deltaTime;
            }

            if (obj->velocity.y > maxFallSpeed) obj->velocity.y = maxFallSpeed;
            if (obj->velocity.y < -maxFallSpeed) obj->velocity.y = -maxFallSpeed;

            obj->velocity *= linearDamping;
            obj->angularVelocity *= 0.95f;

            obj->position += obj->velocity * deltaTime;
            obj->rotation += obj->angularVelocity * deltaTime;
        }
    }

    const int iterations = 8;
    for (int iter = 0; iter < iterations; ++iter) {
        std::vector<Contact> contacts;
        for (size_t i = 0; i < objects.size(); ++i) {
            for (size_t j = i + 1; j < objects.size(); ++j) {
                auto a = objects[i];
                auto b = objects[j];
                if (a->isStatic && b->isStatic) continue;
                Contact contact;
                if (checkOBBvsOBB(a, b, contact)) {
                    contacts.push_back(contact);
                }
            }
        }
        if (contacts.empty()) break;
        for (auto& contact : contacts) {
            resolveContact(contact, deltaTime);
        }
    }

    for (auto& obj : objects) {
        if (!obj->isStatic) {
            if (obj->velocity.length() < 0.01f) obj->velocity = Vector3(0, 0, 0);
            if (obj->angularVelocity.length() < 0.01f) obj->angularVelocity = Vector3(0, 0, 0);

            if (chunkManager) {
                float bottomY = obj->position.y - obj->scale.y * 0.5f;
                float terrainY = chunkManager->getHeightAt(obj->position.x, obj->position.z);
                if (bottomY < terrainY) {
                    obj->position.y = terrainY + obj->scale.y * 0.5f;
                    if (obj->velocity.y < 0.0f) obj->velocity.y = 0.0f;
                    obj->velocity.x *= 0.9f;
                    obj->velocity.z *= 0.9f;
                    obj->angularVelocity.x *= 0.5f;
                    obj->angularVelocity.z *= 0.5f;
                }
            }
        }
    }
}