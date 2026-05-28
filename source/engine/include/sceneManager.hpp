#ifndef SCENE_MANAGER_HPP
#define SCENE_MANAGER_HPP

#include "InstanceUI.hpp"
#include "Container.hpp"
#include "Button.hpp"
#include "Text.hpp"
#include "Frame.hpp"
#include "Object3D.hpp"
#include "SourceLight.hpp"
#include "Character.hpp"
#include "Image.hpp"

class SceneManager {
public:
    std::shared_ptr<Container> root;
    std::shared_ptr<Container> interface;
    std::shared_ptr<Container> workspace;

    std::vector<std::shared_ptr<Object3D>> collect3DObjects() const;
    std::vector<std::shared_ptr<SourceLight>> collectLights() const;
    void renderInterface(sf::RenderWindow& window) const;

    SceneManager() {
        root = std::make_shared<Container>("game", "Container");
        interface = std::make_shared<Container>("interface", "Container");
        workspace = std::make_shared<Container>("workspace", "Container");
    }
};

#endif