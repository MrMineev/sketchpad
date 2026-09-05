#include "geometry.h"
#include "alg.h"
#include "../../gui_tools/src/Gui/Gui.hpp"
#include "../parser/parse.h"

#include "../geo_genie/searcher.h"
#include "../geo_genie/property.h"

#include <string>

typedef long double ld;

using json = nlohmann::json; 

const long double EPS = 10;

const int CIRCLE_ASCII_LOC = 48;

void GeometryVisual::save_configuration(std::string &filepath) {
  std::ofstream file(filepath);
  std::string s = protocol.get_string_format();
  file << s << std::endl;
}

void GeometryVisual::load_configuration(std::string &filepath) {
  protocol.load_data(filepath);
  this->rebuild();
}

void GeometryVisual::rebuild() {
  this->lines.clear();
  this->circles.clear();
  this->points.clear();
  this->conics.clear();
  this->cubics.clear();

  for (auto &obj : protocol.get_order()) {
    string return_type = obj.first;
    int index = obj.second;
    // std::cout << "THIS IS " << return_type << " " << index << std::endl;

    json information_command = protocol.get_info(return_type, index);

    std::string command_type = information_command["func"].template get<std::string>();

    if (command_type == "newPoint") {
      ld px = information_command["location"][0];
      ld py = information_command["location"][1];
      this->points.push_back(GPoint(px, py));
    } else if (command_type == "newPointOnLine") {
      ld line_index = information_command["args"][0];
      ld point_ratio = information_command["args"][1];
      AlgGeom::Point p = AlgGeom::CoreGeometryTools::get_point_on_line_via_ratio(
        AlgGeom::Point(
          this->lines[line_index].x1,
          this->lines[line_index].y1
        ),
        AlgGeom::Point(
          this->lines[line_index].x2,
          this->lines[line_index].y2
        ),
        point_ratio
      );
      this->points.push_back(GPoint(p.x, p.y));
    } else if (command_type == "newLine") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      int p3 = information_command["version"];
      this->lines.push_back(
        GLine(
          this->points[p1].x_pos, this->points[p1].y_pos,
          this->points[p2].x_pos, this->points[p2].y_pos,
          p3
        )
      );
    } else if (command_type == "newCircle") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      this->circles.push_back(
        GCircle(
          this->points[p1].x_pos, this->points[p1].y_pos,
          AlgGeom::CoreGeometryTools::dist_points(
            AlgGeom::Point(this->points[p1].x_pos, this->points[p1].y_pos),
            AlgGeom::Point(this->points[p2].x_pos, this->points[p2].y_pos)
          )
        )
      );
    } else if (command_type == "midpoint") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      AlgGeom::Point new_loc = AlgGeom::CoreGeometryTools::midpoint(
        AlgGeom::Point(this->points[p1].x_pos, this->points[p1].y_pos),
        AlgGeom::Point(this->points[p2].x_pos, this->points[p2].y_pos)
      );
      this->points.push_back(GPoint(new_loc.x, new_loc.y));
    } else if (command_type == "circumcircle") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      int p3 = information_command["args"][2];
      AlgGeom::Line l1 = AlgGeom::CoreGeometryTools::perp_bisector(
        AlgGeom::Point(this->points[p1].x_pos, this->points[p1].y_pos),
        AlgGeom::Point(this->points[p2].x_pos, this->points[p2].y_pos)
      );
      AlgGeom::Line l2 = AlgGeom::CoreGeometryTools::perp_bisector(
        AlgGeom::Point(this->points[p2].x_pos, this->points[p2].y_pos),
        AlgGeom::Point(this->points[p3].x_pos, this->points[p3].y_pos)
      );
      AlgGeom::Point p = AlgGeom::CoreGeometryTools::inter_lines(l1, l2);
      this->circles.push_back(GCircle(
        p.x, p.y, AlgGeom::CoreGeometryTools::dist_points(
          p, AlgGeom::Point(this->points[p1].x_pos, this->points[p1].y_pos)
        )
      ));
    } else if (command_type == "interLL") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      AlgGeom::Line l1 = AlgGeom::Line(
        AlgGeom::Point(this->lines[p1].x1, this->lines[p1].y1),
        AlgGeom::Point(this->lines[p1].x2, this->lines[p1].y2)
      );
      AlgGeom::Line l2 = AlgGeom::Line(
        AlgGeom::Point(this->lines[p2].x1, this->lines[p2].y1),
        AlgGeom::Point(this->lines[p2].x2, this->lines[p2].y2)
      );
      AlgGeom::Point new_loc = AlgGeom::CoreGeometryTools::inter_lines(l1, l2);
      this->points.push_back(GPoint(new_loc.x, new_loc.y));
    } else if (command_type == "interLC") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      AlgGeom::Line l1 = AlgGeom::Line(
        AlgGeom::Point(this->lines[p1].x1, this->lines[p1].y1),
        AlgGeom::Point(this->lines[p1].x2, this->lines[p1].y2)
      );
      AlgGeom::Circle c2 = AlgGeom::Circle(
        AlgGeom::Point(this->circles[p2].x_pos, this->circles[p2].y_pos),
        this->circles[p2].radius
      );
      pair<AlgGeom::Point, AlgGeom::Point> new_loc = AlgGeom::CoreGeometryTools::inter_lc(l1, c2);
      this->points.push_back(
        GPoint(new_loc.first.x, new_loc.first.y)
      );
      this->points.push_back(
        GPoint(new_loc.second.x, new_loc.second.y)
      );
    } else if (command_type == "perpNormal") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      AlgGeom::Line l = AlgGeom::CoreGeometryTools::perp_normal(
        AlgGeom::Point(this->points[p1].x_pos, this->points[p1].y_pos),
        AlgGeom::Line(
          AlgGeom::Point(this->lines[p2].x1, this->lines[p2].y1),
          AlgGeom::Point(this->lines[p2].x2, this->lines[p2].y2)
        )
      );
      pair<pair<ld, ld>, pair<ld, ld>> gen = l.gen_points();
      this->lines.push_back(GLine(
        gen.first.first, gen.first.second, gen.second.first, gen.second.second, 1
      ));
    } else if (command_type == "parallel") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      AlgGeom::Line l = AlgGeom::CoreGeometryTools::parallel_line(
        AlgGeom::Point(this->points[p1].x_pos, this->points[p1].y_pos),
        AlgGeom::Line(
          AlgGeom::Point(this->lines[p2].x1, this->lines[p2].y1),
          AlgGeom::Point(this->lines[p2].x2, this->lines[p2].y2)
        )
      );
      pair<pair<ld, ld>, pair<ld, ld>> gen = l.gen_points();
      this->lines.push_back(GLine(
        gen.first.first, gen.first.second, gen.second.first, gen.second.second, 1
      ));
    } else if (command_type == "new_incenter") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      int p3 = information_command["args"][2];
      AlgGeom::Point new_loc = AlgGeom::CoreGeometryTools::incenter(
        AlgGeom::Point(this->points[p1].x_pos, this->points[p1].y_pos),
        AlgGeom::Point(this->points[p2].x_pos, this->points[p2].y_pos),
        AlgGeom::Point(this->points[p3].x_pos, this->points[p3].y_pos)
      );
      // this->points[p] = GPoint(new_loc.x, new_loc.y);
      this->points.push_back(GPoint(new_loc.x, new_loc.y));
    } else if (command_type == "new_excenter") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      int p3 = information_command["args"][2];
      AlgGeom::Point new_loc = AlgGeom::CoreGeometryTools::excenter(
        AlgGeom::Point(this->points[p1].x_pos, this->points[p1].y_pos),
        AlgGeom::Point(this->points[p2].x_pos, this->points[p2].y_pos),
        AlgGeom::Point(this->points[p3].x_pos, this->points[p3].y_pos)
      );
      // this->points[p] = GPoint(new_loc.x, new_loc.y);
      this->points.push_back(GPoint(new_loc.x, new_loc.y));
    } else if (command_type == "newConic") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      int p3 = information_command["args"][2];
      int p4 = information_command["args"][3];
      int p5 = information_command["args"][4];
      AlgGeom::Conic c = AlgGeom::CoreGeometryTools::fitConicThrough5(
        AlgGeom::Point(this->points[p1].x_pos, this->points[p1].y_pos),
        AlgGeom::Point(this->points[p2].x_pos, this->points[p2].y_pos),
        AlgGeom::Point(this->points[p3].x_pos, this->points[p3].y_pos),
        AlgGeom::Point(this->points[p4].x_pos, this->points[p4].y_pos),
        AlgGeom::Point(this->points[p5].x_pos, this->points[p5].y_pos)
      );
      this->conics.push_back(GConic(c.a, c.b, c.c, c.d, c.e, c.f));
    } else if (command_type == "newCubic") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      int p3 = information_command["args"][2];
      int p4 = information_command["args"][3];
      int p5 = information_command["args"][4];
      int p6 = information_command["args"][5];
      int p7 = information_command["args"][6];
      int p8 = information_command["args"][7];
      int p9 = information_command["args"][8];
      AlgGeom::Cubic c = AlgGeom::CoreGeometryTools::fitCubicThrough9(
        AlgGeom::Point(this->points[p1].x_pos, this->points[p1].y_pos),
        AlgGeom::Point(this->points[p2].x_pos, this->points[p2].y_pos),
        AlgGeom::Point(this->points[p3].x_pos, this->points[p3].y_pos),
        AlgGeom::Point(this->points[p4].x_pos, this->points[p4].y_pos),
        AlgGeom::Point(this->points[p5].x_pos, this->points[p5].y_pos),
        AlgGeom::Point(this->points[p6].x_pos, this->points[p6].y_pos),
        AlgGeom::Point(this->points[p7].x_pos, this->points[p7].y_pos),
        AlgGeom::Point(this->points[p8].x_pos, this->points[p8].y_pos),
        AlgGeom::Point(this->points[p9].x_pos, this->points[p9].y_pos)
      );
      this->cubics.push_back(GCubic(c.a, c.b, c.c, c.d, c.e, c.f, c.g, c.h, c.i, c.j));
    } else if (command_type == "newAngleBisector") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      int p3 = information_command["args"][2];
      AlgGeom::Line l = AlgGeom::CoreGeometryTools::angle_bisector(
        AlgGeom::Point(this->points[0].x_pos, this->points[0].y_pos),
        AlgGeom::Point(this->points[1].x_pos, this->points[1].y_pos),
        AlgGeom::Point(this->points[2].x_pos, this->points[2].y_pos)
      );
      pair<pair<ld, ld>, pair<ld, ld>> gen = l.gen_points();
      this->lines.push_back(GLine(
        gen.first.first, gen.first.second, gen.second.first, gen.second.second,
        1
      ));
    } else if (command_type == "newReflectLineOverLine") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      AlgGeom::Line l1 = AlgGeom::Line(
        AlgGeom::Point(this->lines[p1].x1, this->lines[p1].y1),
        AlgGeom::Point(this->lines[p1].x2, this->lines[p1].y2)
      );
      AlgGeom::Line l2 = AlgGeom::Line(
        AlgGeom::Point(this->lines[p2].x1, this->lines[p2].y1),
        AlgGeom::Point(this->lines[p2].x2, this->lines[p2].y2)
      );

      AlgGeom::Line l = AlgGeom::CoreGeometryTools::reflect_line_over_line(l1, l2);
      pair<pair<ld, ld>, pair<ld, ld>> gen = l.gen_points();
      this->lines.push_back(GLine(
        gen.first.first, gen.first.second, gen.second.first, gen.second.second, 1
      ));
    } else if (command_type == "newIsogonalConjugate") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      int p3 = information_command["args"][2];
      int p4 = information_command["args"][3];
      AlgGeom::Point new_loc = AlgGeom::CoreGeometryTools::isogonal_conjugate(
        AlgGeom::Point(this->points[p1].x_pos, this->points[p1].y_pos),
        AlgGeom::Point(this->points[p2].x_pos, this->points[p2].y_pos),
        AlgGeom::Point(this->points[p3].x_pos, this->points[p3].y_pos),
        AlgGeom::Point(this->points[p4].x_pos, this->points[p4].y_pos)
      );
      // this->points[p] = GPoint(new_loc.x, new_loc.y);
      this->points.push_back(GPoint(new_loc.x, new_loc.y));
    } else if (command_type == "newReflectPointOverLine") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      AlgGeom::Point p = AlgGeom::CoreGeometryTools::reflect_point_over_line(
        AlgGeom::Point(this->points[p1].x_pos, this->points[p1].y_pos),
        AlgGeom::Line(
          AlgGeom::Point(this->lines[p2].x1, this->lines[p2].y1),
          AlgGeom::Point(this->lines[p2].x2, this->lines[p2].y2)
        )
      );
      this->points.push_back(GPoint(
        p.x, p.y
      ));
    } else {
      std::cerr << "[ERROR]: unknown command type => " << command_type << std::endl;
    }
  }

  for (int i = 0; i < this->points.size(); ++i) this->points[i].index = i;
  for (int i = 0; i < this->lines.size(); ++i) this->lines[i].index = i;
  for (int i = 0; i < this->circles.size(); ++i) this->circles[i].index = i;
}

