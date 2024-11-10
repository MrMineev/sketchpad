#include <SFML/Graphics.hpp>

#include <SFML/Graphics.hpp>
#include <cmath>

class GLine {
 private:
  std::pair<float, float> inter_line(float x1, float y1, float x2, float y2, float x_intersect) {
    float m = (y2 - y1) / (x2 - x1);
    float b = y1 - m * x1;
    float y_intersect = m * x_intersect + b;
    return {x_intersect, y_intersect};
  }

 public:
  int index = 0;
  bool is_segment = false;
  float x1, y1, x2, y2;
  sf::VertexArray line;

  GLine(float _x1, float _y1, float _x2, float _y2, bool _is_segment = true)
    : x1(_x1), y1(_y1), x2(_x2), y2(_y2), is_segment(_is_segment) {
    if (is_segment == true) {
      line.setPrimitiveType(sf::Lines);
      line.append(sf::Vertex(sf::Vector2f(x1, y1), sf::Color::Blue));
      line.append(sf::Vertex(sf::Vector2f(x2, y2), sf::Color::Blue));
    } else {
      auto [new_x1, new_y1] = inter_line(x1, y1, x2, y2, 5); 
      auto [new_x2, new_y2] = inter_line(x1, y1, x2, y2, 5000);

      line.setPrimitiveType(sf::Lines);
      line.append(sf::Vertex(sf::Vector2f(new_x1, new_y1), sf::Color::Blue));
      line.append(sf::Vertex(sf::Vector2f(new_x2, new_y2), sf::Color::Blue));
    }
  }

  void draw(sf::RenderWindow& window) {
    window.draw(line);
  }
};
