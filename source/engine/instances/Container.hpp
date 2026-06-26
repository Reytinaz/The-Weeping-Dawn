#ifndef CONTAINER_HPP
#define CONTAINER_HPP

#include "instance.hpp"

class Container : public Instance {
public:
    Container(const std::string& name = "Container", const std::string& className = "Container");
};

#endif