#include "Text.hpp"

Text::Text(const std::string& name, const sf::Font& font,
    const std::string& content, unsigned int size, const sf::Color& textColor, const sf::Color& backgroundColor)
    : InstanceUI(name, "Text") {
    shape = new sf::RectangleShape();
    text = new sf::Text(font, content, size);
    text->setFillColor(textColor);
    shape->setFillColor(backgroundColor);
}

Text::Text(const std::string& name, const std::string& texturePath, const sf::Font& font,
    const std::string& content, unsigned int size, const sf::Color& textColor, const sf::Color& backgroundColor)
    : InstanceUI(name, "Text") {
    shape = new sf::RectangleShape();
    text = new sf::Text(font, content, size);
    text->setFillColor(textColor);
    shape->setFillColor(backgroundColor);

    texture = new sf::Texture();
    if (texture->loadFromFile(texturePath)) {
        shape->setTexture(texture, true);
    }
}

Text::~Text() {
    delete shape;
    delete text;
}

void Text::render(sf::RenderWindow& window) {
    if (!visible || !shape || !text) return;

    sf::Vector2f windowSize = static_cast<sf::Vector2f>(window.getSize());
    sf::Vector2f absSize = sf::Vector2f(size.x * windowSize.x, size.y * windowSize.y);
    sf::Vector2f absPos = sf::Vector2f(position.x * windowSize.x, position.y * windowSize.y);
    sf::FloatRect textBounds = text->getLocalBounds();

    shape->setSize(absSize);
    shape->setPosition(absPos);

    float padding = text->getCharacterSize() / 5.0f;

    float textX = absPos.x;
    if (alignmentX == 1) {
        textX = absPos.x + padding;
    }
    else if (alignmentX == 2) {
        textX = absPos.x + absSize.x - textBounds.size.x - padding;
    }
    else if (alignmentX == 3) {
        textX = absPos.x + (absSize.x - textBounds.size.x) / 2.0f;
    }

    float textY = absPos.y;
    if (alignmentY == 1) {
        textY = absPos.y + padding;
    }
    else if (alignmentY == 2) {
        textY = absPos.y + absSize.y - textBounds.size.y - padding;
    }
    else if (alignmentY == 3) {
        textY = absPos.y + (absSize.y - textBounds.size.y) / 2.0f - padding;
    }

    text->setPosition(sf::Vector2f(textX, textY));

    window.draw(*shape);
    window.draw(*text);
}