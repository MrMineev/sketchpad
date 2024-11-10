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
};

}


