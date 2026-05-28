#include "Frame.hpp"

Frame::Frame(const std::string& name, const sf::Color& backgroundColor)
    : InstanceUI(name, "Frame") {
    shape = new sf::RectangleShape();
    shape->setFillColor(backgroundColor);
}

void Frame::render(sf::RenderWindow& window) {
    if (!visible || !shape) return;
    sf::Vector2f newSize = sf::Vector2f(size.x * window.getSize().x, size.x * window.getSize().y);
    sf::Vector2f newPosition = sf::Vector2f(position.x * window.getSize().x, position.x * window.getSize().y);
    shape->setSize(newSize);
    shape->setPosition(newPosition);
    window.draw(*shape);
}