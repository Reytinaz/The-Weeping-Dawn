#include "instance.hpp"

Instance::Instance(const std::string& name, const std::string& className)
    : name(name), className(className) {}

void Instance::removeChild(std::shared_ptr<Instance> child) {
    auto it = std::find(children.begin(), children.end(), child);
    if (it != children.end()) {
        (*it)->parent.reset();
        children.erase(it);
    }
}

std::shared_ptr<Instance> Instance::findChild(const std::string& name) {
    for (auto& child : children) {
        if (child->name == name) return child;
        if (auto found = child->findChild(name)) return found;
    }
    return nullptr;
}

std::vector<std::shared_ptr<Instance>> Instance::findAll() {
    std::vector<std::shared_ptr<Instance>> result;
    for (auto& child : children) {
        result.push_back(child);
    }
    return result;
}
std::vector<std::shared_ptr<Instance>> Instance::findAll(const std::string& name) {
    std::vector<std::shared_ptr<Instance>> result;
    for (auto& child : children) {
        if (child->name == name) result.push_back(child);
    }
    return result;
}

std::vector<std::shared_ptr<Instance>> Instance::findAllByTag(const std::string& tag) const {
    std::vector<std::shared_ptr<Instance>> result;
    for (auto& child : children) {
        if (child->hasTag(tag)) result.push_back(child);
    }
    return result;
}

void Instance::removeAll() {
    children.clear();
}

void Instance::addTag(const std::string& t) { 
    tags.push_back(t); 
}
void Instance::removeTag(const std::string& t) {
    tags.erase(std::remove(tags.begin(), tags.end(), t), tags.end());
}
bool Instance::hasTag(const std::string& t) const {
    return std::find(tags.begin(), tags.end(), t) != tags.end();
}
const std::vector<std::string>& Instance::getTags() const { 
    return tags; 
}