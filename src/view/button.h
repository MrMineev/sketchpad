#pragma once

#include <SFML/Graphics.hpp>
#include <string>

class Button {
public:
    Button(const std::string& text, const sf::Vector2f& position, const sf::Vector2f& size, const sf::Font& font);
    
    void setColors(const sf::Color& normalColor, const sf::Color& hoverColor, const sf::Color& pressedColor);
    void setTextColor(const sf::Color& color);
    
    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void draw(sf::RenderWindow& window) const;
    bool isClicked() const;

private:
    sf::RectangleShape shape;
    sf::Text buttonText;

    sf::Color normalColor;
    sf::Color hoverColor;
    sf::Color pressedColor;

    bool clicked;
    bool isHovered(const sf::RenderWindow& window) const;
};

