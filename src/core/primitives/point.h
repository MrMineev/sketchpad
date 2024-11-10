#include <SFML/Graphics.hpp>

class GPoint {
public:
  float x_pos, y_pos;
  sf::CircleShape shape;

  int index;

  GPoint(float x, float y) {
    x_pos = x;
    y_pos = y;
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

