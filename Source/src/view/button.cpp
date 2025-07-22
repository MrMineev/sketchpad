#include "button.h"

Button::Button(const std::string& text, const sf::Vector2f& position, const sf::Vector2f& size, const sf::Font& font)
    : clicked(false), normalColor(sf::Color::White), hoverColor(sf::Color::Green), pressedColor(sf::Color::Red) {

    shape.setSize(size);
    shape.setPosition(position);
    shape.setFillColor(normalColor);

    buttonText.setString(text);
    buttonText.setFont(font);
    buttonText.setCharacterSize(static_cast<unsigned int>(size.y * 0.6)); // adjust text size to fit button
    buttonText.setFillColor(sf::Color::Black);
    buttonText.setPosition(
        position.x + (size.x - buttonText.getLocalBounds().width) / 2,
        position.y + (size.y - buttonText.getLocalBounds().height) / 2 - 10
    );
}

void Button::setColors(const sf::Color& normalColor, const sf::Color& hoverColor, const sf::Color& pressedColor) {
    this->normalColor = normalColor;
    this->hoverColor = hoverColor;
    this->pressedColor = pressedColor;
}

void Button::setTextColor(const sf::Color& color) {
    buttonText.setFillColor(color);
}

bool Button::isHovered(const sf::RenderWindow& window) const {
    auto mousePos = sf::Mouse::getPosition(window);
    return shape.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos));
}

void Button::handleEvent(const sf::Event& event, const sf::RenderWindow& window) {
    if (isHovered(window)) {
        shape.setFillColor(hoverColor);

        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            shape.setFillColor(pressedColor);
            clicked = true;
        } else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
            shape.setFillColor(hoverColor);
        }
    } else {
        shape.setFillColor(normalColor);
        clicked = false;
    }
}

void Button::draw(sf::RenderWindow& window) const {
    window.draw(shape);
    window.draw(buttonText);
}

bool Button::isClicked() const {
    return clicked;
}

