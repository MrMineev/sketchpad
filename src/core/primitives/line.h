#include <SFML/Graphics.hpp>

class Line {
public:
    sf::VertexArray line;

    Line(float x1, float y1, float x2, float y2) {
        line.setPrimitiveType(sf::Lines);
        line.append(sf::Vertex(sf::Vector2f(x1, y1), sf::Color::Blue));
        line.append(sf::Vertex(sf::Vector2f(x2, y2), sf::Color::Blue));
    }

    void draw(sf::RenderWindow& window) {
        window.draw(line);
    }
};

