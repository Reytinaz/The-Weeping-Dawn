#ifndef SOURCE_LIGHT_HPP
#define SOURCE_LIGHT_HPP

#include "vector3.hpp"
#include "Instance.hpp"

enum LightType {
    DIRECTIONAL_LIGHT,
    POINT_LIGHT,
    SPOT_LIGHT
};

class SourceLight : public Instance {
public:
    LightType type;
    Vector3 position;
    Vector3 direction;
    Vector3 color;
    float intensity;
    float constant;
    float linear;
    float quadratic;
    float cutOff;
    float outerCutOff;
    bool enabled = false;

    bool enableGodRays = false;
    float rayExposure = 0.3f;
    float rayDecay = 0.97f;
    float rayDensity = 0.9f;
    float rayWeight = 0.1f;
    float raySize = 0.05f;

    SourceLight(const std::string& name);
};

#endif