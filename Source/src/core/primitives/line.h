#include <SFML/Graphics.hpp>

#include <SFML/Graphics.hpp>
#include <cmath>
#include <iostream>

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
  int line_type = 0;
  float x1, y1, x2, y2;
  sf::VertexArray line;

  GLine(float _x1, float _y1, float _x2, float _y2, int _is_segment)
    : x1(_x1), y1(_y1), x2(_x2), y2(_y2), line_type(_is_segment) {
    if (line_type == 0) {
      line.setPrimitiveType(sf::Lines);
      line.append(sf::Vertex(sf::Vector2f(x1, y1), sf::Color::Blue));
      line.append(sf::Vertex(sf::Vector2f(x2, y2), sf::Color::Blue));
    } else if (line_type == 1) {
      auto [new_x1, new_y1] = inter_line(x1, y1, x2, y2, 5); 
      auto [new_x2, new_y2] = inter_line(x1, y1, x2, y2, 5000);

      line.setPrimitiveType(sf::Lines);
      line.append(sf::Vertex(sf::Vector2f(new_x1, new_y1), sf::Color::Blue));
      line.append(sf::Vertex(sf::Vector2f(new_x2, new_y2), sf::Color::Blue));
    } else if (line_type == 2) {
      auto [new_x1, new_y1] = inter_line(x1, y1, x2, y2, 5); 
      auto [new_x2, new_y2] = inter_line(x1, y1, x2, y2, 5000);

      // HERE
      sf::Vector2f p1(new_x1, new_y1);
      sf::Vector2f p2(new_x2, new_y2);
      sf::Vector2f dir = p2 - p1;

      float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
      dir /= length; // normalize

      float dashLength = 10.0f;
      float gapLength = 5.0f;
      float total = 0;

      line.setPrimitiveType(sf::Lines);

      while (total + dashLength < length) {
        sf::Vector2f start = p1 + dir * total;
        sf::Vector2f end = p1 + dir * (total + dashLength);

        line.append(sf::Vertex(start, sf::Color(238, 130, 238))); // violet
        line.append(sf::Vertex(end, sf::Color(238, 130, 238)));

        total += dashLength + gapLength;
      }
    }
  }

  void draw(sf::RenderWindow& window) {
    window.draw(line);
  }
};
