#pragma once

#include "property.h"
#include "../core/protocol/protocol.h"
#include "../../json_manager/json/single_include/nlohmann/json.hpp"

#include "../core/geometry.h"
#include "../core/alg.h"

#include <algorithm>
#include <set>
#include <tuple>

inline AlgGeom::Point convert_gpoint(const GPoint &p) {
  return AlgGeom::Point(p.x_pos, p.y_pos);
}

inline AlgGeom::Line convert_gline(const GLine &line) {
  return AlgGeom::Line(
    AlgGeom::Point(line.x1, line.y1),
    AlgGeom::Point(line.x2, line.y2)
  );
}

namespace GeoGenie {
  static std::vector<int> searchable_indices(const Protocol *prot, const std::string &category, int size) {
    std::vector<int> indices;
    const json &objects = prot->protocol[category];
    for (int i = 0; i < size; ++i) {
      if (!objects.is_array() || i >= objects.size() || !objects[i].is_object() ||
          !objects[i].value("searcher", false)) {
        indices.push_back(i);
      }
    }
    return indices;
  }

  static long long quantize(ld value, ld tolerance) {
    return llround(value / tolerance);
  }

  static bool circle_from_points(AlgGeom::Point p1, AlgGeom::Point p2, AlgGeom::Point p3,
                                 AlgGeom::Circle &circle) {
    const ld denominator = 2 * (p1.x * (p2.y - p3.y) +
                                p2.x * (p3.y - p1.y) +
                                p3.x * (p1.y - p2.y));
    const ld scale = max(
      (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y),
      max((p2.x - p3.x) * (p2.x - p3.x) + (p2.y - p3.y) * (p2.y - p3.y),
          (p3.x - p1.x) * (p3.x - p1.x) + (p3.y - p1.y) * (p3.y - p1.y))
    );
    if (scale < AlgGeom::EPS || abs(denominator) / scale < 1e-3L) return false;
    const ld n1 = p1.x * p1.x + p1.y * p1.y;
    const ld n2 = p2.x * p2.x + p2.y * p2.y;
    const ld n3 = p3.x * p3.x + p3.y * p3.y;
    circle.p = AlgGeom::Point(
      (n1 * (p2.y - p3.y) + n2 * (p3.y - p1.y) + n3 * (p1.y - p2.y)) / denominator,
      (n1 * (p3.x - p2.x) + n2 * (p1.x - p3.x) + n3 * (p2.x - p1.x)) / denominator
    );
    circle.radius = AlgGeom::CoreGeometryTools::dist_points(circle.p, p1);
    return isfinite(circle.p.x) && isfinite(circle.p.y) && isfinite(circle.radius);
  }

  static bool line_intersection(AlgGeom::Line l1, AlgGeom::Line l2, AlgGeom::Point &point) {
    const ld norm1 = sqrt(l1.a * l1.a + l1.b * l1.b);
    const ld norm2 = sqrt(l2.a * l2.a + l2.b * l2.b);
    const ld denominator = l1.a * l2.b - l2.a * l1.b;
    if (norm1 < AlgGeom::EPS || norm2 < AlgGeom::EPS ||
        abs(denominator) < AlgGeom::EPS * norm1 * norm2) return false;
    point = AlgGeom::CoreGeometryTools::inter_lines(l1, l2);
    return isfinite(point.x) && isfinite(point.y);
  }

  static bool concurrency_point(const std::vector<GLine> &lines, const GeoProp::Concurrency &property,
                                AlgGeom::Point &intersection, int &first, int &second) {
    const int indices[] = {property.i1, property.i2, property.i3};
    for (int i = 0; i < 3; ++i) {
      for (int j = i + 1; j < 3; ++j) {
        const AlgGeom::Line l1 = convert_gline(lines[indices[i]]);
        const AlgGeom::Line l2 = convert_gline(lines[indices[j]]);
        const ld norm1 = sqrt(l1.a * l1.a + l1.b * l1.b);
        const ld norm2 = sqrt(l2.a * l2.a + l2.b * l2.b);
        if (norm1 < AlgGeom::EPS || norm2 < AlgGeom::EPS ||
            abs(l1.a * l2.b - l2.a * l1.b) < AlgGeom::EPS * norm1 * norm2) continue;
        intersection = AlgGeom::CoreGeometryTools::inter_lines(l1, l2);
        first = indices[i];
        second = indices[j];
        return true;
      }
    }
    return false;
  }

