#pragma once

#include <iostream>
#include <vector>
#include <cmath>
#include <complex>

using namespace std;

typedef long double ld;
typedef complex<ld> cd;

#define det(a, b, c, d) ((a) * (d) - (b) * (c))

namespace AlgGeom {

const ld EPS = 1e-4;

struct Point {
  ld x, y;

  Point(ld _x, ld _y) : x(_x), y(_y) {}
  Point() {}
};

struct Line {
  ld a, b, c; // ax + by + c = 0

  Line(Point p1, Point p2) {
    // | x    y    1|
    // | p1.x p1.y 1|
    // | p2.x p2.y 1|
    a = det(p1.y, 1, p2.y, 1);
    b = -det(p1.x, 1, p2.x, 1);
    c = det(p1.x, p1.y, p2.x, p2.y);
  }

  pair<pair<ld, ld>, pair<ld, ld>> gen_points() {
    return make_pair(
      make_pair(0, -c / b),
      make_pair(-c / a, 0)
    );
  }

  Line(ld _a, ld _b, ld _c) : a(_a), b(_b), c(_c) {}
  Line() {}
};

struct Circle {
  Point p;
  ld radius;

  Circle(Point _p, ld _r) : p(_p), radius(_r) {}
  Circle() {}
};

struct Conic {
  ld a, b, c, d, e, f;

  Conic(ld _a, ld _b, ld _c, ld _d, ld _e, ld _f) : a(_a), b(_b), c(_c), d(_d), e(_e), f(_f) {}
  Conic() {}
};

struct Cubic {
  ld a, b, c, d, e, f, g,h,i,j;

  Cubic(ld _a, ld _b, ld _c, ld _d, ld _e, ld _f, ld _g, ld _h, ld _i, ld _j) : a(_a), b(_b), c(_c), d(_d), e(_e), f(_f), g(_g), h(_h), i(_i), j(_j) {}
  Cubic() {}
};

class CoreGeometryTools {
 public:
  static bool points_are_equal(Point p1, Point p2) {
    return dist_points(p1, p2) < EPS;
  }

  // midpoint
  static Point midpoint(Point p1, Point p2) {
    return Point((p1.x + p2.x) / 2, (p1.y + p2.y) / 2);
  }

  // equation for a line through P perpendicular to L
  static Line perp_normal(Point p, Line l) {
    ld a = l.b;
    ld b = -l.a;
    ld c = - a * p.x - b * p.y;
    return Line{a, b, c};
  }

  // equation for a line through P parallel to L
  static Line parallel_line(Point p, Line l) {
    ld a = l.a;
    ld b = l.b;
    ld c = -a * p.x - b * p.y;
    return Line{a, b, c};
  }

  // equation of perp bisector

  static Line perp_bisector(Point p1, Point p2) {
    return AlgGeom::CoreGeometryTools::perp_normal(
      AlgGeom::CoreGeometryTools::midpoint(p1, p2),
      AlgGeom::Line(p1, p2)
    );
  }

