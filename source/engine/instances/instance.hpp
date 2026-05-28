#ifndef INSTANCE_HPP
#define INSTANCE_HPP

#include "vector3.hpp"
#include "SFML/Window.hpp"
#include "SFML/Graphics.hpp"
#include "memory"
#include "functional"
#include "algorithm"

class Renderer;
class Engine;

class Instance : public std::enable_shared_from_this<Instance> {
private:
    std::vector<std::string> tags;
public:
    std::string name;
    std::string className;
    std::weak_ptr<Instance> parent;
    std::vector<std::shared_ptr<Instance>> children;
    bool visible = true;

    Instance(const std::string& name = "Instance", const std::string& className = "Instance");
    virtual ~Instance() = default;

    template<typename T, typename... Args>
    std::shared_ptr<T> addChild(Args&&... args) {
        static_assert(std::is_base_of<Instance, T>::value, "T must be derived from Instance");
        auto child = std::make_shared<T>(std::forward<Args>(args)...);
        child->parent = shared_from_this();
        children.push_back(child);
        return child;
    }
    bool isVisible() const {
        if (!visible) return false;
        auto p = parent.lock();
        if (p) return p->isVisible();
        return true;
    }

    std::shared_ptr<Instance> findChild(const std::string& name);
    std::vector<std::shared_ptr<Instance>> findAll();
    std::vector<std::shared_ptr<Instance>> findAll(const std::string& name);
    std::vector<std::shared_ptr<Instance>> findAllByTag(const std::string& tag) const;
    void removeAll();
    void removeChild(std::shared_ptr<Instance> child);
    void addTag(const std::string& t);
    void removeTag(const std::string& t);
    bool hasTag(const std::string& t) const;
    const std::vector<std::string>& getTags() const;
};

#endif