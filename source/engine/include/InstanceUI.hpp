#ifndef UI_INSTANCE_HPP
#define UI_INSTANCE_HPP

#include "Instance.hpp"

class InstanceUI : public Instance, public std::enable_shared_from_this<InstanceUI> {
public:
    sf::Vector2f position;
    sf::Vector2f size;
    int zIndex = 1;

    InstanceUI(const std::string& name, const std::string& className) : Instance(name, className) {};
};

#endif