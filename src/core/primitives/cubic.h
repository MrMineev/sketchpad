#pragma once

#include <SFML/Graphics.hpp>
#include <cmath>
#include <vector>
#include <array>
#include <algorithm>
#include <limits>

class GCubic {
public:
    // A x^3 + B x^2 y + C x y^2 + D y^3
    // + E x^2 + F x y + G y^2
    // + H x + I y + J = 0
    float A,B,C,D,E,F,G,H,I,J;

    // sampling range & resolution
    float x_min = -2000, x_max = 2000;
    int   steps = 200;

    // all zero‐crossing segments
    sf::VertexArray segments{sf::Lines};

    GCubic(float _A, float _B, float _C, float _D,
           float _E, float _F, float _G,
           float _H, float _I, float _J)
      : A(_A),B(_B),C(_C),D(_D),
        E(_E),F(_F),G(_G),
        H(_H),I(_I),J(_J)
    {
        buildShape();
    }

    void draw(sf::RenderWindow& window) {
        if (segments.getVertexCount() > 0)
            window.draw(segments);
    }

private:
    // implicit function
    inline float Fval(float x, float y) const {
        return  A*x*x*x
              + B*x*x*y
              + C*x*y*y
              + D*y*y*y
              + E*x*x
              + F*x*y
              + G*y*y
              + H*x
              + I*y
              + J;
    }

    // first pass: find min/max y over all real roots in [x_min..x_max]
    void estimateYRange(float &y0, float &y1) {
        y0 =  std::numeric_limits<float>::infinity();
        y1 = -std::numeric_limits<float>::infinity();

        auto solve = [&](float x){
            // same cubic‐in‐y solver as before:
            float a3 = D;
            float a2 = C*x + G;
            float a1 = B*x*x + F*x + I;
            float a0 = A*x*x*x + E*x*x + H*x + J;
            std::vector<float> r;
            if (std::fabs(a3) < 1e-8f) {
                if (std::fabs(a2) < 1e-8f) return r;
                float Dq = a1*a1 - 4*a2*a0;
                if (Dq >= 0) {
                    r.push_back((-a1 + std::sqrt(Dq)) / (2*a2));
                    r.push_back((-a1 - std::sqrt(Dq)) / (2*a2));
                }
                std::sort(r.begin(), r.end());
                return r;
            }
            float p = a2/a3, q = a1/a3, ro = a0/a3;
            float shift = p/3.f;
            float A2 = q - p*p/3.f;
            float A3 = 2*p*p*p/27.f - p*q/3.f + ro;
            float Q = A2/3.f, R = A3/2.f;
            float D = R*R + Q*Q*Q;
            if (D > 0) {
                float sD = std::sqrt(D);
                float S = std::cbrt(-R + sD), T = std::cbrt(-R - sD);
                r.push_back(S + T - shift);
            } else {
                float theta = std::acos(-R/std::sqrt(-Q*Q*Q));
                float mag = 2*std::sqrt(-Q);
                for (int k = 0; k < 3; ++k)
                    r.push_back(mag * std::cos((theta + 2*M_PI*k)/3.f) - shift);
                std::sort(r.begin(), r.end());
            }
            return r;
        };

        float dx = (x_max - x_min) / float(steps);
        for (int i = 0; i <= steps; ++i) {
            float x = x_min + i*dx;
            for (float y : solve(x)) {
                y0 = std::min(y0, y);
                y1 = std::max(y1, y);
            }
        }
        // add a margin so we capture the full contour
        float margin = 0.1f*(y1-y0 + 1e-3f);
        y0 -= margin;
        y1 += margin;
    }

    // linear interpolate between (x1,y1)->(x2,y2)
    static sf::Vector2f lerp(float x1,float y1,float f1,
                             float x2,float y2,float f2) {
        float t = f1/(f1 - f2);
        return { x1 + t*(x2-x1),  y1 + t*(y2-y1) };
    }

    void buildShape() {
        segments.clear();

        // get y‐range
        float y_min, y_max;
        estimateYRange(y_min, y_max);

        float dx = (x_max - x_min) / float(steps);
        float dy = (y_max - y_min) / float(steps);

        // marching‐squares lookup
        static const std::array<std::vector<std::pair<int,int>>,16> table = []{
            std::array<std::vector<std::pair<int,int>>,16> t{};
            t[1]  = {{0,3}};   t[2]  = {{0,1}};
            t[3]  = {{1,3}};   t[4]  = {{1,2}};
            t[5]  = {{0,1},{2,3}}; t[6]  = {{0,2}};
            t[7]  = {{2,3}};   t[8]  = {{2,3}};
            t[9]  = {{0,2}};   t[10] = {{0,3},{1,2}};
            t[11] = {{1,2}};   t[12] = {{1,3}};
            t[13] = {{0,1}};  t[14] = {{0,3}};
            return t;
        }();

        // scan each cell
        for (int iy=0; iy<steps; ++iy) {
            for (int ix=0; ix<steps; ++ix) {
                float xL = x_min + ix   * dx;
                float xR = x_min + (ix+1)* dx;
                float yB = y_min + iy   * dy;
                float yT = y_min + (iy+1)* dy;

                std::array<sf::Vector2f,4> P = {{
                    {xL,yB},{xR,yB},{xR,yT},{xL,yT}
                }};
                std::array<float,4> Fv;
                for (int c=0;c<4;++c)
                    Fv[c] = Fval(P[c].x, P[c].y);

                int mask = 0;
                for (int c=0;c<4;++c)
                    if (Fv[c] > 0) mask |= (1<<c);
                if (mask==0 || mask==15) continue;

                for (auto [e1,e2] : table[mask]) {
                    // edges 0:BL→BR,1:BR→TR,2:TR→TL,3:TL→BL
                    auto vert = [&](int e){
                        switch(e){
                          case 0: return lerp(P[0].x,P[0].y,Fv[0], P[1].x,P[1].y,Fv[1]);
                          case 1: return lerp(P[1].x,P[1].y,Fv[1], P[2].x,P[2].y,Fv[2]);
                          case 2: return lerp(P[2].x,P[2].y,Fv[2], P[3].x,P[3].y,Fv[3]);
                          case 3: return lerp(P[3].x,P[3].y,Fv[3], P[0].x,P[0].y,Fv[0]);
                        }
                        return sf::Vector2f{};
                    };
                    auto a = vert(e1), b = vert(e2);
                    segments.append({a, sf::Color::Cyan});
                    segments.append({b, sf::Color::Cyan});
                }
            }
        }
    }
};

