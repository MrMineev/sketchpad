#pragma once

#include <iostream>
#include <vector>

using namespace std;

typedef long double ld;

#define det(a, b, c, d) ((a) * (d) - (b) * (c))

namespace AlgGeom {

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

class CoreGeometryTools {
 public:
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

  // equation of perp bisector

  static Line perp_bisector(Point p1, Point p2) {
    return AlgGeom::CoreGeometryTools::perp_normal(
      AlgGeom::CoreGeometryTools::midpoint(p1, p2),
      AlgGeom::Line(p1, p2)
    );
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
  
  static ld get_ratio_points(Point p1, Point p2, Point p) {
    return 0;
  }

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

};

}


