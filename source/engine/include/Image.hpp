#ifndef IMAGE_INSTANCE_HPP
#define IMAGE_INSTANCE_HPP

#include "InstanceUI.hpp"

class Image : public InstanceUI {
public:
    sf::RectangleShape* shape;
    sf::Texture* texture;

    Image(const std::string& name, const std::string& texturePath, const sf::Color& backgroundColor = sf::Color::White);
    void render(sf::RenderWindow& window);
};

#endif