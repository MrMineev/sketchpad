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
  ld a, b, c; // ax + by = c

  Line(Point p1, Point p2) {
    // | x    y    1|
    // | p1.x p1.y 1|
    // | p2.x p2.y 1|
    a = det(p1.y, 1, p2.y, 1);
    b = -det(p1.x, 1, p2.x, 1);
    c = det(p1.x, p1.y, p2.x, p2.y);
  }

  Line() {}
};

class CoreGeometryTools {
 public:
  // equation for a line through P perpendicular to L
  static Line perp_normal(Point p, Line l);

  // intersection of two lines
  static Point inter_points(Line l1, Line l2);

  static long double dist_points(Point p1, Point p2) {
    return sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y));
  }
};

}

