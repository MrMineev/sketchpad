#include <SFML/Graphics.hpp>

class Circle {
public:
    sf::CircleShape shape;

    Circle(float x, float y, float radius) {
        shape.setRadius(radius);
        shape.setFillColor(sf::Color::Transparent);
        shape.setOutlineColor(sf::Color::Green);
        shape.setOutlineThickness(1);
        shape.setPosition(x - radius, y - radius);
    }

    void draw(sf::RenderWindow& window) {
        window.draw(shape);
    }
};
