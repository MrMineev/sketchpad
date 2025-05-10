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
            // cout << "[SEARCHER]: COLINEAR " << i << " " << j << " " << k << endl;
          }
        }
      }
    }

    for (GeoProp::Colinearity c : proper.col) {
      prot->new_line(machine.lines.size(), c.i1, c.i2, 2);
    }

    return proper;
  };
};