  static GeoProp::GeoProperties search(Protocol *prot) {
    GeoProp::GeoProperties proper;

    // Colinearity searcher
    GeometryVisual machine(0);
    machine.protocol = *prot;
    machine.rebuild();

    const std::vector<int> point_indices = searchable_indices(prot, "Point", machine.points.size());
    const std::vector<int> line_indices = searchable_indices(prot, "Line", machine.lines.size());

    std::vector<std::tuple<long long, long long, long long, int, int>> collinear_candidates;
    collinear_candidates.reserve(point_indices.size() * (point_indices.size() - 1) / 2);
    for (int a = 0; a < point_indices.size(); ++a) {
      for (int b = a + 1; b < point_indices.size(); ++b) {
        const int i = point_indices[a];
        const int j = point_indices[b];
        const AlgGeom::Point p1 = convert_gpoint(machine.points[i]);
        const AlgGeom::Point p2 = convert_gpoint(machine.points[j]);
        ld line_a = p1.y - p2.y;
        ld line_b = p2.x - p1.x;
        ld line_c = p1.x * p2.y - p2.x * p1.y;
        const ld norm = sqrt(line_a * line_a + line_b * line_b);
        if (norm < AlgGeom::EPS) continue;
        line_a /= norm;
        line_b /= norm;
        line_c /= norm;
        if (line_a < 0 || (abs(line_a) < AlgGeom::EPS && line_b < 0)) {
          line_a = -line_a;
          line_b = -line_b;
          line_c = -line_c;
        }
        collinear_candidates.emplace_back(
          quantize(line_a, 1e-6L), quantize(line_b, 1e-6L), quantize(line_c, 1e-3L), i, j
        );
      }
    }
    std::sort(collinear_candidates.begin(), collinear_candidates.end());
    for (size_t start = 0; start < collinear_candidates.size();) {
      size_t end = start + 1;
      while (end < collinear_candidates.size() &&
             std::get<0>(collinear_candidates[end]) == std::get<0>(collinear_candidates[start]) &&
             std::get<1>(collinear_candidates[end]) == std::get<1>(collinear_candidates[start]) &&
             std::get<2>(collinear_candidates[end]) == std::get<2>(collinear_candidates[start])) ++end;
      std::set<int> group;
      for (size_t i = start; i < end; ++i) {
        group.insert(std::get<3>(collinear_candidates[i]));
        group.insert(std::get<4>(collinear_candidates[i]));
      }
      if (group.size() >= 3) {
        auto point = group.begin();
        const int i = *point++;
        const int j = *point++;
        const int k = *point;
        if (AlgGeom::CoreGeometryTools::are_colinear(
              convert_gpoint(machine.points[i]), convert_gpoint(machine.points[j]),
              convert_gpoint(machine.points[k]))) {
          proper.col.push_back(GeoProp::Colinearity(i, j, k));
        }
      }
      start = end;
    }

    std::vector<std::tuple<long long, long long, long long, int, int, int>> cyclic_candidates;
    for (int a = 0; a < point_indices.size(); ++a) {
      for (int b = a + 1; b < point_indices.size(); ++b) {
        for (int c = b + 1; c < point_indices.size(); ++c) {
          const int i = point_indices[a];
          const int j = point_indices[b];
          const int k = point_indices[c];
          AlgGeom::Circle circle;
          if (!circle_from_points(convert_gpoint(machine.points[i]), convert_gpoint(machine.points[j]),
                                  convert_gpoint(machine.points[k]), circle)) continue;
          cyclic_candidates.emplace_back(
            quantize(circle.p.x, 1e-1L), quantize(circle.p.y, 1e-1L),
            quantize(circle.radius, 1e-1L), i, j, k
          );
        }
      }
    }
    std::sort(cyclic_candidates.begin(), cyclic_candidates.end());
    for (size_t start = 0; start < cyclic_candidates.size();) {
      size_t end = start + 1;
      while (end < cyclic_candidates.size() &&
             std::get<0>(cyclic_candidates[end]) == std::get<0>(cyclic_candidates[start]) &&
             std::get<1>(cyclic_candidates[end]) == std::get<1>(cyclic_candidates[start]) &&
             std::get<2>(cyclic_candidates[end]) == std::get<2>(cyclic_candidates[start])) ++end;
      std::set<int> group;
      for (size_t i = start; i < end; ++i) {
        group.insert(std::get<3>(cyclic_candidates[i]));
        group.insert(std::get<4>(cyclic_candidates[i]));
        group.insert(std::get<5>(cyclic_candidates[i]));
      }
      if (group.size() >= 4) {
        auto point = group.begin();
        const int i = *point++;
        const int j = *point++;
        const int k = *point++;
        const int h = *point;
        if (AlgGeom::CoreGeometryTools::are_cyclic(
              convert_gpoint(machine.points[i]), convert_gpoint(machine.points[j]),
              convert_gpoint(machine.points[k]), convert_gpoint(machine.points[h]))) {
          proper.cyc.push_back(GeoProp::Cyclic(i, j, k, h));
        }
      }
      start = end;
    }

    std::vector<std::tuple<long long, long long, int, int>> concurrent_candidates;
    concurrent_candidates.reserve(line_indices.size() * (line_indices.size() - 1) / 2);
    for (int a = 0; a < line_indices.size(); ++a) {
      for (int b = a + 1; b < line_indices.size(); ++b) {
        const int i = line_indices[a];
        const int j = line_indices[b];
        AlgGeom::Point intersection;
        if (!line_intersection(convert_gline(machine.lines[i]), convert_gline(machine.lines[j]),
                               intersection)) continue;
        concurrent_candidates.emplace_back(
          quantize(intersection.x, 1e-3L), quantize(intersection.y, 1e-3L), i, j
        );
      }
    }
    std::sort(concurrent_candidates.begin(), concurrent_candidates.end());
    for (size_t start = 0; start < concurrent_candidates.size();) {
      size_t end = start + 1;
      while (end < concurrent_candidates.size() &&
             std::get<0>(concurrent_candidates[end]) == std::get<0>(concurrent_candidates[start]) &&
             std::get<1>(concurrent_candidates[end]) == std::get<1>(concurrent_candidates[start])) ++end;
      std::set<int> group;
      for (size_t i = start; i < end; ++i) {
        group.insert(std::get<2>(concurrent_candidates[i]));
        group.insert(std::get<3>(concurrent_candidates[i]));
      }
      if (group.size() >= 3) {
        auto line = group.begin();
        const int i = *line++;
        const int j = *line++;
        const int k = *line;
        if (AlgGeom::CoreGeometryTools::are_concurrent(
              convert_gline(machine.lines[i]), convert_gline(machine.lines[j]),
              convert_gline(machine.lines[k]))) {
          proper.conc.push_back(GeoProp::Concurrency(i, j, k));
        }
      }
      start = end;
    }

    // Colinearity
    int cnt = 0;
    for (GeoProp::Colinearity c : proper.col) {
      // cout << "new property " << c.i1 << " " << c.i2 << endl;
      bool exists = false;
      const json &lines = prot->protocol["Line"];
      if (lines.is_array()) {
        for (const json &line : lines) {
          if (line.is_object() && line.value("func", "") == "newLine" &&
              line.value("version", -1) == 2 && line["args"] == json({c.i1, c.i2})) {
            exists = true;
            break;
          }
        }
      }
      if (!exists) {
        const int index = machine.lines.size() + cnt;
        prot->new_line(index, c.i1, c.i2, 2);
        prot->protocol["Line"][index]["searcher"] = true;
        ++cnt;
      }
    }

    // Cyclicity
    cnt = 0;
    for (GeoProp::Cyclic c : proper.cyc) {
      // cout << "new property " << c.i1 << " " << c.i2 << endl;
      bool exists = false;
      const json &circles = prot->protocol["Circle"];
      if (circles.is_array()) {
        for (const json &circle : circles) {
          if (circle.is_object() && circle.value("func", "") == "circumcircle" &&
              circle["args"] == json({c.i1, c.i2, c.i3})) {
            exists = true;
            break;
          }
        }
      }
      if (!exists) {
        const int index = machine.circles.size() + cnt;
        prot->new_circumcircle(index, c.i1, c.i2, c.i3);
        prot->protocol["Circle"][index]["searcher"] = true;
        // prot->new_line(machine.lines.size() + cnt, c.i1, c.i2, 2);
        ++cnt;
      }
    }

    std::vector<AlgGeom::Point> concurrency_markers;
    cnt = 0;
    for (const GeoProp::Concurrency &c : proper.conc) {
      AlgGeom::Point intersection;
      int first = -1;
      int second = -1;
      if (!concurrency_point(machine.lines, c, intersection, first, second)) continue;
      bool exists = false;
      for (const GPoint &point : machine.points) {
        if (AlgGeom::CoreGeometryTools::points_are_equal(convert_gpoint(point), intersection)) {
          exists = true;
          break;
        }
      }
      for (const AlgGeom::Point &point : concurrency_markers) {
        if (AlgGeom::CoreGeometryTools::points_are_equal(point, intersection)) {
          exists = true;
          break;
        }
      }
      if (!exists) {
        const int index = machine.points.size() + cnt;
        prot->new_inter_ll(index, first, second);
        prot->protocol["Point"][index]["searcher"] = true;
        concurrency_markers.push_back(intersection);
        ++cnt;
      }
    }

    return proper;
  };

  static GeoProp::GeoProperties refresh(Protocol *prot) {
    prot->delete_searcher_objects();
    return search(prot);
  }
};
