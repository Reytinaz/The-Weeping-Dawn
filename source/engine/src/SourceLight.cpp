#include "SourceLight.hpp"

SourceLight::SourceLight(const std::string& name) : type(LightType::DIRECTIONAL_LIGHT), position(0, 0, 0), direction(0, -1, 0),
	color(1, 1, 1), intensity(1.0f), constant(1.0f), linear(0.09f),
	quadratic(0.032f), cutOff(12.5f), outerCutOff(15.0f), Instance(name, "SourceLight") {
}