#ifndef FRAME_INSTANCE_HPP
#define FRAME_INSTANCE_HPP

#include "InstanceUI.hpp"

class Frame : public InstanceUI {
public:
    sf::RectangleShape* shape;

    Frame(const std::string& name, const sf::Color& backgroundColor = sf::Color::White);
    void render(sf::RenderWindow& window);
};

#endif