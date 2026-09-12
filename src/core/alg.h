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

  static bool rectangular_hyperbola(Point center, Point p1, Point p2, Conic &result) {
    const ld u1 = p1.x - center.x;
    const ld v1 = p1.y - center.y;
    const ld u2 = p2.x - center.x;
    const ld v2 = p2.y - center.y;
    const ld q1 = u1 * u1 - v1 * v1;
    const ld r1 = u1 * v1;
    const ld q2 = u2 * u2 - v2 * v2;
    const ld r2 = u2 * v2;
    ld a = r1 - r2;
    ld b = q2 - q1;
    ld f = q1 * r2 - r1 * q2;
    const ld scale = max(abs(a), abs(b));
    if (scale < EPS || abs(f) < EPS) return false;
    a /= scale;
    b /= scale;
    f /= scale;
    result = Conic(
      a, b, -a,
      -2 * a * center.x - b * center.y,
      2 * a * center.y - b * center.x,
      a * (center.x * center.x - center.y * center.y) + b * center.x * center.y + f
    );
    return true;
  }

  static bool conic_center(Conic conic, Point &result) {
    const ld determinant = 4 * conic.a * conic.c - conic.b * conic.b;
    if (abs(determinant) < EPS) return false;
    result = Point(
      (conic.b * conic.e - 2 * conic.c * conic.d) / determinant,
      (conic.b * conic.d - 2 * conic.a * conic.e) / determinant
    );
    return isfinite(result.x) && isfinite(result.y);
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

  static Point reflect_point_over_point(Point p, Point center) {
    return Point(2 * center.x - p.x, 2 * center.y - p.y);
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

  static bool isotomic_conjugate(Point a, Point b, Point c, Point p, Point &result) {
    const ld denominator = (b.y - c.y) * (a.x - c.x) +
                           (c.x - b.x) * (a.y - c.y);
    if (abs(denominator) < EPS) return false;
    const ld x = ((b.y - c.y) * (p.x - c.x) +
                  (c.x - b.x) * (p.y - c.y)) / denominator;
    const ld y = ((c.y - a.y) * (p.x - c.x) +
                  (a.x - c.x) * (p.y - c.y)) / denominator;
    const ld z = 1 - x - y;
    if (abs(x) < EPS || abs(y) < EPS || abs(z) < EPS) return false;
    const ld wa = y * z;
    const ld wb = z * x;
    const ld wc = x * y;
    const ld sum = wa + wb + wc;
    if (abs(sum) < EPS) return false;
    result = Point(
      (wa * a.x + wb * b.x + wc * c.x) / sum,
      (wa * a.y + wb * b.y + wc * c.y) / sum
    );
    return isfinite(result.x) && isfinite(result.y);
  }

  static void deduplicate_points(vector<Point> &points) {
    vector<Point> unique;
    for (const Point &point : points) {
      bool duplicate = false;
      for (const Point &other : unique) {
        const ld scale = max<ld>(1, max(abs(point.x) + abs(point.y), abs(other.x) + abs(other.y)));
        if (CoreGeometryTools::dist_points(point, other) <= 1e-7L * scale) {
          duplicate = true;
          break;
        }
      }
      if (!duplicate && isfinite(point.x) && isfinite(point.y)) unique.push_back(point);
    }
    sort(unique.begin(), unique.end(), [](const Point &first, const Point &second) {
      if (abs(first.x - second.x) > 1e-8L) return first.x < second.x;
      return first.y < second.y;
    });
    points = unique;
  }

  static vector<ld> real_polynomial_roots(vector<ld> coefficients) {
    ld scale = 0;
    for (ld coefficient : coefficients) scale = max(scale, abs(coefficient));
    if (scale < 1e-18L) return {};
    while (coefficients.size() > 1 && abs(coefficients.back()) < scale * 1e-14L) {
      coefficients.pop_back();
    }
    const int degree = static_cast<int>(coefficients.size()) - 1;
    if (degree == 0) return {};
    if (degree == 1) return {-coefficients[0] / coefficients[1]};
    if (degree == 2) {
      const ld a = coefficients[2];
      const ld b = coefficients[1];
      const ld c = coefficients[0];
      ld discriminant = b * b - 4 * a * c;
      const ld tolerance = 1e-14L * max<ld>(1, b * b + abs(4 * a * c));
      if (discriminant < -tolerance) return {};
      if (abs(discriminant) <= tolerance) return {-b / (2 * a)};
      discriminant = sqrt(max<ld>(0, discriminant));
      const ld q = -0.5L * (b + (b >= 0 ? discriminant : -discriminant));
      vector<ld> roots = {q / a, c / q};
      sort(roots.begin(), roots.end());
      return roots;
    }

    const ld leading = coefficients.back();
    ld radius = 1;
    for (int i = 0; i < degree; ++i) radius = max(radius, 1 + abs(coefficients[i] / leading));
    vector<cd> roots(degree);
    const ld pi = acosl(-1.0L);
    for (int i = 0; i < degree; ++i) {
      roots[i] = polar(radius, 2 * pi * (i + 0.37L) / degree);
    }
    for (int iteration = 0; iteration < 2000; ++iteration) {
      const vector<cd> previous = roots;
      ld largest_change = 0;
      for (int i = 0; i < degree; ++i) {
        cd value = coefficients.back();
        for (int j = degree - 1; j >= 0; --j) value = value * previous[i] + coefficients[j];
        cd denominator = 1;
        for (int j = 0; j < degree; ++j) {
          if (i != j) denominator *= previous[i] - previous[j];
        }
        if (abs(denominator) < 1e-24L) denominator += cd(1e-18L, 1e-18L);
        const cd change = value / denominator;
        roots[i] = previous[i] - change;
        largest_change = max(largest_change, abs(change));
      }
      if (largest_change < 1e-14L) break;
    }
    vector<ld> real_roots;
    for (cd root : roots) {
      if (abs(root.imag()) <= 1e-8L * max<ld>(1, abs(root.real()))) {
        ld value = root.real();
        for (int iteration = 0; iteration < 8; ++iteration) {
          ld polynomial = coefficients.back();
          ld derivative = degree * coefficients.back();
          for (int i = degree - 1; i >= 1; --i) {
            polynomial = polynomial * value + coefficients[i];
            derivative = derivative * value + i * coefficients[i];
          }
          polynomial = polynomial * value + coefficients[0];
          if (abs(derivative) < 1e-18L) break;
          value -= polynomial / derivative;
        }
        real_roots.push_back(value);
      }
    }
    sort(real_roots.begin(), real_roots.end());
    real_roots.erase(unique(real_roots.begin(), real_roots.end(), [](ld first, ld second) {
      return abs(first - second) <= 1e-7L * max<ld>(1, max(abs(first), abs(second)));
    }), real_roots.end());
    return real_roots;
  }

  static vector<Point> inter_line_line(Line first, Line second) {
    const ld determinant = first.a * second.b - second.a * first.b;
    const ld scale = sqrt(first.a * first.a + first.b * first.b) *
                     sqrt(second.a * second.a + second.b * second.b);
    if (scale < EPS || abs(determinant) <= EPS * scale) return {};
    return {Point(
      (first.b * second.c - second.b * first.c) / determinant,
      (first.c * second.a - second.c * first.a) / determinant
    )};
  }

  static vector<Point> inter_line_conic(Line line, Conic conic) {
    vector<Point> result;
    auto solve = [&](ld quadratic, ld linear, ld constant, auto make_point) {
      for (ld root : CoreGeometryTools::real_polynomial_roots({constant, linear, quadratic})) {
        result.push_back(make_point(root));
      }
    };
    if (abs(line.b) >= abs(line.a) && abs(line.b) > EPS) {
      const ld m = -line.a / line.b;
      const ld n = -line.c / line.b;
      solve(
        conic.a + conic.b * m + conic.c * m * m,
        conic.b * n + 2 * conic.c * m * n + conic.d + conic.e * m,
        conic.c * n * n + conic.e * n + conic.f,
        [&](ld x) { return Point(x, m * x + n); }
      );
    } else if (abs(line.a) > EPS) {
      const ld m = -line.b / line.a;
      const ld n = -line.c / line.a;
      solve(
        conic.c + conic.b * m + conic.a * m * m,
        conic.b * n + 2 * conic.a * m * n + conic.e + conic.d * m,
        conic.a * n * n + conic.d * n + conic.f,
        [&](ld y) { return Point(m * y + n, y); }
      );
    }
    CoreGeometryTools::deduplicate_points(result);
    return result;
  }

  static vector<Point> inter_circle_circle(Circle first, Circle second) {
    vector<Point> result;
    const ld dx = second.p.x - first.p.x;
    const ld dy = second.p.y - first.p.y;
    const ld distance = sqrt(dx * dx + dy * dy);
    if (distance < EPS || distance > first.radius + second.radius + EPS ||
        distance < abs(first.radius - second.radius) - EPS) return result;
    const ld along = (first.radius * first.radius - second.radius * second.radius +
                      distance * distance) / (2 * distance);
    ld height_squared = first.radius * first.radius - along * along;
    if (height_squared < -EPS) return result;
    const ld height = sqrt(max<ld>(0, height_squared));
    const Point base(first.p.x + along * dx / distance, first.p.y + along * dy / distance);
    result.push_back(Point(base.x - height * dy / distance, base.y + height * dx / distance));
    if (height > EPS) {
      result.push_back(Point(base.x + height * dy / distance, base.y - height * dx / distance));
    }
    CoreGeometryTools::deduplicate_points(result);
    return result;
  }

  static Conic circle_as_conic(Circle circle) {
    return Conic(
      1, 0, 1, -2 * circle.p.x, -2 * circle.p.y,
      circle.p.x * circle.p.x + circle.p.y * circle.p.y - circle.radius * circle.radius
    );
  }

  static vector<Point> inter_conic_conic(Conic first, Conic second) {
    using Polynomial = vector<ld>;
    auto add = [](Polynomial a, const Polynomial &b, ld factor = 1) {
      a.resize(max(a.size(), b.size()), 0);
      for (size_t i = 0; i < b.size(); ++i) a[i] += factor * b[i];
      return a;
    };
    auto multiply = [](const Polynomial &a, const Polynomial &b) {
      Polynomial result(a.size() + b.size() - 1, 0);
      for (size_t i = 0; i < a.size(); ++i) {
        for (size_t j = 0; j < b.size(); ++j) result[i + j] += a[i] * b[j];
      }
      return result;
    };
    const Polynomial u1 = {first.f, first.d, first.a};
    const Polynomial u2 = {second.f, second.d, second.a};
    const Polynomial v1 = {first.e, first.b};
    const Polynomial v2 = {second.e, second.b};
    Polynomial p = add(Polynomial(u2.size(), 0), u2, first.c);
    p = add(p, u1, -second.c);
    Polynomial q = add(Polynomial(v1.size(), 0), v1, second.c);
    q = add(q, v2, -first.c);
    Polynomial r = add(multiply(v1, u2), multiply(v2, u1), -1);
    Polynomial resultant = add(multiply(p, p), multiply(q, r));
    ld resultant_scale = 0;
    for (ld coefficient : resultant) resultant_scale = max(resultant_scale, abs(coefficient));
    if (resultant_scale < 1e-18L && abs(first.c) < EPS && abs(second.c) < EPS &&
        (abs(first.a) >= EPS || abs(second.a) >= EPS)) {
      const Conic swapped_first(first.c, first.b, first.a, first.e, first.d, first.f);
      const Conic swapped_second(second.c, second.b, second.a, second.e, second.d, second.f);
      vector<Point> swapped = CoreGeometryTools::inter_conic_conic(swapped_first, swapped_second);
      for (Point &point : swapped) std::swap(point.x, point.y);
      CoreGeometryTools::deduplicate_points(swapped);
      return swapped;
    }

    vector<Point> result;
    for (ld x : CoreGeometryTools::real_polynomial_roots(resultant)) {
      const ld u_first = first.a * x * x + first.d * x + first.f;
      const ld v_first = first.b * x + first.e;
      const ld u_second = second.a * x * x + second.d * x + second.f;
      const ld v_second = second.b * x + second.e;
      vector<ld> y_roots;
      if (abs(first.c) >= abs(second.c) && abs(first.c) > EPS) {
        y_roots = CoreGeometryTools::real_polynomial_roots({u_first, v_first, first.c});
      } else if (abs(second.c) > EPS) {
        y_roots = CoreGeometryTools::real_polynomial_roots({u_second, v_second, second.c});
      } else if (abs(v_first) >= abs(v_second) && abs(v_first) > EPS) {
        y_roots = {-u_first / v_first};
      } else if (abs(v_second) > EPS) {
        y_roots = {-u_second / v_second};
      }
      for (ld y : y_roots) {
        const ld first_value = first.a * x * x + first.b * x * y + first.c * y * y +
                               first.d * x + first.e * y + first.f;
        const ld second_value = second.a * x * x + second.b * x * y + second.c * y * y +
                                second.d * x + second.e * y + second.f;
        const ld coordinate_scale = max<ld>(1, x * x + y * y);
        if (abs(first_value) <= 1e-5L * coordinate_scale &&
            abs(second_value) <= 1e-5L * coordinate_scale) result.push_back(Point(x, y));
      }
    }
    CoreGeometryTools::deduplicate_points(result);
    return result;
  }

  static vector<Point> inter_circle_conic(Circle circle, Conic conic) {
    return CoreGeometryTools::inter_conic_conic(CoreGeometryTools::circle_as_conic(circle), conic);
  }

  static bool tangent_from_point(Point point, Circle circle, int branch, Line &result) {
    const ld dx = point.x - circle.p.x;
    const ld dy = point.y - circle.p.y;
    const ld distance_squared = dx * dx + dy * dy;
    const ld radius_squared = circle.radius * circle.radius;
    if (distance_squared < EPS * EPS || circle.radius < EPS ||
        distance_squared < radius_squared - EPS) return false;
    if (abs(distance_squared - radius_squared) <= EPS) {
      if (branch != 0) return false;
      result = Line(dx, dy, -(dx * point.x + dy * point.y));
      return true;
    }
    const ld along = radius_squared / distance_squared;
    const ld side = circle.radius * sqrt(distance_squared - radius_squared) / distance_squared;
    const Point base(circle.p.x + along * dx, circle.p.y + along * dy);
    const Point contact(
      base.x + (branch == 0 ? -side * dy : side * dy),
      base.y + (branch == 0 ? side * dx : -side * dx)
    );
    result = Line(point, contact);
    return branch == 0 || branch == 1;
  }

  static bool common_tangent(Circle first, Circle second, int branch, Line &result) {
    const ld dx = second.p.x - first.p.x;
    const ld dy = second.p.y - first.p.y;
    const ld distance_squared = dx * dx + dy * dy;
    if (distance_squared < EPS * EPS || first.radius < EPS || second.radius < EPS ||
        branch < 0 || branch > 3) return false;
    const bool transverse = branch >= 2;
    const ld signed_radius = transverse ? -(first.radius + second.radius)
                                        : second.radius - first.radius;
    if (signed_radius * signed_radius > distance_squared + EPS) return false;
    const ld projection = signed_radius / distance_squared;
    const ld side = sqrt(max<ld>(0, distance_squared - signed_radius * signed_radius)) /
                    distance_squared * (branch % 2 == 0 ? 1 : -1);
    const ld a = projection * dx - side * dy;
    const ld b = projection * dy + side * dx;
    if (branch % 2 == 1 && abs(distance_squared - signed_radius * signed_radius) <= EPS) return false;
    result = Line(a, b, first.radius - a * first.p.x - b * first.p.y);
    return true;
  }

  static bool polar_line(Point point, Conic conic, Line &result) {
    const ld a = 2 * conic.a * point.x + conic.b * point.y + conic.d;
    const ld b = conic.b * point.x + 2 * conic.c * point.y + conic.e;
    const ld c = conic.d * point.x + conic.e * point.y + 2 * conic.f;
    if (a * a + b * b < EPS * EPS) return false;
    result = Line(a, b, c);
    return true;
  }

  static bool radical_axis(Circle first, Circle second, Line &result) {
    const ld a = 2 * (second.p.x - first.p.x);
    const ld b = 2 * (second.p.y - first.p.y);
    const ld c = first.p.x * first.p.x + first.p.y * first.p.y -
                 first.radius * first.radius - second.p.x * second.p.x -
                 second.p.y * second.p.y + second.radius * second.radius;
    if (a * a + b * b < EPS * EPS) return false;
    result = Line(a, b, c);
    return true;
  }

  static bool radical_center(Circle first, Circle second, Circle third, Point &result) {
    Line first_axis, second_axis;
    if (!CoreGeometryTools::radical_axis(first, second, first_axis) ||
        !CoreGeometryTools::radical_axis(second, third, second_axis)) return false;
    const vector<Point> intersections = CoreGeometryTools::inter_line_line(first_axis, second_axis);
    if (intersections.empty()) return false;
    result = intersections.front();
    return true;
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

  static bool project_point_to_conic(Point point, Conic conic, Point &result) {
    result = point;
    for (int iteration = 0; iteration < 40; ++iteration) {
      const ld value = conic.a * result.x * result.x + conic.b * result.x * result.y +
                       conic.c * result.y * result.y + conic.d * result.x +
                       conic.e * result.y + conic.f;
      const ld gradient_x = 2 * conic.a * result.x + conic.b * result.y + conic.d;
      const ld gradient_y = conic.b * result.x + 2 * conic.c * result.y + conic.e;
      const ld gradient_squared = gradient_x * gradient_x + gradient_y * gradient_y;
      if (gradient_squared < EPS * EPS) return false;
      if (abs(value) / sqrt(gradient_squared) < EPS) return true;
      const ld step = value / gradient_squared;
      result.x -= step * gradient_x;
      result.y -= step * gradient_y;
      if (!isfinite(result.x) || !isfinite(result.y)) return false;
    }
    const ld value = conic.a * result.x * result.x + conic.b * result.x * result.y +
                     conic.c * result.y * result.y + conic.d * result.x +
                     conic.e * result.y + conic.f;
    const ld gradient_x = 2 * conic.a * result.x + conic.b * result.y + conic.d;
    const ld gradient_y = conic.b * result.x + 2 * conic.c * result.y + conic.e;
    const ld gradient = sqrt(gradient_x * gradient_x + gradient_y * gradient_y);
    return gradient > EPS && abs(value) / gradient < EPS;
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
  

  static bool circumcenter(Point p1, Point p2, Point p3, Point &result) {
    const ld denominator = 2 * (p1.x * (p2.y - p3.y) +
                                p2.x * (p3.y - p1.y) +
                                p3.x * (p1.y - p2.y));
    if (abs(denominator) < EPS) return false;
    const ld n1 = p1.x * p1.x + p1.y * p1.y;
    const ld n2 = p2.x * p2.x + p2.y * p2.y;
    const ld n3 = p3.x * p3.x + p3.y * p3.y;
    result = Point(
      (n1 * (p2.y - p3.y) + n2 * (p3.y - p1.y) + n3 * (p1.y - p2.y)) / denominator,
      (n1 * (p3.x - p2.x) + n2 * (p1.x - p3.x) + n3 * (p2.x - p1.x)) / denominator
    );
    return isfinite(result.x) && isfinite(result.y);
  }

  static bool triangle_center(int number, Point p1, Point p2, Point p3, Point &result) {
    if (number < 1 || number > 100) return false;
    const ld a = CoreGeometryTools::dist_points(p2, p3);
    const ld b = CoreGeometryTools::dist_points(p3, p1);
    const ld c = CoreGeometryTools::dist_points(p1, p2);
    const ld semiperimeter = (a + b + c) / 2;
    const ld twice_area = abs((p2.x - p1.x) * (p3.y - p1.y) -
                              (p2.y - p1.y) * (p3.x - p1.x));
    if (twice_area < EPS) return false;

    auto barycentric = [&](ld x, ld y, ld z) {
      const ld sum = x + y + z;
      if (abs(sum) < EPS) return false;
      result = Point(
        (x * p1.x + y * p2.x + z * p3.x) / sum,
        (x * p1.y + y * p2.y + z * p3.y) / sum
      );
      return isfinite(result.x) && isfinite(result.y);
    };

    if (number == 1) return barycentric(a, b, c);
    if (number == 2) return barycentric(1, 1, 1);

    Point circum;
    if (!CoreGeometryTools::circumcenter(p1, p2, p3, circum)) return false;
    if (number == 3) {
      result = circum;
      return true;
    }
    const Point orthocenter(
      p1.x + p2.x + p3.x - 2 * circum.x,
      p1.y + p2.y + p3.y - 2 * circum.y
    );
    if (number == 4) {
      result = orthocenter;
      return true;
    }
    if (number == 5) {
      result = CoreGeometryTools::midpoint(circum, orthocenter);
      return true;
    }
    if (number == 6) return barycentric(a * a, b * b, c * c);
    if (number == 7) {
      if (semiperimeter - a < EPS || semiperimeter - b < EPS || semiperimeter - c < EPS) return false;
      return barycentric(
        1 / (semiperimeter - a), 1 / (semiperimeter - b), 1 / (semiperimeter - c)
      );
    }
    if (number == 8) {
      return barycentric(semiperimeter - a, semiperimeter - b, semiperimeter - c);
    }
    if (number == 9) {
      return barycentric(
        a * (semiperimeter - a), b * (semiperimeter - b), c * (semiperimeter - c)
      );
    }
    if (number == 10) return barycentric(b + c, c + a, a + b);

    const ld area = twice_area / 2;
    const ld pi = acosl(-1.0L);
    auto angle = [](ld opposite, ld adjacent1, ld adjacent2) {
      const ld cosine = std::clamp(
        (adjacent1 * adjacent1 + adjacent2 * adjacent2 - opposite * opposite) /
        (2 * adjacent1 * adjacent2), -1.0L, 1.0L
      );
      return acosl(cosine);
    };
    const ld angle_a = angle(a, b, c);
    const ld angle_b = angle(b, c, a);
    const ld angle_c = angle(c, a, b);
    const ld undefined = std::numeric_limits<ld>::quiet_NaN();
    auto divide = [&](ld numerator, ld denominator) {
      return abs(denominator) < 1e-12L ? undefined : numerator / denominator;
    };
    auto coordinate = [&](ld x, ld y, ld z, ld X, ld Y, ld Z) {
      const ld x2 = x * x, y2 = y * y, z2 = z * z;
      const ld x4 = x2 * x2, y4 = y2 * y2, z4 = z2 * z2;
      switch (number) {
        case 11: return (y + z - x) * (y - z) * (y - z);
        case 12: return divide((y + z) * (y + z), y + z - x);
        case 13: return x4 - 2 * (y2 - z2) * (y2 - z2) + x2 * (y2 + z2 + 4 * sqrt(3.0L) * area);
        case 14: return x4 - 2 * (y2 - z2) * (y2 - z2) + x2 * (y2 + z2 - 4 * sqrt(3.0L) * area);
        case 15: return x * sin(X + pi / 3);
        case 16: return x * sin(X - pi / 3);
        case 17: return divide(x, sin(X + pi / 6));
        case 18: return divide(x, sin(X - pi / 6));
        case 19: return x * tan(X);
        case 20: return -3 * x4 + 2 * x2 * (y2 + z2) + (y2 - z2) * (y2 - z2);
        case 21: return divide(x, cos(Y) + cos(Z));
        case 22: return x2 * (y4 + z4 - x4);
        case 23: return x2 * (y4 + z4 - x4 - y2 * z2);
        case 24: return tan(X) * cos(2 * X);
        case 25: return divide(x2, y2 + z2 - x2);
        case 26: return x2 * (y2 * cos(2 * Y) + z2 * cos(2 * Z) - x2 * cos(2 * X));
        case 27: return divide(tan(X), y + z);
        case 28: return divide(sin(X) * tan(X), y + z);
        case 29: return divide(tan(X), cos(Y) + cos(Z));
        case 30: return 2 * x4 - (y2 - z2) * (y2 - z2) - x2 * (y2 + z2);
        case 31: return x * x2;
        case 32: return x4;
        case 33: return sin(X) + tan(X);
        case 34: return sin(X) - tan(X);
        case 35: return x2 * (y2 + z2 - x2 + y * z);
        case 36: return x2 * (y2 + z2 - x2 - y * z);
        case 37: return x * (y + z);
        case 38: return x * (y2 + z2);
        case 39: return x2 * (y2 + z2);
        case 40: return x * (divide(y, z + x - y) + divide(z, x + y - z) - divide(x, y + z - x));
        case 41: return x * x2 * (y + z - x);
        case 42: return x2 * (y + z);
        case 43: return x * (x * y + x * z - y * z);
        case 44: return x * (y + z - 2 * x);
        case 45: return x * (2 * y + 2 * z - x);
        case 46: return x * (cos(Y) + cos(Z) - cos(X));
        case 47: return x * cos(2 * X);
        case 48: return x * sin(2 * X);
        case 49: return sin(X) * cos(3 * X);
        case 50: return sin(X) * sin(3 * X);
        case 51: return x * x2 * cos(Y - Z);
        case 52: return tan(X) * (divide(1, cos(2 * Y)) + divide(1, cos(2 * Z)));
        case 53: return x * tan(X) * cos(Y - Z);
        case 54: return divide(sin(X), cos(Y - Z));
        case 55: return x2 * (y + z - x);
        case 56: return divide(x2, y + z - x);
        case 57: return divide(x, y + z - x);
        case 58: return divide(x2, y + z);
        case 59: return divide(x, 1 - cos(Y - Z));
        case 60: return divide(x, 1 + cos(Y - Z));
        case 61: return sin(X) * sin(X + pi / 6);
        case 62: return sin(X) * sin(X - pi / 6);
        case 63: return cos(X);
        case 64: return divide(x, cos(X) - cos(Y) * cos(Z));
        case 65: return divide(x * (y + z), y + z - x);
        case 66: return divide(1, y4 + z4 - x4);
        case 67: return divide(1, y4 + z4 - x4 - y2 * z2);
        case 68: return tan(2 * X);
        case 69: return divide(1, tan(X));
        case 70: return divide(1, y2 * cos(2 * Y) + z2 * cos(2 * Z) - x2 * cos(2 * X));
        case 71: return (y + z) * sin(2 * X);
        case 72: return (y + z) * cos(X);
        case 73: return (cos(Y) + cos(Z)) * sin(2 * X);
        case 74: return divide(x, cos(X) - 2 * cos(Y) * cos(Z));
        case 75: return divide(1, x);
        case 76: return divide(1, x2);
        case 77: return divide(x, 1 + divide(1, cos(X)));
        case 78: return divide(x, 1 - divide(1, cos(X)));
        case 79: return divide(1, y2 + z2 - x2 + y * z);
        case 80: return divide(1, y2 + z2 - x2 - y * z);
        case 81: return divide(x, y + z);
        case 82: return divide(x, y2 + z2);
        case 83: return divide(1, y2 + z2);
        case 84: return divide(sin(X), cos(Y) + cos(Z) - cos(X) - 1);
        case 85: return divide(y * z, y + z - x);
        case 86: return divide(1, y + z);
        case 87: return divide(x, x * y + x * z - y * z);
        case 88: return divide(x, y + z - 2 * x);
        case 89: return divide(x, 2 * y + 2 * z - x);
        case 90: return divide(x, cos(Y) + cos(Z) - cos(X));
        case 91: return divide(sin(X), cos(2 * X));
        case 92: return divide(1, cos(X));
        case 93: return divide(sin(X), cos(3 * X));
        case 94: return divide(sin(X), sin(3 * X));
        case 95: return divide(y * z, cos(Y - Z));
        case 96: return divide(x, cos(2 * X) * cos(Y - Z));
        case 97: return divide(cos(X), cos(Y - Z));
        case 98: return divide(1, y4 + z4 - x2 * y2 - x2 * z2);
        case 99: return divide(1, y2 - z2);
        case 100: return divide(x, y - z);
      }
      return undefined;
    };
    return barycentric(
      coordinate(a, b, c, angle_a, angle_b, angle_c),
      coordinate(b, c, a, angle_b, angle_c, angle_a),
      coordinate(c, a, b, angle_c, angle_a, angle_b)
    );
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
    const ld d1 = CoreGeometryTools::dist_points(p1, p2);
    const ld d2 = CoreGeometryTools::dist_points(p2, p3);
    const ld d3 = CoreGeometryTools::dist_points(p3, p1);
    const ld longest_side = max(d1, max(d2, d3));
    if (longest_side < EPS) return false;
    const ld twice_area = abs((p2.x - p1.x) * (p3.y - p1.y) -
                              (p2.y - p1.y) * (p3.x - p1.x));
    return twice_area / longest_side < EPS;
  }

  // are colinear
  static bool are_cyclic(Point p1, Point p2, Point p3, Point p4) {
    const Point points[] = {p1, p2, p3, p4};
    for (int i = 0; i < 4; ++i) {
      for (int j = i + 1; j < 4; ++j) {
        if (CoreGeometryTools::points_are_equal(points[i], points[j])) return false;
      }
    }

    const ld denominator = 2 * (p1.x * (p2.y - p3.y) +
                                p2.x * (p3.y - p1.y) +
                                p3.x * (p1.y - p2.y));
    if (abs(denominator) < EPS) return false;

    const ld p1_norm = p1.x * p1.x + p1.y * p1.y;
    const ld p2_norm = p2.x * p2.x + p2.y * p2.y;
    const ld p3_norm = p3.x * p3.x + p3.y * p3.y;
    const Point center(
      (p1_norm * (p2.y - p3.y) + p2_norm * (p3.y - p1.y) +
       p3_norm * (p1.y - p2.y)) / denominator,
      (p1_norm * (p3.x - p2.x) + p2_norm * (p1.x - p3.x) +
       p3_norm * (p2.x - p1.x)) / denominator
    );
    const ld radius = CoreGeometryTools::dist_points(center, p1);
    return abs(CoreGeometryTools::dist_points(center, p4) - radius) < EPS;
  }

  static bool are_concurrent(Line l1, Line l2, Line l3) {
    auto intersects_on = [](Line first, Line second, Line third) {
      const ld first_norm = sqrt(first.a * first.a + first.b * first.b);
      const ld second_norm = sqrt(second.a * second.a + second.b * second.b);
      const ld third_norm = sqrt(third.a * third.a + third.b * third.b);
      if (first_norm < EPS || second_norm < EPS || third_norm < EPS) return false;
      const ld denominator = first.a * second.b - second.a * first.b;
      if (abs(denominator) < EPS * first_norm * second_norm) return false;
      const Point intersection(
        (first.b * second.c - second.b * first.c) / denominator,
        (first.c * second.a - second.c * first.a) / denominator
      );
      return abs(third.a * intersection.x + third.b * intersection.y + third.c) / third_norm < EPS;
    };
    return intersects_on(l1, l2, l3) || intersects_on(l1, l3, l2) || intersects_on(l2, l3, l1);
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

