void GeometryVisual::delete_object(std::string type, int index) {
  this->protocol.delete_obj(type, index);
  this->live_stack.clear();
  this->live_stack_lines.clear();
  this->live_stack_circles.clear();
  this->selected_point = -1;
  this->selected_line = -1;
  this->selected_circle = -1;
  this->follower = -1;
  this->isDragging = false;
  this->refresh_geo_genie_on_release = false;
  this->rebuild();
}

void GeometryVisual::delete_point(int index) {
  if (index < 0 || index >= this->points.size()) return;
  this->delete_object("Point", index);
}

void GeometryVisual::delete_line(int index) {
  if (index < 0 || index >= this->lines.size()) return;
  this->delete_object("Line", index);
}

void GeometryVisual::delete_circle(int index) {
  if (index < 0 || index >= this->circles.size()) return;
  this->delete_object("Circle", index);
}

void GeometryVisual::hide_geo_genie() {
  this->protocol.delete_searcher_objects();
  this->live_stack.clear();
  this->live_stack_lines.clear();
  this->live_stack_circles.clear();
  this->selected_point = -1;
  this->selected_line = -1;
  this->selected_circle = -1;
  this->follower = -1;
  this->isDragging = false;
  this->refresh_geo_genie_on_release = false;
  this->rebuild();
}

