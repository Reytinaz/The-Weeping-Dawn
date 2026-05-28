#ifndef TEXT_INSTANCE_HPP
#define TEXT_INSTANCE_HPP

#include "InstanceUI.hpp"

class Text : public InstanceUI {
public:
    sf::Text* text;
    sf::RectangleShape* shape;
    sf::Texture* texture;
    int alignmentX = 1;
    int alignmentY = 1;

    Text(const std::string& name, const sf::Font& font,
        const std::string& content = "", unsigned int size = 30,
        const sf::Color& textColor = sf::Color::Black, const sf::Color& backgroundColor = sf::Color::White);
    Text(const std::string& name, const std::string& texturePath, const sf::Font& font,
        const std::string& content = "", unsigned int size = 30,
        const sf::Color& textColor = sf::Color::Black, const sf::Color& backgroundColor = sf::Color::White);
    virtual ~Text();
    void render(sf::RenderWindow& window);
};

#endif