  // conic through five points
  static Conic fitConicThrough5(Point p1, Point p2, Point p3, Point p4, Point p5) {
    const Point pts[5] = {p1,p2,p3,p4,p5};

    // 1) compute column‐strengths
    long double strength[6] = {0};
    for (int i = 0; i < 5; ++i) {
        ld x = pts[i].x, y = pts[i].y;
        long double col[6] = { x*x, x*y, y*y, x, y, 1.0L };
        for (int c = 0; c < 6; ++c)
            strength[c] += std::fabsl(col[c]);
    }
    // pick the index with max strength
    int fixCol = std::max_element(strength, strength+6) - strength;

    // 2) build a 5×6 augmented matrix that omits the fixed‐col from the unknowns,
    //    and puts it on the RHS multiplied by –1.
    //    unknownIndices[] maps 0..4 → the five free columns
    int unknownIndices[5], idx = 0;
    for (int c = 0; c < 6; ++c) {
        if (c == fixCol) continue;
        unknownIndices[idx++] = c;
    }

    ld M[5][6];   // 5 rows, (5 unknowns + 1 RHS)
    for (int i = 0; i < 5; ++i) {
        ld x = pts[i].x, y = pts[i].y;
        ld cols[6] = { x*x, x*y, y*y, x, y, 1.0L };
        for (int j = 0; j < 5; ++j) {
            M[i][j] = cols[ unknownIndices[j] ];
        }
        // RHS = –(fixedColTerm)
        M[i][5] = -cols[ fixCol ];
    }

    // 3) Gauss–Jordan on the 5×6 matrix, exactly as before
    const ld EPS = std::numeric_limits<ld>::epsilon() * 1e3L;
    int n = 5, m = 5;
    for (int col = 0, row = 0; col < m && row < n; ++col, ++row) {
        int sel = row;
        for (int i = row+1; i < n; ++i)
            if (std::fabsl(M[i][col]) > std::fabsl(M[sel][col]))
                sel = i;
        if (std::fabsl(M[sel][col]) < EPS)
            throw std::runtime_error("Singular configuration: cannot fit conic.");

        for (int j = col; j <= m; ++j)
            std::swap(M[sel][j], M[row][j]);
        ld inv = 1.0L / M[row][col];
        for (int j = col; j <= m; ++j)
            M[row][j] *= inv;
        for (int i = 0; i < n; ++i) {
            if (i == row) continue;
            ld fct = M[i][col];
            for (int j = col; j <= m; ++j)
                M[i][j] -= fct * M[row][j];
        }
    }

    // 4) gather the full [a,b,c,d,e,f] back in order:
    ld sol[6];
    sol[fixCol] = 1.0L;           // the one we fixed
    for (int j = 0; j < 5; ++j) {
        sol[ unknownIndices[j] ] = M[j][5];
    }

    return Conic(sol[0], sol[1], sol[2], sol[3], sol[4], sol[5]);
  }

  /*
  static Conic fitConicThrough5(Point p1, Point p2, Point p3, Point p4, Point p5) {
    // Build augmented 5×6 matrix for unknowns [a,b,c,d,e] with f fixed = 1.
    ld M[5][6];
    const Point pts[5] = {p1, p2, p3, p4, p5};
    for (int i = 0; i < 5; ++i) {
        ld x = pts[i].x, y = pts[i].y;
        M[i][0] = x*x;      // a·x^2
        M[i][1] = x*y;      // b·x·y
        M[i][2] = y*y;      // c·y^2
        M[i][3] = x;        // d·x
        M[i][4] = y;        // e·y
        M[i][5] = -1.0;    // RHS = –f (we set f=1)
    }

    const ld EPS = std::numeric_limits<ld>::epsilon() * 1e3L;
    int n = 5, m = 5;

    // Gauss–Jordan elimination with partial pivoting
    for (int col = 0, row = 0; col < m && row < n; ++col, ++row) {
        // 1) find pivot in col ≥ row
        int sel = row;
        for (int i = row + 1; i < n; ++i) {
            if (std::fabsl(M[i][col]) > std::fabsl(M[sel][col]))
                sel = i;
        }
        if (std::fabsl(M[sel][col]) < EPS) {
            throw std::runtime_error("Singular configuration: cannot fit conic.");
        }
        // 2) swap pivot row into place
        for (int j = col; j <= m; ++j)
            std::swap(M[sel][j], M[row][j]);
        // 3) normalize pivot row
        ld inv = 1.0 / M[row][col];
        for (int j = col; j <= m; ++j)
            M[row][j] *= inv;
        // 4) eliminate all other rows
        for (int i = 0; i < n; ++i) {
            if (i == row) continue;
            ld factor = M[i][col];
            for (int j = col; j <= m; ++j)
                M[i][j] -= factor * M[row][j];
        }
    }

    cout << "solutions : " << endl;
    for (int i = 0; i <= 4; i++) {
      cout << M[0][i] << " ";
    }
    cout << endl;

    // Extract solution: a..e in M[0..4][5], f = 1
    return Conic(
        M[0][5],  // a
        M[1][5],  // b
        M[2][5],  // c
        M[3][5],  // d
        M[4][5],  // e
        1.0      // f
    );
  }
  */