/*
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

    // up to three real branches; drawn as independent segments
    array<sf::VertexArray,3> branches;

    GCubic(float _A, float _B, float _C, float _D,
           float _E, float _F, float _G,
           float _H, float _I, float _J)
      : A(_A),B(_B),C(_C),D(_D),
        E(_E),F(_F),G(_G),
        H(_H),I(_I),J(_J)
    {
        for (auto &b : branches)
            b = sf::VertexArray(sf::Lines);
        buildShape();
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
            // quadratic: b x^2 + c x + d = 0
            if (fabs(b) < 1e-8f) return r;
            float D = c*c - 4*b*d;
            if (D >= 0) {
                r.push_back((-c + sqrtf(D)) / (2*b));
                r.push_back((-c - sqrtf(D)) / (2*b));
            }
            sort(r.begin(), r.end());
            return r;
        }
        float p = b/a, q = c/a, r0 = d/a;
        float shift = p/3.0f;
        float A2 = q - p*p/3.0f;
        float A3 = 2*p*p*p/27.0f - p*q/3.0f + r0;
        float Q = A2/3.0f, R = A3/2.0f;
        float D = R*R + Q*Q*Q;
        if (D > 0) {
            float sqrtD = sqrtf(D);
            float S = cbrtf(-R + sqrtD);
            float T = cbrtf(-R - sqrtD);
            r.push_back(S + T - shift);
        } else {
            float theta = acosf(-R / sqrtf(-Q*Q*Q));
            float mag = 2 * sqrtf(-Q);
            for (int k = 0; k < 3; ++k) {
                float angle = (theta + 2*M_PI*k) / 3.0f;
                r.push_back(mag * cosf(angle) - shift);
            }
            sort(r.begin(), r.end());
        }
        return r;
    }

    void buildShape() {
        // Maximum allowed jump in y between samples; larger jumps are discontinuities
        float dx = (x_max - x_min) / float(steps);
        float maxDY = dx * 5.0f;  // adjust factor to control sensitivity

        for (auto &b : branches)
            b.clear();

        // last x and y for each branch
        array<bool,3> haveLast = {false,false,false};
        array<float,3> lastX, lastY;

        for (int i = 0; i <= steps; ++i) {
            float x = x_min + i * dx;
            // cubic in y: D y^3 + (C*x+G) y^2 + (B*x^2+F*x+I) y + (A*x^3+E*x^2+H*x+J)
            float a3 = D;
            float a2 = C*x + G;
            float a1 = B*x*x + F*x + I;
            float a0 = A*x*x*x + E*x*x + H*x + J;

            auto roots = solveCubic(a3,a2,a1,a0);
            // match current roots to previous branches by nearest y
            array<int,3> assign = {-1,-1,-1};
            vector<bool> used(roots.size(), false);
            for (int k = 0; k < 3; ++k) {
                if (!haveLast[k]) continue;
                float bestDist = 1e9;
                int bestIdx = -1;
                for (int j = 0; j < (int)roots.size(); ++j) {
                    if (used[j]) continue;
                    float dY = roots[j] - lastY[k];
                    if (fabs(dY) < bestDist) {
                        bestDist = fabs(dY);
                        bestIdx = j;
                    }
                }
                if (bestIdx >= 0) {
                    assign[k] = bestIdx;
                    used[bestIdx] = true;
                }
            }
            // assign any unused roots to empty branches
            for (int j = 0; j < (int)roots.size(); ++j) {
                if (used[j]) continue;
                for (int k = 0; k < 3; ++k) {
                    if (assign[k] < 0) {
                        assign[k] = j;
                        used[j] = true;
                        break;
                    }
                }
            }
            array<bool,3> nextHave = {false,false,false};
            array<float,3> nextY;

            // draw segments
            for (int k = 0; k < 3; ++k) {
                int idx = assign[k];
                if (idx < 0) continue;
                float y = roots[idx];
                if (haveLast[k]) {
                    // only draw if vertical jump is within threshold
                    if (fabs(y - lastY[k]) <= maxDY) {
                        branches[k].append({{lastX[k], lastY[k]}, sf::Color::Cyan});
                        branches[k].append({{       x,            y}, sf::Color::Cyan});
                    }
                }
                nextHave[k] = true;
                nextY[k] = y;
            }

            // update last arrays
            for (int k = 0; k < 3; ++k) {
                haveLast[k] = nextHave[k];
                if (haveLast[k]) {
                    lastX[k] = x;
                    lastY[k] = nextY[k];
                }
            }
        }
    }
};
*/

/*
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
*/
