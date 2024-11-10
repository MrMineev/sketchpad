#include "geometry.h"
#include "alg.h"
#include "../../gui_tools/src/Gui/Gui.hpp"

typedef long double ld;

const long double EPS = 10;

string GeometryVisual::new_point(int pos, ld x, ld y) {
  string res = "";
  char c = 'A' + this->points.size() - 1;
  res += "\n";
  res += c;
  res += " = (" + to_string(x) + " " + to_string(y) + ")";
  return res;
}

pair<pair<int, int>, GPoint> GeometryVisual::point_searcher(GPoint p) {
  for (int i = 0; i < this->points.size(); i++) {
    if (AlgGeom::CoreGeometryTools::dist_points(
      AlgGeom::Point(p.x_pos, p.y_pos),
      AlgGeom::Point(this->points[i].x_pos, this->points[i].y_pos)
    ) <= EPS) {
      return make_pair(make_pair(i, -1), this->points[i]);
    }
  }
  for (int i = 0; i < this->lines.size(); i++) {
    // TODO MAKE DIFFERENTIATE BETWEEN LINE AND SEGMENT
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
      return make_pair(make_pair(-1, i), GPoint(new_loc.x, new_loc.y));
    }
  }
  return make_pair(make_pair(-1, -1), p);
}

void GeometryVisual::handleEvent(const sf::Event& event, sf::RenderWindow& window, gui::Menu& menu) {
  if (event.type == sf::Event::MouseButtonPressed) {
    if (event.mouseButton.x <= this->X_MENU_BORDER) return;

    GPoint _p(event.mouseButton.x, event.mouseButton.y);
    auto [_indexes, p] = this->point_searcher(_p);
    auto [index_search, line_search] = _indexes;
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 1) {
      if (index_search == -1) {
        this->points.push_back(p);
        protocol += new_point(this->points.size() - 1, p.x_pos, p.y_pos);
      }
    }
    
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 2) {
      this->live_stack.push_back(p);
      if (index_search == -1) {
        this->points.push_back(p);
        protocol += new_point(this->points.size() - 1, p.x_pos, p.y_pos);
      }
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 3) {
      this->live_stack.push_back(p);
      if (index_search == -1) {
        this->points.push_back(p);
        protocol += new_point(this->points.size() - 1, p.x_pos, p.y_pos);
      }
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 4) {
      this->live_stack.push_back(p);
      if (index_search == -1) {
        this->points.push_back(p);
        protocol += new_point(this->points.size() - 1, p.x_pos, p.y_pos);
      }
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 5) {
      this->live_stack.push_back(p);
      if (index_search == -1) {
        this->points.push_back(p);
        protocol += new_point(this->points.size() - 1, p.x_pos, p.y_pos);
      }
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 6) {
      this->live_stack.push_back(p);
      if (index_search == -1) {
        this->points.push_back(p);
        protocol += new_point(this->points.size() - 1, p.x_pos, p.y_pos);
      }
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 7) {
      this->live_stack_lines.push_back(this->lines[line_search]);
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 8) {
      cout << "POP! " << this->live_stack_lines.size() << endl;
      if (this->live_stack_lines.size() == 1) {
        cout << "POINT!" << endl;
        this->live_stack.push_back(p);
      } else {
        if (line_search != -1) {
          this->live_stack_lines.push_back(this->lines[line_search]);
        } else {
          this->live_stack.push_back(p);
        }
      }
    }
  }
  
  if (this->current_tool == 2 && this->live_stack.size() >= 2) {
    this->lines.push_back(GLine(
      this->live_stack[0].x_pos,
      this->live_stack[0].y_pos,
      this->live_stack[1].x_pos,
      this->live_stack[1].y_pos
    ));

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

    this->live_stack.clear();
    this->live_stack_lines.clear();
    this->live_stack_circles.clear();
  }

  if (this->current_tool == 4 && this->live_stack.size() >= 2) {
    this->points.push_back(GPoint(
      (this->live_stack[0].x_pos + this->live_stack[1].x_pos) / 2,
      (this->live_stack[0].y_pos + this->live_stack[1].y_pos) / 2
    ));

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
      false
    ));

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
      false
    ));

    this->live_stack.clear();
    this->live_stack_lines.clear();
    this->live_stack_circles.clear();
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
}