  // cubic through nine points

static Cubic fitCubicThrough9(
    Point p1, Point p2, Point p3, Point p4,
    Point p5, Point p6, Point p7, Point p8,
    Point p9) {
    const Point pts[9] = {p1,p2,p3,p4,p5,p6,p7,p8,p9};

    // 1) compute column strengths for [x^3, x^2y, xy^2, y^3, x^2, xy, y^2, x, y, 1]
    long double strength[10] = {0};
    for (int i = 0; i < 9; ++i) {
        ld x = pts[i].x, y = pts[i].y;
        long double col[10] = { x*x*x, x*x*y, x*y*y, y*y*y,
                                x*x,   x*y,   y*y,
                                x,     y,
                                1.0L };
        for (int c = 0; c < 10; ++c)
            strength[c] += std::fabsl(col[c]);
    }
    int fixCol = std::max_element(strength, strength+10) - strength;

    // 2) build unknownIndices[9] mapping free columns
    int unknownIdx[9], idx = 0;
    for (int c = 0; c < 10; ++c) {
        if (c == fixCol) continue;
        unknownIdx[idx++] = c;
    }

    // 3) build augmented 9×10 matrix M[..][0..8]=coeff, [..][9]=RHS
    ld M[9][10];
    for (int r = 0; r < 9; ++r) {
        ld x = pts[r].x, y = pts[r].y;
        ld col[10] = { x*x*x, x*x*y, x*y*y, y*y*y,
                       x*x,   x*y,   y*y,
                       x,     y,
                       1.0L };
        // free columns
        for (int j = 0; j < 9; ++j)
            M[r][j] = col[ unknownIdx[j] ];
        // RHS = - fixed column term
        M[r][9] = -col[ fixCol ];
    }

    // 4) Gauss–Jordan elimination
    const ld EPS = std::numeric_limits<ld>::epsilon() * 1e3L;
    int n = 9, m = 9;
    for (int col = 0, row = 0; col < m && row < n; ++col, ++row) {
        int sel = row;
        for (int i = row+1; i < n; ++i)
            if (std::fabsl(M[i][col]) > std::fabsl(M[sel][col]))
                sel = i;
        if (std::fabsl(M[sel][col]) < EPS)
            throw std::runtime_error("Singular configuration: cannot fit cubic.");
        for (int j = col; j <= m; ++j)
            std::swap(M[sel][j], M[row][j]);
        ld inv = 1.0L / M[row][col];
        for (int j = col; j <= m; ++j)
            M[row][j] *= inv;
        for (int i = 0; i < n; ++i) {
            if (i == row) continue;
            ld fac = M[i][col];
            for (int j = col; j <= m; ++j)
                M[i][j] -= fac * M[row][j];
        }
    }

    // 5) reconstruct full coefficient vector [a..j]
    ld sol[10];
    sol[fixCol] = 1.0L;
    for (int j = 0; j < 9; ++j)
        sol[ unknownIdx[j] ] = M[j][9];

    return Cubic(
        sol[0], sol[1], sol[2], sol[3], sol[4],
        sol[5], sol[6], sol[7], sol[8], sol[9]
    );
}

  static Line angle_bisector(Point a, Point b, Point c) {
    AlgGeom::Point I = AlgGeom::CoreGeometryTools::incenter(a, b, c);
    return AlgGeom::Line(b, I);
  }

  static Point reflect_point_over_line(Point p, Line l) {
    // Denominator = a^2 + b^2
    long double denom = l.a*l.a + l.b*l.b;
    // Signed distance times (a,b)
    long double factor = (l.a*p.x + l.b*p.y + l.c) / denom;
    // Subtract twice the projection onto the normal
    return Point{
        p.x - 2*l.a * factor,
        p.y - 2*l.b * factor
    };
  }

