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
    if (event.mouseButton.x <= this->X_MENU_BORDER) return;

    GPoint _p(event.mouseButton.x, event.mouseButton.y);
    auto [_indexes, p] = this->point_searcher(_p);
    auto [index_search, _object_search] = _indexes;
    auto [line_search, circle_search] = _object_search;

    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 0) {
      if (index_search != -1) {
        isDragging = true;
        follower = index_search;
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
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 9) {
      if (this->live_stack_circles.size() == 1) {
        if (line_search != -1) {
          this->live_stack_lines.push_back(this->lines[line_search]);
        } else if (circle_search != -1) {
          this->live_stack_circles.push_back(this->circles[circle_search]);
        } else {
          this->live_stack.push_back(p);
        }
      } else {
        this->live_stack_circles.push_back(this->circles[circle_search]);
      }
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

  if (this->current_tool == 9 && this->live_stack_circles.size() >= 1 && this->live_stack.size() + this->live_stack_lines.size() + this->live_stack_circles.size() == 2) {
    if (this->live_stack.size() == 1) {
      AlgGeom::Point p = AlgGeom::CoreGeometryTools::inversion_point(
        AlgGeom::Point(this->live_stack[0].x_pos, this->live_stack[0].y_pos),
        AlgGeom::Circle(
          AlgGeom::Point(
            this->live_stack_circles[0].x_pos, this->live_stack_circles[0].y_pos
          ),
          this->live_stack_circles[0].radius
        )
      );
      this->points.push_back(
        GPoint(p.x, p.y)
      );
    } else if (this->live_stack_lines.size() == 1) {
      AlgGeom::Circle l = AlgGeom::CoreGeometryTools::inversion_line(
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
      this->circles.push_back(
        GCircle(l.p.x, l.p.y, l.radius)
      );
    }
    
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
}

void GeometryVisual::draw(sf::RenderWindow& window) {
  for (auto& point : this->points) {
    point.draw(window);
  }
  for (auto& line : this->lines) {
    line.draw(window);
  }
  for (auto& circle : this->circles) {
    circle.draw(window);
  }
  for (auto& conic : this->conics) {
    conic.draw(window);
  }
  for (auto& cubic : this->cubics) {
    cubic.draw(window);
  }
}










