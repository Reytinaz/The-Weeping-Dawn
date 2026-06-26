#include "dimensionManager.hpp"

Dimension::Dimension(const std::string& name) : name(name) {}
Dimension::~Dimension() {
    celestialBodies.clear();
    skybox.reset();
}


void Dimension::renderSkybox(std::shared_ptr<Camera> camera) const {
    if (!skybox) return;
    skybox->updateDayNightCycle(0.01f);
    skybox->updateClouds(0.01f);
    skybox->render(camera, celestialBodies);
}