  static Line reflect_line_over_line(const Line &l1, const Line &l2) {
    // 1) pick two distinct points on l1
    Point p1, p2;
    if (fabsl(l1.b) > 1e-12) {
        // let x=0 and x=1
        p1 = Point{0.0L,           -l1.c/l1.b};
        p2 = Point{1.0L, -(l1.a*1.0L + l1.c)/l1.b};
    } else {
        // vertical-ish line: let y=0 and y=1
        p1 = Point{-l1.c/l1.a, 0.0L};
        p2 = Point{-(l1.b*1.0L + l1.c)/l1.a, 1.0L};
    }

    // 2) reflect them across l2
    Point r1 = AlgGeom::CoreGeometryTools::reflect_point_over_line(p1, l2);
    Point r2 = AlgGeom::CoreGeometryTools::reflect_point_over_line(p2, l2);

    // 3) compute line through r1,r2: (y1 - y2) x + (x2 - x1) y + c = 0
    long double A = r1.y - r2.y;
    long double B = r2.x - r1.x;
    long double C = - (A*r1.x + B*r1.y);

    return Line{A, B, C};
  }

  static Point isogonal_conjugate(Point a, Point b, Point c, Point p) {
    AlgGeom::Line l1 = AlgGeom::CoreGeometryTools::angle_bisector(a, b, c);
    AlgGeom::Line l2 = AlgGeom::CoreGeometryTools::angle_bisector(a, c, b);

    AlgGeom::Line l = AlgGeom::CoreGeometryTools::reflect_line_over_line(
      AlgGeom::Line(b, p),
      l1
    );
    AlgGeom::Line ll = AlgGeom::CoreGeometryTools::reflect_line_over_line(
      AlgGeom::Line(c, p),
      l2
    );
    return AlgGeom::CoreGeometryTools::inter_lines(l, ll);
  }

