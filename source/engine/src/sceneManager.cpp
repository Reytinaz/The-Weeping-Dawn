#include "sceneManager.hpp"

std::vector<std::shared_ptr<Object3D>> SceneManager::collect3DObjects() const {
    std::vector<std::shared_ptr<Object3D>> result;

    std::function<void(std::shared_ptr<Instance>)> traverse = [&](std::shared_ptr<Instance> node) {
        if (auto objInst = std::dynamic_pointer_cast<Object3D>(node)) {
            if (objInst->initialized && objInst->isVisible()) {
                result.push_back(objInst);
            }
        }
        for (auto& child : node->children) {
            traverse(child);
        }
    };

    traverse(workspace);
    return result;
}

std::vector<std::shared_ptr<SourceLight>> SceneManager::collectLights() const {
    std::vector<std::shared_ptr<SourceLight>> result;

    std::function<void(std::shared_ptr<Instance>)> traverse = [&](std::shared_ptr<Instance> node) {
        if (auto objInst = std::dynamic_pointer_cast<SourceLight>(node)) {
            if (objInst->enabled) {
                result.push_back(objInst);
            }
        }
        for (auto& child : node->children) {
            traverse(child);
        }
        };

    traverse(workspace);
    return result;
}

void SceneManager::renderInterface(sf::RenderWindow& window) const {
    std::vector<std::shared_ptr<Instance>> visibleElements;

    std::function<void(std::shared_ptr<Instance>)> traverse = [&](std::shared_ptr<Instance> node) {
        if (!node->visible) return;
        visibleElements.push_back(node);
        for (auto& child : node->children) {
            traverse(child);
        }
    };
    traverse(interface);

    std::sort(visibleElements.begin(), visibleElements.end(),
        [](std::shared_ptr<Instance> a, std::shared_ptr<Instance> b) {
            if (std::dynamic_pointer_cast<InstanceUI>(a) && std::dynamic_pointer_cast<InstanceUI>(b)) {
                return std::dynamic_pointer_cast<InstanceUI>(a)->zIndex < std::dynamic_pointer_cast<InstanceUI>(b)->zIndex;
        }
    });


    for (auto& node : visibleElements) {
        if (auto obj = std::dynamic_pointer_cast<Text>(node)) {
            obj->render(window);
        }
        else if (auto obj = std::dynamic_pointer_cast<Button>(node)) {
            obj->render(window);
        }
        else if (auto obj = std::dynamic_pointer_cast<Frame>(node)) {
            obj->render(window);
        }
        else if (auto obj = std::dynamic_pointer_cast<Image>(node)) {
            obj->render(window);
        }
    }
}