int GeometryVisual::take_inversion_request() {
  const int request = this->inversion_circle_request;
  this->inversion_circle_request = -1;
  return request;
}

void GeometryVisual::build_inversion(const GeometryVisual &source, int inversion_circle) {
  this->points.clear();
  this->lines.clear();
  this->circles.clear();
  this->conics.clear();
  this->cubics.clear();
  if (inversion_circle < 0 || inversion_circle >= source.circles.size()) return;

  const GCircle &base = source.circles[inversion_circle];
  const AlgGeom::Point center(base.x_pos, base.y_pos);
  const ld radius_squared = static_cast<ld>(base.radius) * base.radius;
  if (radius_squared < AlgGeom::EPS) return;

  auto is_searcher = [&](const std::string &category, int index) {
    if (!source.protocol.protocol.contains(category)) return false;
    const json &objects = source.protocol.protocol[category];
    return objects.is_array() && index >= 0 && index < objects.size() && objects[index].is_object() &&
           objects[index].value("searcher", false);
  };
  auto add_line = [&](AlgGeom::Point point, AlgGeom::Point direction) {
    const ld length = sqrt(direction.x * direction.x + direction.y * direction.y);
    if (length < AlgGeom::EPS) return;
    direction.x /= length;
    direction.y /= length;
    GLine line(
      point.x - direction.x * 5000, point.y - direction.y * 5000,
      point.x + direction.x * 5000, point.y + direction.y * 5000, 0
    );
    line.line_type = 1;
    line.index = this->lines.size();
    this->lines.push_back(line);
  };

  for (int i = 0; i < source.points.size(); ++i) {
    if (is_searcher("Point", i)) continue;
    const AlgGeom::Point point(source.points[i].x_pos, source.points[i].y_pos);
    if (AlgGeom::CoreGeometryTools::dist_points(point, center) < AlgGeom::EPS) continue;
    const AlgGeom::Point inverted = AlgGeom::CoreGeometryTools::inversion_point(
      point, AlgGeom::Circle(center, base.radius)
    );
    GPoint result(inverted.x, inverted.y);
    result.index = this->points.size();
    this->points.push_back(result);
  }

  for (int i = 0; i < source.lines.size(); ++i) {
    if (source.lines[i].line_type == 0 || is_searcher("Line", i)) continue;
    const AlgGeom::Line line(
      AlgGeom::Point(source.lines[i].x1, source.lines[i].y1),
      AlgGeom::Point(source.lines[i].x2, source.lines[i].y2)
    );
    const ld norm = sqrt(line.a * line.a + line.b * line.b);
    if (norm < AlgGeom::EPS) continue;
    const ld distance = abs(line.a * center.x + line.b * center.y + line.c) / norm;
    if (distance < AlgGeom::EPS) {
      add_line(center, AlgGeom::Point(-line.b, line.a));
    } else {
      const AlgGeom::Point closest = AlgGeom::CoreGeometryTools::project_point_to_line(center, line);
      const AlgGeom::Point image = AlgGeom::CoreGeometryTools::inversion_point(
        closest, AlgGeom::Circle(center, base.radius)
      );
      const AlgGeom::Point inverted_center = AlgGeom::CoreGeometryTools::midpoint(center, image);
      this->circles.push_back(GCircle(
        inverted_center.x, inverted_center.y,
        AlgGeom::CoreGeometryTools::dist_points(inverted_center, center)
      ));
      this->circles.back().index = this->circles.size() - 1;
    }
  }

  for (int i = 0; i < source.circles.size(); ++i) {
    if (is_searcher("Circle", i)) continue;
    const AlgGeom::Point offset(
      source.circles[i].x_pos - center.x,
      source.circles[i].y_pos - center.y
    );
    const ld source_radius = source.circles[i].radius;
    const ld denominator = offset.x * offset.x + offset.y * offset.y - source_radius * source_radius;
    if (abs(denominator) < AlgGeom::EPS) {
      const ld offset_squared = offset.x * offset.x + offset.y * offset.y;
      if (offset_squared < AlgGeom::EPS) continue;
      const AlgGeom::Point point(
        center.x + offset.x * radius_squared / (2 * offset_squared),
        center.y + offset.y * radius_squared / (2 * offset_squared)
      );
      add_line(point, AlgGeom::Point(-offset.y, offset.x));
    } else {
      const ld factor = radius_squared / denominator;
      this->circles.push_back(GCircle(
        center.x + factor * offset.x,
        center.y + factor * offset.y,
        abs(factor) * source_radius
      ));
      this->circles.back().index = this->circles.size() - 1;
    }
  }
}

