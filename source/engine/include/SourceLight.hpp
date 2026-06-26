#ifndef SOURCE_LIGHT_HPP
#define SOURCE_LIGHT_HPP

#include "vector3.hpp"
#include "instance.hpp"

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
    float intensity = 1.0f;
    float constant = 1.0f;   // обычно 1.0
    float linear = 0.09f;  // подбирается под размер сцены
    float quadratic = 0.032f;
    float cutOff = 12.5f;   // градусы, внутренний конус
    float outerCutOff = 17.5f;   // градусы, внешний конус
    bool enabled = false;

    SourceLight(const std::string& name);
};

#endif