  // intersection of two conics
  static vector<Point> intersectConics(const Conic &C1, const Conic &C2) {
    // Build coefficients of resultant polynomial R(x) = Res_y(C1, C2), degree ≤ 4
    // We eliminate y via Sylvester matrix determinant:
    // C1: c1*y^2 + (b1*x + e1)*y + (a1*x*x + d1*x + f1) = 0
    // C2: c2*y^2 + (b2*x + e2)*y + (a2*x*x + d2*x + f2) = 0
    // Sylvester 4×4 matrix rows: [c1, (b1 x+e1), (a1 x^2+d1 x+f1), 0;
    //                            0, c1, (b1 x+e1), (a1 x^2+d1 x+f1);
    //                            c2, (b2 x+e2), (a2 x^2+d2 x+f2), 0;
    //                            0, c2, (b2 x+e2), (a2 x^2+d2 x+f2)]
    // The determinant gives a degree-4 polynomial in x.
    // Expand by block structure to get coefficients r0..r4
    ld a1=C1.a, b1=C1.b, c1=C1.c, d1=C1.d, e1=C1.e, f1=C1.f;
    ld a2=C2.a, b2=C2.b, c2=C2.c, d2=C2.d, e2=C2.e, f2=C2.f;

    // Precompute powers
    // Let U1(x) = a1 x^2 + d1 x + f1, V1(x) = b1 x + e1
    //     U2(x) = a2 x^2 + d2 x + f2, V2(x) = b2 x + e2
    // Sylvester determinant simplifies to:
    // R(x) = c1^2 * U2(x)^2 - c1*c2*(U1(x)*U2(x) + V1(x)*V2(x))
    //        + c2^2 * U1(x)^2 + (b1*e2 - b2*e1)^2 * (x^2)
    // (one can verify this form; it's the resultant of two quadratics)
    // For safety, we build it by symbolic expansion:

    // We'll compute coefficients of R(x)=r4 x^4 + r3 x^3 + r2 x^2 + r1 x + r0
    array<ld,5> r = {0,0,0,0,0};
    auto add_poly = [&](const array<ld,5>& p, ld mul){
        for(int i=0;i<5;i++) r[i] += mul*p[i];
    };
    // Compute poly for U1^2 * c2*c2
    // U1^2: coeffs u10 x^4+u11 x^3+u12 x^2+u13 x+u14
    array<ld,5> u1sq = {
        a1*a1,
        2*a1*d1,
        2*a1*f1 + d1*d1,
        2*d1*f1,
        f1*f1
    };
    add_poly(u1sq, c2*c2);

    // poly for U2^2 * c1*c1
    array<ld,5> u2sq = {
        a2*a2,
        2*a2*d2,
        2*a2*f2 + d2*d2,
        2*d2*f2,
        f2*f2
    };
    add_poly(u2sq, c1*c1);

    // poly for -2*c1*c2 * U1*U2
    array<ld,5> u1u2 = {0,0,0,0,0};
    // U1*U2: degree 4
    // coefficients by convolution:
    array<ld,3> U1 = {a1, d1, f1}, U2 = {a2, d2, f2};
    for(int i=0;i<3;i++) for(int j=0;j<3;j++){
        u1u2[i+j] += U1[i]*U2[j];
    }
    add_poly(u1u2, -2*c1*c2);

    // poly for -2*c1*c2 * V1*V2 * x^2? Actually resultant includes term from y-coeff elimination:
    // V1*V2 = (b1 x + e1)*(b2 x + e2) = b1 b2 x^2 + (b1 e2 + b2 e1)x + e1 e2
    array<ld,5> v1v2 = {0,0,0,0,0};
    v1v2[2] = b1*b2;
    v1v2[1] = b1*e2 + b2*e1;
    v1v2[0] = e1*e2;
    add_poly(v1v2, -c1*c2);

    // Now r holds full resultant polynomial
    // Solve quartic r[4] x^4 + r[3] x^3 + r[2] x^2 + r[1] x + r[0] = 0 via Durand–Kerner
    int N = 4;
    vector<cd> roots(N);
    // initialize with roots of unity
    const cd init = polar((ld)1.0, (ld)(2*M_PI/N));
    for(int i=0;i<N;i++) roots[i] = pow(init,i);
    for(int iter=0; iter<1000; ++iter) {
        bool conv = true;
        for(int i=0;i<N;i++){
            cd num = ((roots[i]*(roots[i]*(roots[i]*(r[4]) + r[3]) + r[2]) + r[1])*roots[i] + r[0]);
            cd den = 1.0;
            for(int j=0;j<N;j++) if(i!=j) den *= (roots[i]-roots[j]);
            cd delta = num / den;
            roots[i] -= delta;
            if (abs(delta) > 1e-12) conv = false;
        }
        if (conv) break;
    }

    vector<Point> sol;
    for(auto &z: roots){
        if (abs(z.imag()) > 1e-6) continue;
        ld x = z.real();
        // solve C1 for y: c1 y^2 + (b1 x + e1)y + (a1 x^2 + d1 x + f1)=0
        ld A = c1;
        ld B = b1*x + e1;
        ld C = a1*x*x + d1*x + f1;
        ld D = B*B - 4*A*C;
        if (D < -EPS) continue;
        if (D < 0) D = 0;
        ld sq = sqrt(D);
        ld y1 = (-B + sq)/(2*A);
        ld y2 = (-B - sq)/(2*A);
        sol.emplace_back(x,y1);
        if (sq>EPS) sol.emplace_back(x,y2);
    }
    return sol;
  }

  static Point miquel_point(Point A, Point B, Point C, Point D) {
    // the “diagonals” AC and BD
    Line AC(A, C);
    Line BD(B, D);

    // their intersection
    Point R = AlgGeom::CoreGeometryTools::inter_lines(AC, BD);

    // Miquel point = isogonal conjugate of R in triangle ABC
    return AlgGeom::CoreGeometryTools::isogonal_conjugate(A, B, C, R);
  }

