#pragma once

#include <vector>

namespace GeoProp {

struct Colinearity { // colinearity of points
  int i1, i2, i3;

  Colinearity(int _i1, int _i2, int _i3) : i1(_i1), i2(_i2), i3(_i3) {}
};

struct Cyclic { // four points are cyclic
  int i1, i2, i3, i4;

  Cyclic(int _i1, int _i2, int _i3, int _i4) : i1(_i1), i2(_i2), i3(_i3), i4(_i4) {}
};

struct Parallel { // two lines are parallel
  int i1, i2;

  Parallel(int _i1, int _i2) : i1(_i1), i2(_i2) {}
};

struct Perpendicular {
  int i1, i2;

  Perpendicular(int _i1, int _i2) : i1(_i1), i2(_i2) {}
};

struct GeoProperties {
  std::vector<Colinearity> col;
  std::vector<Cyclic> cyc;
  std::vector<Parallel> par;
  std::vector<Perpendicular> perp;

  GeoProperties() {}
};

};



