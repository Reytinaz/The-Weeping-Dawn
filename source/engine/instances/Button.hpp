#ifndef BUTTON_HPP
#define BUTTON_HPP

#include "InstanceUI.hpp"

class Button : public InstanceUI {
public:
    sf::RectangleShape* shape;
    sf::Text* text;
    sf::Texture* texture;
    sf::Color hoverColor;
    sf::Color normalColor;
    sf::Color pressedColor;
    std::function<void(Engine&, std::shared_ptr<Button>&)> onClick;
    int alignmentX = 1;
    int alignmentY = 1;

    Button(const std::string& name, const sf::Font& font,
        const std::string& label = "", unsigned int size = 24,
        const sf::Color& textColor = sf::Color::White, const sf::Color& backgroundColor = sf::Color::Black);
    Button(const std::string& name, const std::string& texturePath, const sf::Font& font,
        const std::string& label = "", unsigned int size = 24,
        const sf::Color& textColor = sf::Color::White, const sf::Color& backgroundColor = sf::Color::Black);
    virtual ~Button();
    bool contains(const sf::Vector2f& point, sf::RenderWindow& window) const;
    void render(sf::RenderWindow& window);
};

#endif