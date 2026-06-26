#include "SourceLight.hpp"

SourceLight::SourceLight(const std::string& name) : type(LightType::DIRECTIONAL_LIGHT), position(0, 0, 0), direction(0, -1, 0),
	color(1, 1, 1), Instance(name, "SourceLight") {
}