pair<pair<int, pair<int, int>>, GPoint> GeometryVisual::point_searcher(GPoint p) {
  for (int i = 0; i < this->points.size(); i++) {
    if (AlgGeom::CoreGeometryTools::dist_points(
      AlgGeom::Point(p.x_pos, p.y_pos),
      AlgGeom::Point(this->points[i].x_pos, this->points[i].y_pos)
    ) <= EPS) {
      return make_pair(make_pair(i, make_pair(-1, -1)), this->points[i]);
    }
  }
  for (int i = 0; i < this->lines.size(); i++) {
    // TODO DIFFERENTIATE BETWEEN LINE AND SEGMENT
    const ld dx = this->lines[i].x2 - this->lines[i].x1;
    const ld dy = this->lines[i].y2 - this->lines[i].y1;
    const ld length_squared = dx * dx + dy * dy;
    if (length_squared == 0) continue;
    const ld projection = ((p.x_pos - this->lines[i].x1) * dx +
                           (p.y_pos - this->lines[i].y1) * dy) / length_squared;
    if (this->lines[i].line_type == 0 && (projection < 0 || projection > 1)) continue;
    if (AlgGeom::CoreGeometryTools::dist_point_to_line(
      AlgGeom::Point(p.x_pos, p.y_pos),
      AlgGeom::Line(
        AlgGeom::Point(this->lines[i].x1, this->lines[i].y1),
        AlgGeom::Point(this->lines[i].x2, this->lines[i].y2)
      )
    ) <= EPS) {
      AlgGeom::Point new_loc = AlgGeom::CoreGeometryTools::project_point_to_line(
        AlgGeom::Point(p.x_pos, p.y_pos),
        AlgGeom::Line(
          AlgGeom::Point(this->lines[i].x1, this->lines[i].y1),
          AlgGeom::Point(this->lines[i].x2, this->lines[i].y2)
        )
      );
      return make_pair(make_pair(-1, make_pair(i, -1)), GPoint(new_loc.x, new_loc.y));
    }
  }
  for (int i = 0; i < this->circles.size(); i++) {
    ld dist = abs(this->circles[i].radius - AlgGeom::CoreGeometryTools::dist_points(
      AlgGeom::Point(p.x_pos, p.y_pos),
      AlgGeom::Point(
        this->circles[i].x_pos, this->circles[i].y_pos
      )
    ));
    if (dist <= EPS) {
      AlgGeom::Point new_loc = AlgGeom::CoreGeometryTools::project_point_to_circle(
        AlgGeom::Point(p.x_pos, p.y_pos),
        AlgGeom::Circle(
          AlgGeom::Point(this->circles[i].x_pos, this->circles[i].y_pos),
          this->circles[i].radius
        )
      );
      return make_pair(make_pair(-1, make_pair(-1, i)), GPoint(new_loc.x, new_loc.y));
    }
  }
  return make_pair(make_pair(-1, make_pair(-1, -1)), p);
}

