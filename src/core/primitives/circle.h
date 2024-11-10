#include <SFML/Graphics.hpp>

class GCircle {
public:
  float x_pos, y_pos, radius;

  sf::CircleShape shape;

  GCircle(float x, float y, float _radius) {
    x_pos = x;
    y_pos = y;
    radius = _radius;
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
