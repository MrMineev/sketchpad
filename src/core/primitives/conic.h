#pragma once

#include <SFML/Graphics.hpp>
#include <cmath>

class GConic {
public:
    // general conic: A x^2 + B x y + C y^2 + D x + E y + F = 0
    float A, B, C, D, E, F;
    
    // sampling range and resolution
    float x_min = -3000, x_max = 3000;
    int steps = 10000;

    sf::VertexArray branch1;   // y = y1(x), drawn as independent segments
    sf::VertexArray branch2;   // y = y2(x)

    GConic(float _A, float _B, float _C,
           float _D, float _E, float _F)
    : A(_A), B(_B), C(_C), D(_D), E(_E), F(_F),
      branch1(sf::Lines), branch2(sf::Lines)
    {
        buildShape();
    }

    void draw(sf::RenderWindow& window) {
        if (branch1.getVertexCount() > 1)
            window.draw(branch1);
        if (branch2.getVertexCount() > 1)
            window.draw(branch2);
    }

private:
    void buildShape() {
        branch1.clear();
        branch2.clear();

        float dx = (x_max - x_min) / float(steps);
        bool haveLast = false;
        float lastX = 0, lastY1 = 0, lastY2 = 0;

        for (int i = 0; i <= steps; ++i) {
            float x = x_min + i * dx;

            // solve C y^2 + (B x + E) y + (A x^2 + D x + F) = 0
            float a = C;
            float b = B*x + E;
            float c = A*x*x + D*x + F;
            float disc = b*b - 4*a*c;

            if (disc < 0 || std::abs(a) < 1e-6f) {
                haveLast = false;
                continue;
            }

            float sqrtD = std::sqrt(disc);
            float y1 = (-b + sqrtD) / (2*a);
            float y2 = (-b - sqrtD) / (2*a);

            if (haveLast) {
                // add a segment from (lastX,lastY) to (x,y)
                branch1.append({{lastX, lastY1}, sf::Color::Cyan});
                branch1.append({{   x,    y1}, sf::Color::Cyan});

                branch2.append({{lastX, lastY2}, sf::Color::Cyan});
                branch2.append({{   x,    y2}, sf::Color::Cyan});
            }

            lastX = x;
            lastY1 = y1;
            lastY2 = y2;
            haveLast = true;
        }
    }
};

/*
#pragma once

#include <SFML/Graphics.hpp>
#include <cmath>
#include <vector>

using namespace std;

class GConic {
public:
    // general conic: A x^2 + B x y + C y^2 + D x + E y + F = 0
    float A, B, C, D, E, F;
    
    // sampling range and resolution
    float x_min = -3000, x_max = 3000;
    int steps = 10000;

    int index = 0;
    sf::VertexArray branch1;   // y = y1(x)
    sf::VertexArray branch2;   // y = y2(x)

    GConic(float _A, float _B, float _C,
           float _D, float _E, float _F)
    : A(_A), B(_B), C(_C), D(_D), E(_E), F(_F),
      branch1(sf::LineStrip), branch2(sf::LineStrip)
    {
        buildShape();
        // If you really want to restyle after buifloating, use index loops:
        for (std::size_t i = 0; i < branch1.getVertexCount(); ++i)
            branch1[i].color = sf::Color::Cyan;
        for (std::size_t i = 0; i < branch2.getVertexCount(); ++i)
            branch2[i].color = sf::Color::Cyan;
    }

    void draw(sf::RenderWindow& window) {
        if (branch1.getVertexCount() > 1)
            window.draw(branch1);
        if (branch2.getVertexCount() > 1)
            window.draw(branch2);
    }

private:
    void buildShape() {
        branch1.clear();
        branch2.clear();

        float dx = (x_max - x_min) / float(steps);
        for (int i = 0; i <= steps; ++i) {
            float x = x_min + i * dx;

            // solve C y^2 + (B x + E) y + (A x^2 + D x + F) = 0
            float a = C;
            float b = B*x + E;
            float c = A*x*x + D*x + F;
            float disc = b*b - 4*a*c;

            if (disc < 0 || std::abs(a) < 1e-6f)
                continue; // no real roots or degenerate

            float sqrtD = std::sqrt(disc);
            float y1 = (-b + sqrtD) / (2*a);
            float y2 = (-b - sqrtD) / (2*a);

            // set color here to avoid a separate styling loop
            branch1.append({{x, y1}, sf::Color::Cyan});
            branch2.append({{x, y2}, sf::Color::Cyan});
        }
    }
};
*/