void GeometryVisual::handleEvent(const sf::Event& event, sf::RenderWindow& window, gui::Menu& menu) {
  if (event.type == sf::Event::MouseButtonPressed) {
    const float close_x = window.getSize().x - 44.f;
    if (event.mouseButton.button == sf::Mouse::Left && this->protocol.has_searcher_objects() &&
        event.mouseButton.x >= close_x && event.mouseButton.x <= close_x + 32 &&
        event.mouseButton.y >= 48 && event.mouseButton.y <= 80) {
      this->hide_geo_genie();
      return;
    }
    if (event.mouseButton.x <= this->X_MENU_BORDER) {
      this->selected_point = -1;
      this->selected_line = -1;
      this->selected_circle = -1;
      this->isDragging = false;
      this->refresh_geo_genie_on_release = false;
      return;
    }

    GPoint _p(event.mouseButton.x, event.mouseButton.y);
    auto [_indexes, p] = this->point_searcher(_p);
    auto [index_search, _object_search] = _indexes;
    auto [line_search, circle_search] = _object_search;

    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 0) {
      this->selected_point = index_search;
      this->selected_line = index_search == -1 ? line_search : -1;
      this->selected_circle = index_search == -1 && line_search == -1 ? circle_search : -1;
      if (index_search != -1 &&
          (this->protocol.is_point_def_by_func(index_search, "newPoint") ||
           this->protocol.is_point_def_by_func(index_search, "newPointOnLine"))) {
        isDragging = true;
        follower = index_search;
        this->refresh_geo_genie_on_release = this->protocol.has_searcher_objects();
        if (this->refresh_geo_genie_on_release) {
          this->protocol.delete_searcher_objects();
          this->rebuild();
        }
      } else {
        isDragging = false;
        follower = -1;
        this->refresh_geo_genie_on_release = false;
      }
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 1) {
      if (index_search == -1) {
        this->points.push_back(p);
        if (line_search != -1) {
          ld ratio_value = AlgGeom::CoreGeometryTools::get_point_on_line_ratio(
            AlgGeom::Point(
              this->lines[line_search].x1,
              this->lines[line_search].y1
            ),
            AlgGeom::Point(
              this->lines[line_search].x2,
              this->lines[line_search].y2
            ),
            AlgGeom::Point(p.x_pos, p.y_pos)
          );
          protocol.new_point_on_line(this->points.size() - 1, line_search, ratio_value);
        } else {
          protocol.new_point(this->points.size() - 1, p.x_pos, p.y_pos);
        }
      }
    }
    
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 2) {
      p.index = index_search;
      if (index_search == -1) {
        p.index = this->points.size();
        this->points.push_back(p);
        // protocol += new_point(this->points.size() - 1, p.x_pos, p.y_pos);
        protocol.new_point(this->points.size() - 1, p.x_pos, p.y_pos);
      }
      this->live_stack.push_back(p);
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 3) {
      p.index = index_search;
      if (index_search == -1) {
        p.index = this->points.size();
        this->points.push_back(p);
        // protocol += new_point(this->points.size() - 1, p.x_pos, p.y_pos);
        protocol.new_point(this->points.size() - 1, p.x_pos, p.y_pos);
      }
      this->live_stack.push_back(p);
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 4) {
      p.index = index_search;
      if (index_search == -1) {
        p.index = this->points.size();
        this->points.push_back(p);
        // protocol += new_point(this->points.size() - 1, p.x_pos, p.y_pos);
        protocol.new_point(this->points.size() - 1, p.x_pos, p.y_pos);
      }
      this->live_stack.push_back(p);
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 5) {
      p.index = index_search;
      if (index_search == -1) {
        p.index = this->points.size();
        this->points.push_back(p);
        // protocol += new_point(this->points.size() - 1, p.x_pos, p.y_pos);
        protocol.new_point(this->points.size() - 1, p.x_pos, p.y_pos);
      }
      this->live_stack.push_back(p);
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 6) {
      p.index = index_search;
      if (index_search == -1) {
        p.index = this->points.size();
        this->points.push_back(p);
        // protocol += new_point(this->points.size() - 1, p.x_pos, p.y_pos);
        protocol.new_point(this->points.size() - 1, p.x_pos, p.y_pos);
      }
      this->live_stack.push_back(p);
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 7) {
      this->live_stack_lines.push_back(this->lines[line_search]);
      this->live_stack_lines[this->live_stack_lines.size() - 1].index = line_search;
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 8) {
      if (this->live_stack_lines.size() == 1) {
        p.index = index_search;
        this->live_stack.push_back(p);
      } else {
        if (line_search != -1) {
          this->lines[line_search].index = line_search;
          this->live_stack_lines.push_back(this->lines[line_search]);
        } else {
          p.index = index_search;
          this->live_stack.push_back(p);
        }
      }
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 9 && circle_search != -1) {
      this->inversion_circle_request = circle_search;
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 10) {
      p.index = index_search;
      this->live_stack.push_back(p);
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 11) {
      p.index = index_search;
      this->live_stack.push_back(p);
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 12) {
      if (this->live_stack_lines.size() == 1) {
        this->live_stack_circles.push_back(this->circles[circle_search]);
        this->live_stack_circles[this->live_stack_circles.size() - 1].index = circle_search;
      } else {
        if (line_search != -1) {
          this->lines[line_search].index = line_search;
          this->live_stack_lines.push_back(this->lines[line_search]);
        } else {
          this->live_stack_circles.push_back(this->circles[circle_search]);
          this->live_stack_circles[this->live_stack_circles.size() - 1].index = circle_search;
        }
      }
    }

    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 13) {
      GeoProp::GeoProperties something = GeoGenie::search(&protocol);
      rebuild();
    }

    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 14) {
      if (this->live_stack_lines.size() == 1) {
        p.index = index_search;
        this->live_stack.push_back(p);
      } else {
        if (line_search != -1) {
          this->lines[line_search].index = line_search;
          this->live_stack_lines.push_back(this->lines[line_search]);
        } else {
          p.index = index_search;
          this->live_stack.push_back(p);
        }
      }
    }

    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 15) {
      if (this->live_stack.size() <= 5) {
        p.index = index_search;
        this->live_stack.push_back(p);
      }
    }

    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 16) {
      

      if (this->live_stack.size() <= 9) {
        p.index = index_search;
        this->live_stack.push_back(p);
      }
    }

    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 17) {
      if (this->live_stack.size() <= 3) {
        p.index = index_search;
        this->live_stack.push_back(p);
      }
    }

    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 18) {
      if (this->live_stack_lines.size() <= 2) {
         if (line_search != -1) {
          this->lines[line_search].index = line_search;
          this->live_stack_lines.push_back(this->lines[line_search]);
         }
      }
    }

    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 19) {
      if (this->live_stack.size() <= 4) {
        p.index = index_search;
        this->live_stack.push_back(p);
      }
    }

    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 20) {
      cout << "click!" << endl;
      if (this->live_stack_lines.size() <= 1) {
        if (line_search != -1) {
          cout << "added line!" << endl;
          this->lines[line_search].index = line_search;
          this->live_stack_lines.push_back(this->lines[line_search]);
        } else if (this->live_stack.size() <= 1) {
          cout << "added point!" << endl;
          p.index = index_search;
          this->live_stack.push_back(p);
        }
      }
    }
  }
  if (event.type == sf::Event::MouseButtonReleased) {
    if (event.mouseButton.button == sf::Mouse::Left) {
      isDragging = false;
      if (this->refresh_geo_genie_on_release) {
        GeoGenie::search(&this->protocol);
        this->refresh_geo_genie_on_release = false;
        this->rebuild();
      }
    }
  }

  if (isDragging) {
    sf::Vector2i mousePos = sf::Mouse::getPosition(window);

    if (this->protocol.is_point_def_by_func(follower, "newPointOnLine")) {
      int line_index = this->protocol.get_point_info(follower)["args"][0];
      AlgGeom::Point new_loc = AlgGeom::CoreGeometryTools::project_point_to_line(
        AlgGeom::Point(mousePos.x, mousePos.y),
        AlgGeom::Line(
          AlgGeom::Point(this->lines[line_index].x1, this->lines[line_index].y1),
          AlgGeom::Point(this->lines[line_index].x2, this->lines[line_index].y2)
        )
      );
      ld ratio_value = AlgGeom::CoreGeometryTools::get_point_on_line_ratio(
        AlgGeom::Point(
          this->lines[line_index].x1,
          this->lines[line_index].y1
        ),
        AlgGeom::Point(
          this->lines[line_index].x2,
          this->lines[line_index].y2
        ),
        new_loc
      );
      this->protocol.protocol["Point"][follower]["args"][1] = ratio_value;
    } else {
      this->protocol.edit_position(follower, mousePos.x, mousePos.y);
    }
    // this->points[follower] = GPoint(mousePos.x, mousePos.y);
    // std::cout << "data = " << protocol.get_string_format() << std::endl;

    this->rebuild();
  }
  
  if (this->current_tool == 2 && this->live_stack.size() >= 2) {
    this->lines.push_back(GLine(
      this->live_stack[0].x_pos,
      this->live_stack[0].y_pos,
      this->live_stack[1].x_pos,
      this->live_stack[1].y_pos,
      0
    ));

    protocol.new_line(this->lines.size() - 1, this->live_stack[0].index, this->live_stack[1].index, 0);

    // protocol += GeometryVisual::new_line(this->lines.size() - 1, this->live_stack[0].index, this->live_stack[1].index, true);

    this->live_stack.clear();
    this->live_stack_lines.clear();
    this->live_stack_circles.clear();
  }

  if (this->current_tool == 3 && this->live_stack.size() >= 2) {
    this->circles.push_back(GCircle(
      this->live_stack[0].x_pos,
      this->live_stack[0].y_pos,
      AlgGeom::CoreGeometryTools::dist_points(
        AlgGeom::Point(
          this->live_stack[0].x_pos,
          this->live_stack[0].y_pos
        ),
        AlgGeom::Point(
          this->live_stack[1].x_pos,
          this->live_stack[1].y_pos
        )
      )
    ));

    protocol.new_circle(this->circles.size() - 1, this->live_stack[0].index, this->live_stack[1].index);
    
    // protocol += GeometryVisual::new_circle(this->circles.size() - 1, this->live_stack[0].index, this->live_stack[1].index);

    this->live_stack.clear();
    this->live_stack_lines.clear();
    this->live_stack_circles.clear();
  }

  if (this->current_tool == 4 && this->live_stack.size() >= 2) {
    this->points.push_back(GPoint(
      (this->live_stack[0].x_pos + this->live_stack[1].x_pos) / 2,
      (this->live_stack[0].y_pos + this->live_stack[1].y_pos) / 2
    ));

    protocol.new_midpoint(this->points.size() - 1, this->live_stack[0].index, this->live_stack[1].index);

    // protocol += GeometryVisual::new_midpoint(this->points.size() - 1, this->live_stack[0].index, this->live_stack[1].index);

    this->live_stack.clear();
    this->live_stack_lines.clear();
    this->live_stack_circles.clear();
  }

  if (this->current_tool == 5 && this->live_stack.size() >= 3) {
    AlgGeom::Line lllll(
      AlgGeom::Point(this->live_stack[0].x_pos, this->live_stack[0].y_pos),
      AlgGeom::Point(this->live_stack[1].x_pos, this->live_stack[1].y_pos)
    );
    AlgGeom::Line l1 = AlgGeom::CoreGeometryTools::perp_bisector(
      AlgGeom::Point(this->live_stack[0].x_pos, this->live_stack[0].y_pos),
      AlgGeom::Point(this->live_stack[1].x_pos, this->live_stack[1].y_pos)
    );
    AlgGeom::Line l2 = AlgGeom::CoreGeometryTools::perp_bisector(
      AlgGeom::Point(this->live_stack[1].x_pos, this->live_stack[1].y_pos),
      AlgGeom::Point(this->live_stack[2].x_pos, this->live_stack[2].y_pos)
    );
    AlgGeom::Point p = AlgGeom::CoreGeometryTools::inter_lines(l1, l2);
    this->circles.push_back(GCircle(
      p.x, p.y, AlgGeom::CoreGeometryTools::dist_points(
        p, AlgGeom::Point(this->live_stack[0].x_pos, this->live_stack[0].y_pos)
      )
    ));

    protocol.new_circumcircle(this->circles.size() - 1, this->live_stack[0].index, this->live_stack[1].index, this->live_stack[2].index);

    // protocol += GeometryVisual::new_circumcircle(this->circles.size() - 1, this->live_stack[0].index, this->live_stack[1].index, this->live_stack[2].index);

    this->live_stack.clear();
    this->live_stack_lines.clear();
    this->live_stack_circles.clear();
  }

  if (this->current_tool == 6 && this->live_stack.size() >= 2) {
    this->lines.push_back(GLine(
      this->live_stack[0].x_pos,
      this->live_stack[0].y_pos,
      this->live_stack[1].x_pos,
      this->live_stack[1].y_pos,
      1 
    ));

    protocol.new_line(this->lines.size() - 1, this->live_stack[0].index, this->live_stack[1].index, 1);

    // protocol += GeometryVisual::new_line(this->lines.size() - 1, this->live_stack[0].index, this->live_stack[1].index, false);

    this->live_stack.clear();
    this->live_stack_lines.clear();
    this->live_stack_circles.clear();
  }

  if (this->current_tool == 7 && this->live_stack_lines.size() >= 2) {
    AlgGeom::Point p = AlgGeom::CoreGeometryTools::inter_lines(
      AlgGeom::Line(
        AlgGeom::Point(this->live_stack_lines[0].x1, this->live_stack_lines[0].y1),
        AlgGeom::Point(this->live_stack_lines[0].x2, this->live_stack_lines[0].y2)
      ),
      AlgGeom::Line(
        AlgGeom::Point(this->live_stack_lines[1].x1, this->live_stack_lines[1].y1),
        AlgGeom::Point(this->live_stack_lines[1].x2, this->live_stack_lines[1].y2)
      )
    );
    this->points.push_back(GPoint(p.x, p.y));

    protocol.new_inter_ll(this->points.size() - 1, this->live_stack_lines[0].index, this->live_stack_lines[1].index);

    // protocol += GeometryVisual::new_line_intersection(this->points.size() - 1, this->live_stack_lines[0].index, this->live_stack_lines[1].index);

    this->live_stack.clear();
    this->live_stack_lines.clear();
    this->live_stack_circles.clear();
  }

  if (this->current_tool == 8 && this->live_stack_lines.size() == 1 && this->live_stack.size() == 1) {
    AlgGeom::Line l = AlgGeom::CoreGeometryTools::perp_normal(
      AlgGeom::Point(this->live_stack[0].x_pos, this->live_stack[0].y_pos),
      AlgGeom::Line(
        AlgGeom::Point(this->live_stack_lines[0].x1, this->live_stack_lines[0].y1),
        AlgGeom::Point(this->live_stack_lines[0].x2, this->live_stack_lines[0].y2)
      )
    );
    pair<pair<ld, ld>, pair<ld, ld>> gen = l.gen_points();
    this->lines.push_back(GLine(
      gen.first.first, gen.first.second, gen.second.first, gen.second.second,
      1
    ));

    protocol.new_perp_normal(this->lines.size() - 1, this->live_stack[0].index, this->live_stack_lines[0].index);

    // protocol += GeometryVisual::new_perp_normal(this->lines.size() - 1, this->live_stack[0].index, this->live_stack_lines[0].index);

    this->live_stack.clear();
    this->live_stack_lines.clear();
    this->live_stack_circles.clear();
  }

  if (this->current_tool == 10 && this->live_stack.size() >= 3) {
    AlgGeom::Point p = AlgGeom::CoreGeometryTools::incenter(
      AlgGeom::Point(this->live_stack[0].x_pos, this->live_stack[0].y_pos),
      AlgGeom::Point(this->live_stack[1].x_pos, this->live_stack[1].y_pos),
      AlgGeom::Point(this->live_stack[2].x_pos, this->live_stack[2].y_pos)
    );
    this->points.push_back(
      GPoint(p.x, p.y)
    );

    protocol.new_incenter(this->points.size() - 1, this->live_stack[0].index, this->live_stack[1].index, this->live_stack[2].index);

    // protocol += GeometryVisual::new_incenter(this->points.size() - 1, this->live_stack[0].index, this->live_stack[1].index, this->live_stack[2].index);

    this->live_stack.clear();
    this->live_stack_lines.clear();
    this->live_stack_circles.clear();
  }

  if (this->current_tool == 11 && this->live_stack.size() >= 3) {
    AlgGeom::Point p = AlgGeom::CoreGeometryTools::excenter(
      AlgGeom::Point(this->live_stack[0].x_pos, this->live_stack[0].y_pos),
      AlgGeom::Point(this->live_stack[1].x_pos, this->live_stack[1].y_pos),
      AlgGeom::Point(this->live_stack[2].x_pos, this->live_stack[2].y_pos)
    );
    this->points.push_back(
      GPoint(p.x, p.y)
    );

    protocol.new_excenter(this->points.size() - 1, this->live_stack[0].index, this->live_stack[1].index, this->live_stack[2].index);

    // protocol += GeometryVisual::new_excenter(this->points.size() - 1, this->live_stack[0].index, this->live_stack[1].index, this->live_stack[2].index);

    this->live_stack.clear();
    this->live_stack_lines.clear();
    this->live_stack_circles.clear();
  }

  if (this->current_tool == 12 && this->live_stack_lines.size() >= 1 && this->live_stack_circles.size() >= 1) {
    pair<AlgGeom::Point, AlgGeom::Point> p = AlgGeom::CoreGeometryTools::inter_lc(
      AlgGeom::Line(
        AlgGeom::Point(this->live_stack_lines[0].x1, this->live_stack_lines[0].y1),
        AlgGeom::Point(this->live_stack_lines[0].x2, this->live_stack_lines[0].y2)
      ),
      AlgGeom::Circle(
        AlgGeom::Point(
          this->live_stack_circles[0].x_pos, this->live_stack_circles[0].y_pos
        ),
        this->live_stack_circles[0].radius
      )
    );
    this->points.push_back(
      GPoint(p.first.x, p.first.y)
    );
    this->points.push_back(
      GPoint(p.second.x, p.second.y)
    );

    protocol.new_inter_lc(this->points.size() - 1, this->points.size() - 2, this->live_stack_lines[0].index, this->live_stack_circles[1].index);

    // protocol += GeometryVisual::new_inter_lc(this->points.size() - 1, this->points.size() - 2, this->live_stack_lines[0].index, this->live_stack_circles[1].index);

    this->live_stack.clear();
    this->live_stack_lines.clear();
    this->live_stack_circles.clear();
  }

  if (this->current_tool == 14 && this->live_stack_lines.size() == 1 && this->live_stack.size() == 1) {
    cout << "CREATING NEW" << endl;
    AlgGeom::Line l = AlgGeom::CoreGeometryTools::parallel_line(
      AlgGeom::Point(this->live_stack[0].x_pos, this->live_stack[0].y_pos),
      AlgGeom::Line(
        AlgGeom::Point(this->live_stack_lines[0].x1, this->live_stack_lines[0].y1),
        AlgGeom::Point(this->live_stack_lines[0].x2, this->live_stack_lines[0].y2)
      )
    );
    pair<pair<ld, ld>, pair<ld, ld>> gen = l.gen_points();
    this->lines.push_back(GLine(
      gen.first.first, gen.first.second, gen.second.first, gen.second.second,
      1
    ));

    protocol.new_parallel(this->lines.size() - 1, this->live_stack[0].index, this->live_stack_lines[0].index);

    // protocol += GeometryVisual::new_perp_normal(this->lines.size() - 1, this->live_stack[0].index, this->live_stack_lines[0].index);

    this->live_stack.clear();
    this->live_stack_lines.clear();
    this->live_stack_circles.clear();
  }

  if (this->current_tool == 15 && this->live_stack.size() == 5) {
    cout << "new Conic!" << endl;
    AlgGeom::Conic c = AlgGeom::CoreGeometryTools::fitConicThrough5(
      AlgGeom::Point(this->live_stack[0].x_pos, this->live_stack[0].y_pos),
      AlgGeom::Point(this->live_stack[1].x_pos, this->live_stack[1].y_pos),
      AlgGeom::Point(this->live_stack[2].x_pos, this->live_stack[2].y_pos),
      AlgGeom::Point(this->live_stack[3].x_pos, this->live_stack[3].y_pos),
      AlgGeom::Point(this->live_stack[4].x_pos, this->live_stack[4].y_pos)
    );
    cout << "points " << this->live_stack[0].x_pos << " " << this->live_stack[0].y_pos << endl;
    cout << "points " << this->live_stack[1].x_pos << " " << this->live_stack[1].y_pos << endl;
cout << "points " << this->live_stack[2].x_pos << " " << this->live_stack[2].y_pos << endl;
cout << "points " << this->live_stack[3].x_pos << " " << this->live_stack[3].y_pos << endl;
cout << "points " << this->live_stack[4].x_pos << " " << this->live_stack[4].y_pos << endl;
    cout << "Coefficients: " << c.a << " " << c.b << " " << c.c << " " << c.d << " " << c.e << " " << c.f << endl;

    this->conics.push_back(GConic(
        c.a, c.b, c.c, c.d, c.e, c.f
    ));
    protocol.new_conic(this->conics.size() - 1,
        this->live_stack[0].index,
        this->live_stack[1].index,
        this->live_stack[2].index,
        this->live_stack[3].index,
        this->live_stack[4].index
    );

    this->live_stack.clear();
  }

  if (this->current_tool == 16 && this->live_stack.size() == 9) {
    cout << "new Cubic!" << endl;
    AlgGeom::Cubic c = AlgGeom::CoreGeometryTools::fitCubicThrough9(
      AlgGeom::Point(this->live_stack[0].x_pos, this->live_stack[0].y_pos),
      AlgGeom::Point(this->live_stack[1].x_pos, this->live_stack[1].y_pos),
      AlgGeom::Point(this->live_stack[2].x_pos, this->live_stack[2].y_pos),
      AlgGeom::Point(this->live_stack[3].x_pos, this->live_stack[3].y_pos),
      AlgGeom::Point(this->live_stack[4].x_pos, this->live_stack[4].y_pos),
      AlgGeom::Point(this->live_stack[5].x_pos, this->live_stack[5].y_pos),
      AlgGeom::Point(this->live_stack[6].x_pos, this->live_stack[6].y_pos),
      AlgGeom::Point(this->live_stack[7].x_pos, this->live_stack[7].y_pos),
      AlgGeom::Point(this->live_stack[8].x_pos, this->live_stack[8].y_pos)
    );
    this->cubics.push_back(GCubic(
        c.a, c.b, c.c, c.d, c.e, c.f, c.g, c.h, c.i, c.j
    ));
    protocol.new_cubic(this->cubics.size() - 1,
        this->live_stack[0].index,
        this->live_stack[1].index,
        this->live_stack[2].index,
        this->live_stack[3].index,
        this->live_stack[4].index,
        this->live_stack[5].index,
        this->live_stack[6].index,
        this->live_stack[7].index,
        this->live_stack[8].index
    );
    this->live_stack.clear();
  }

  if (this->current_tool == 17 && this->live_stack.size() == 3) {
    AlgGeom::Line l = AlgGeom::CoreGeometryTools::angle_bisector(
      AlgGeom::Point(this->live_stack[0].x_pos, this->live_stack[0].y_pos),
      AlgGeom::Point(this->live_stack[1].x_pos, this->live_stack[1].y_pos),
      AlgGeom::Point(this->live_stack[2].x_pos, this->live_stack[2].y_pos)
    );
    pair<pair<ld, ld>, pair<ld, ld>> gen = l.gen_points();
    this->lines.push_back(GLine(
      gen.first.first, gen.first.second, gen.second.first, gen.second.second,
      1
    ));

    protocol.new_angle_bisector(
      this->lines.size() - 1,
      this->live_stack[0].index,
      this->live_stack[1].index,
      this->live_stack[2].index
    );

    this->live_stack.clear();
  }

  if (this->current_tool == 18 && this->live_stack_lines.size() == 2) {
    AlgGeom::Line l1(
        AlgGeom::Point(this->live_stack_lines[0].x1, this->live_stack_lines[0].y1),
        AlgGeom::Point(this->live_stack_lines[0].x2, this->live_stack_lines[0].y2)
    );
    AlgGeom::Line l2(
        AlgGeom::Point(this->live_stack_lines[1].x1, this->live_stack_lines[1].y1),
        AlgGeom::Point(this->live_stack_lines[1].x2, this->live_stack_lines[1].y2)
    );
    AlgGeom::Line l = AlgGeom::CoreGeometryTools::reflect_line_over_line(l1, l2);
    pair<pair<ld, ld>, pair<ld, ld>> gen = l.gen_points();
    this->lines.push_back(GLine(
      gen.first.first, gen.first.second, gen.second.first, gen.second.second,
      1
    ));

    protocol.new_reflect_line_over_line(
        this->lines.size() - 1,
        this->live_stack_lines[0].index,
        this->live_stack_lines[1].index
    );

    this->live_stack_lines.clear();
  }

  if (this->current_tool == 19 && this->live_stack.size() == 4) {
    AlgGeom::Point p = AlgGeom::CoreGeometryTools::isogonal_conjugate(
      AlgGeom::Point(this->live_stack[0].x_pos, this->live_stack[0].y_pos),
      AlgGeom::Point(this->live_stack[1].x_pos, this->live_stack[1].y_pos),
      AlgGeom::Point(this->live_stack[2].x_pos, this->live_stack[2].y_pos),
      AlgGeom::Point(this->live_stack[3].x_pos, this->live_stack[3].y_pos)
    );
    this->points.push_back(
      GPoint(p.x, p.y)
    );

    protocol.new_isogonal_conjugate(this->points.size() - 1, this->live_stack[0].index, this->live_stack[1].index, this->live_stack[2].index, this->live_stack[3].index);

    this->live_stack.clear();
  }

  if (this->current_tool == 20 && this->live_stack.size() == 1 && this->live_stack_lines.size() == 1) {
    cout << "Reflecting!" << endl;
    AlgGeom::Point p = AlgGeom::CoreGeometryTools::reflect_point_over_line(
      AlgGeom::Point(this->live_stack[0].x_pos, this->live_stack[0].y_pos),
      AlgGeom::Line(
        AlgGeom::Point(this->live_stack_lines[0].x1, this->live_stack_lines[0].y1),
        AlgGeom::Point(this->live_stack_lines[0].x2, this->live_stack_lines[0].y2)
      )
    );
    this->points.push_back(
      GPoint(p.x, p.y)
    );
    protocol.new_reflect_point_over_line(
      this->points.size() - 1,
      this->live_stack[0].index,
      this->live_stack_lines[0].index
    );

    this->live_stack.clear();
    this->live_stack_lines.clear();
  }

  if (this->current_tool == 21) {
    // std::cout << "data = " << protocol.get_string_format() << std::endl;
    this->protocol.save_data();
  }

  if (event.type == sf::Event::KeyPressed &&
      (event.key.code == sf::Keyboard::Delete || event.key.code == sf::Keyboard::BackSpace) &&
      this->current_tool == 0) {
    if (this->selected_point != -1) {
      this->delete_point(this->selected_point);
    } else if (this->selected_line != -1) {
      this->delete_line(this->selected_line);
    } else if (this->selected_circle != -1) {
      this->delete_circle(this->selected_circle);
    }
  }
}