  // intersection of two lines
  static Point inter_lines(Line l1, Line l2) {
    ld det_val = det(l1.a, l1.b, l2.a, l2.b);
    if (det_val == 0) {
      throw runtime_error("Lines do not intersect (they are parallel).");
    }

    // Using Cramer's rule to find the intersection point:
    ld x = det(-l1.c, l1.b, -l2.c, l2.b) / det_val;
    ld y = det(l1.a, -l1.c, l2.a, -l2.c) / det_val;
    return Point(x, y);
  }

  static long double dist_points(Point p1, Point p2) {
    return sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y));
  }

  // Distance from a point to a line
  static long double dist_point_to_line(Point p, Line l) {
    return abs(l.a * p.x + l.b * p.y + l.c) / sqrt(l.a * l.a + l.b * l.b);
  }

  static Point project_point_to_line(Point p, Line l) {
    Line n_l = AlgGeom::CoreGeometryTools::perp_normal(p, l);
    return AlgGeom::CoreGeometryTools::inter_lines(l, n_l);
  }

  static Point project_point_to_circle(Point p, Circle c) {
    ld ratio = c.radius / AlgGeom::CoreGeometryTools::dist_points(p, c.p);
    return AlgGeom::Point(
      c.p.x + (p.x - c.p.x) * ratio,
      c.p.y + (p.y - c.p.y) * ratio
    );
  }

  static Point inversion_point(Point p, Circle c) {
    ld k = c.radius * c.radius / (
      AlgGeom::CoreGeometryTools::dist_points(p, c.p) *
      AlgGeom::CoreGeometryTools::dist_points(p, c.p)
    );
    return AlgGeom::Point(
      c.p.x + k * (p.x - c.p.x),
      c.p.y + k * (p.y - c.p.y)
    );
  }

  static Circle inversion_line(Line l, Circle c) {
    // Step 1: Calculate the distance from the center of circle C to the line L
    Point P_closest = AlgGeom::CoreGeometryTools::project_point_to_line(c.p, l);
    Point image = AlgGeom::CoreGeometryTools::inversion_point(P_closest, c);
    Point m = AlgGeom::CoreGeometryTools::midpoint(image, c.p);
    ld inverted_radius = AlgGeom::CoreGeometryTools::dist_points(m, c.p);
    return Circle(m, inverted_radius);
  }
  

  static Point incenter(Point p1, Point p2, Point p3) {
    ld a = AlgGeom::CoreGeometryTools::dist_points(p2, p3);
    ld b = AlgGeom::CoreGeometryTools::dist_points(p1, p3);
    ld c = AlgGeom::CoreGeometryTools::dist_points(p1, p2);
    
    ld x = (a * p1.x + b * p2.x + c * p3.x) / (a + b + c);
    ld y = (a * p1.y + b * p2.y + c * p3.y) / (a + b + c);

    return AlgGeom::Point(x, y);
  }

  static Point excenter(Point p1, Point p2, Point p3) {
    ld a = AlgGeom::CoreGeometryTools::dist_points(p2, p3);
    ld b = AlgGeom::CoreGeometryTools::dist_points(p1, p3);
    ld c = AlgGeom::CoreGeometryTools::dist_points(p1, p2);
    
    ld x = (a * p1.x + b * p2.x - c * p3.x) / (a + b - c);
    ld y = (a * p1.y + b * p2.y - c * p3.y) / (a + b - c);

    return AlgGeom::Point(x, y);
  }
  
  // Get the ratio parameter for point p on the line segment (p1, p2)
  static long double get_point_on_line_ratio(Point p1, Point p2, Point p) {
    // Check if the line is horizontal or vertical
    if (p2.x != p1.x) {
      // Line is not vertical, use x-coordinates
      return (p.x - p1.x) / (p2.x - p1.x);
    } else {
      // Line is vertical, use y-coordinates
      return (p.y - p1.y) / (p2.y - p1.y);
    }
  }

  // Get a point on the line based on the ratio parameter
  static Point get_point_on_line_via_ratio(Point p1, Point p2, long double ratio) {
    long double x = p1.x + ratio * (p2.x - p1.x);
    long double y = p1.y + ratio * (p2.y - p1.y);
    return Point(x, y);
  }

  // intersect line and circle
  static pair<Point, Point> inter_lc(Line l, Circle c) {
    cout << "l = " << l.a << " " << l.b << " " << l.c << endl;
    cout << "c = " << c.p.x << " " << c.p.y << " "<< c.radius << endl;

    // Step 1: Translate the line and circle so the circle center is at the origin.
    ld cx = c.p.x;
    ld cy = c.p.y;
    ld a = l.a;
    ld b = l.b;
    ld c_adjusted = l.c + a * cx + b * cy; // Shift constant term to match the new origin

    // Step 2: If b != 0, we solve for y in terms of x; otherwise, solve as a vertical line
    if (b != 0) {
        // If b != 0, the line can be expressed as y = mx + n
        ld m = -a / b;
        ld n = -c_adjusted / b;

        // Substitute y = mx + n into the circle equation: x^2 + y^2 = R^2
        // which becomes x^2 + (mx + n)^2 = R^2
        ld A = 1 + m * m;
        ld B = 2 * m * n;
        ld C = n * n - c.radius * c.radius;

        // Calculate the discriminant to solve the quadratic equation
        ld discriminant = B * B - 4 * A * C;
        if (discriminant < 0) {
            throw runtime_error("No intersection points (line does not intersect the circle).");
        }

        // Solve for x values using the quadratic formula
        ld sqrt_disc = sqrt(discriminant);
        ld x1 = (-B + sqrt_disc) / (2 * A);
        ld x2 = (-B - sqrt_disc) / (2 * A);

        // Corresponding y values
        ld y1 = m * x1 + n;
        ld y2 = m * x2 + n;

        // Step 3: Translate the points back to the original circle center
        return {Point(x1 + cx, y1 + cy), Point(x2 + cx, y2 + cy)};
    } else {
        // Special case: vertical line (a != 0, b == 0), so x = -c_adjusted / a
        ld x = -c_adjusted / a;
        // Substitute x into the circle equation to find corresponding y values
        ld D = c.radius * c.radius - (x * x);
        if (D < 0) {
            throw runtime_error("No intersection points (line does not intersect the circle).");
        }

        // Compute y values
        ld sqrt_D = sqrt(D);
        ld y1 = sqrt_D;
        ld y2 = -sqrt_D;

        // Translate points back
        return {Point(x + cx, y1 + cy), Point(x + cx, y2 + cy)};


    }
  }

  // are colinear
  static bool are_colinear(Point p1, Point p2, Point p3) {
    ld d1 = CoreGeometryTools::dist_points(p1, p2);
    ld d2 = CoreGeometryTools::dist_points(p2, p3);
    ld d3 = CoreGeometryTools::dist_points(p3, p1);

    vector<ld> val; val.push_back(d1); val.push_back(d2); val.push_back(d3);
    sort(val.begin(), val.end());

    return (abs(val[0] + val[1] - val[2]) < EPS);
  }

  // are colinear
  static bool are_cyclic(Point p1, Point p2, Point p3, Point p4) {
    return CoreGeometryTools::points_are_equal(
      CoreGeometryTools::inter_lines(
        CoreGeometryTools::perp_bisector(p1, p2),
        CoreGeometryTools::perp_bisector(p3, p4)
      ),
      CoreGeometryTools::inter_lines(
        CoreGeometryTools::perp_bisector(p2, p3),
        CoreGeometryTools::perp_bisector(p1, p4)
      )
    );
  }

  // are parallel
  static bool are_parallel(Line l1, Line l2) {
    return abs(l1.a * l2.b - l2.a * l1.b) < EPS;
  }

  // are perpendicular
  static bool are_perpendicular(Line l1, Line l2) {
    return abs(l1.a * l2.a + l1.b * l2.b) < EPS;
  }
};

}

















