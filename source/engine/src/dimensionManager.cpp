#include "dimensionManager.hpp"

Dimension::Dimension(const std::string& name) : name(name) {}
Dimension::~Dimension() {
    clear();
}

void Dimension::clear() {
    celestialBodies.clear();
    skybox.reset();
}
void Dimension::renderSkybox(std::shared_ptr<Camera> camera) const {
    if (!skybox) return;
    skybox->updateDayNightCycle(0.01f, sunlight);
    skybox->render(camera, sunlight);
}