void GeometryVisual::draw(sf::RenderWindow& window) {
  for (int i = 0; i < this->points.size(); ++i) {
    if (i == this->selected_point) {
      this->points[i].shape.setOutlineColor(sf::Color::Black);
      this->points[i].shape.setOutlineThickness(3);
    } else {
      this->points[i].shape.setOutlineThickness(0);
    }
    this->points[i].draw(window);
  }
  for (int i = 0; i < this->lines.size(); ++i) {
    const sf::Color color = i == this->selected_line
      ? sf::Color::Black
      : (this->lines[i].line_type == 2 ? sf::Color(238, 130, 238) : sf::Color::Blue);
    for (size_t j = 0; j < this->lines[i].line.getVertexCount(); ++j) {
      this->lines[i].line[j].color = color;
    }
    this->lines[i].draw(window);
  }
  for (int i = 0; i < this->circles.size(); ++i) {
    this->circles[i].shape.setOutlineColor(
      i == this->selected_circle ? sf::Color::Black : sf::Color::Green
    );
    this->circles[i].shape.setOutlineThickness(i == this->selected_circle ? 3 : 1);
    this->circles[i].draw(window);
  }
  for (auto& conic : this->conics) {
    conic.draw(window);
  }
  for (auto& cubic : this->cubics) {
    cubic.draw(window);
  }
  if (this->protocol.has_searcher_objects()) {
    const float x = window.getSize().x - 44.f;
    sf::RectangleShape close_button(sf::Vector2f(32, 32));
    close_button.setPosition(x, 48);
    close_button.setFillColor(sf::Color::White);
    close_button.setOutlineColor(sf::Color::Black);
    close_button.setOutlineThickness(1);
    window.draw(close_button);

    sf::VertexArray cross(sf::Lines, 4);
    cross[0] = sf::Vertex(sf::Vector2f(x + 8, 56), sf::Color::Black);
    cross[1] = sf::Vertex(sf::Vector2f(x + 24, 72), sf::Color::Black);
    cross[2] = sf::Vertex(sf::Vector2f(x + 24, 56), sf::Color::Black);
    cross[3] = sf::Vertex(sf::Vector2f(x + 8, 72), sf::Color::Black);
    window.draw(cross);
  }
}










