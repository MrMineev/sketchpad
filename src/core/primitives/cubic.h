#pragma once

#include <SFML/Graphics.hpp>
#include <cmath>
#include <vector>
#include <array>
#include <algorithm>

using namespace std;

class GCubic {
public:
    // general cubic:
    // A x^3 + B x^2 y + C x y^2 + D y^3
    // + E x^2 + F x y + G y^2
    // + H x + I y + J = 0
    float A,B,C,D,E,F,G,H,I,J;

    // sampling range and resolution
    float x_min = -1000, x_max = 1000;
    int   steps = 5000;

    // up to three real branches
    array<sf::VertexArray,3> branches;

    GCubic(float _A, float _B, float _C, float _D,
           float _E, float _F, float _G,
           float _H, float _I, float _J)
      : A(_A),B(_B),C(_C),D(_D),
        E(_E),F(_F),G(_G),
        H(_H),I(_I),J(_J)
    {
        for (auto &b : branches)
            b = sf::VertexArray(sf::LineStrip);
        buildShape();
        // color all branches cyan
        for (auto &b : branches)
            for (std::size_t i = 0; i < b.getVertexCount(); ++i)
                b[i].color = sf::Color::Cyan;
    }

    void draw(sf::RenderWindow& window) {
        for (auto &b : branches)
            if (b.getVertexCount() > 1)
                window.draw(b);
    }

private:
    // Solve a*x^3 + b*x^2 + c*x + d = 0 for real roots
    static vector<float> solveCubic(float a, float b, float c, float d) {
        vector<float> r;
        if (fabs(a) < 1e-8f) {
            // degenerate to quadratic
            float D = c*c - 4*b*d;
            if (D >= 0) {
                r.push_back((-c + sqrtf(D)) / (2*b));
                r.push_back((-c - sqrtf(D)) / (2*b));
            }
            return r;
        }
        // normalize to t^3 + p t^2 + q t + r = 0
        float p = b/a, q = c/a, r0 = d/a;
        // depressed: t = y - p/3 => y^3 + A2*y + A3 = 0
        float shift = p/3.0f;
        float A2 = q - p*p/3.0f;
        float A3 = 2*p*p*p/27.0f - p*q/3.0f + r0;

        float Q = A2/3.0f, R = A3/2.0f;
        float D = R*R + Q*Q*Q;  // discriminant

        if (D > 0) {
            // one real root
            float sqrtD = sqrtf(D);
            float S = cbrtf(-R + sqrtD);
            float T = cbrtf(-R - sqrtD);
            r.push_back(S + T - shift);
        } else {
            // three real roots
            float theta = acosf(-R / sqrtf(-Q*Q*Q));
            float mag = 2 * sqrtf(-Q);
            for (int k = 0; k < 3; ++k) {
                float angle = (theta + 2*M_PI*k) / 3.0f;
                r.push_back(mag * cosf(angle) - shift);
            }
            // sort for consistency
            sort(r.begin(), r.end());
        }
        return r;
    }

    void buildShape() {
        for (auto &b : branches)
            b.clear();

        float dx = (x_max - x_min) / float(steps);
        for (int i = 0; i <= steps; ++i) {
            float x = x_min + i * dx;

            // coefficients of the cubic in y:
            //   D·y^3
            // + (C*x + G)·y^2
            // + (B*x^2 + F*x + I)·y
            // + (A*x^3 + E*x^2 + H*x + J) = 0
            float a3 = D;
            float a2 = C*x + G;
            float a1 = B*x*x + F*x + I;
            float a0 = A*x*x*x + E*x*x + H*x + J;

            auto roots = solveCubic(a3, a2, a1, a0);
            for (std::size_t k = 0; k < roots.size() && k < 3; ++k) {
                float y = roots[k];
                branches[k].append({{x,y}, sf::Color::Cyan});
            }
        }
    }
};

