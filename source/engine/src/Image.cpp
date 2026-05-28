#include "Image.hpp"

Image::Image(const std::string& name, const std::string& texturePath, const sf::Color& backgroundColor)
    : InstanceUI(name, "Image") {
    shape = new sf::RectangleShape();
    shape->setFillColor(backgroundColor);

    texture = new sf::Texture();
    if (texture->loadFromFile(texturePath)) {
        shape->setTexture(texture, true);
    }
}

void Image::render(sf::RenderWindow& window) {
    if (!visible || !shape) return;
    sf::Vector2f newSize = sf::Vector2f(size.x * window.getSize().x, size.x * window.getSize().y);
    sf::Vector2f newPosition = sf::Vector2f(position.x * window.getSize().x, position.x * window.getSize().y);
    shape->setSize(newSize);
    shape->setPosition(newPosition);
    window.draw(*shape);
}
