#pragma once

#include "property.h"
#include "../core/protocol/protocol.h"
#include "../../json_manager/json/single_include/nlohmann/json.hpp"

#include "../core/geometry.h"
#include "../core/alg.h"

AlgGeom::Point convert_gpoint(GPoint p) {
  return AlgGeom::Point(
    p.x_pos,
    p.y_pos
  );
}

namespace GeoGenie {
  static GeoProp::GeoProperties search(Protocol *prot) {
    GeoProp::GeoProperties proper;

    // Colinearity searcher
    GeometryVisual machine(0);
    machine.protocol = *prot;
    machine.rebuild();

    for (int i = 0; i < machine.points.size(); i++) {
      for (int j = 0; j < machine.points.size(); j++) {
        for (int k = 0; k < machine.points.size(); k++) {
          if (i == j || j == k || i == k) continue;
          AlgGeom::Point p1 = convert_gpoint(machine.points[i]);
          AlgGeom::Point p2 = convert_gpoint(machine.points[j]);
          AlgGeom::Point p3 = convert_gpoint(machine.points[k]);

          if (AlgGeom::CoreGeometryTools::are_colinear(p1, p2, p3)) {
            proper.col.push_back(GeoProp::Colinearity(i, j, k));
            cout << "[SEARCHER]: COLINEAR " << i << " " << j << " " << k << endl;
          }

          for (int h = 0; h < machine.points.size(); h++) {
            if (h == i || h == j || k == h) continue;
            AlgGeom::Point p4 = convert_gpoint(machine.points[h]);
            cout << i <<  " " << j << " " << k << " " << h << endl;
            if (AlgGeom::CoreGeometryTools::are_cyclic(p1, p2, p3, p4)) {
              proper.cyc.push_back(GeoProp::Cyclic(i, j, k, h));
              cout << "[SEARCHER]: CYCLIC " << i << " " << j << " " << k << " " << h << endl;
            }
          }
        }
      }
    }



    // Colinearity
    int cnt = 0;
    for (GeoProp::Colinearity c : proper.col) {
      // cout << "new property " << c.i1 << " " << c.i2 << endl;
      prot->new_line(machine.lines.size() + cnt, c.i1, c.i2, 2);
      cnt++;
    }

    // Cyclicity
    cnt = 0;
    for (GeoProp::Cyclic c : proper.cyc) {
      // cout << "new property " << c.i1 << " " << c.i2 << endl;
      prot->new_circumcircle(machine.circles.size() + cnt, c.i1, c.i2, c.i3);
      // prot->new_line(machine.lines.size() + cnt, c.i1, c.i2, 2);
      cnt++;
    }


    return proper;
  };
};










