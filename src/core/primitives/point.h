#include <SFML/Graphics.hpp>

class Point {
public:
    sf::CircleShape shape;

    Point(float x, float y) {
        shape.setRadius(5);
        shape.setFillColor(sf::Color::Red);
        shape.setPosition(x - shape.getRadius(), y - shape.getRadius());
    }

    void setPosition(float x, float y) {
        shape.setPosition(x - shape.getRadius(), y - shape.getRadius());
    }

    void draw(sf::RenderWindow& window) {
        window.draw(shape);
    }

    sf::Vector2f getPosition() const {
        return shape.getPosition();
    }
};

