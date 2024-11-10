#include "geometry.h"
#include "alg.h"
#include "../../gui_tools/src/Gui/Gui.hpp"

const long double EPS = 10;

int GeometryVisual::point_searcher(GPoint p) {
  for (int i = 0; i < this->points.size(); i++) {
    if (AlgGeom::CoreGeometryTools::dist_points(
      AlgGeom::Point(p.x_pos, p.y_pos),
      AlgGeom::Point(this->points[i].x_pos, this->points[i].y_pos)
    ) <= EPS) {
      return i;
    }
  }
  return -1;
}

void GeometryVisual::handleEvent(const sf::Event& event, sf::RenderWindow& window, gui::Menu& menu) {
  if (event.type == sf::Event::MouseButtonPressed) {
    if (event.mouseButton.x <= this->X_MENU_BORDER) return;

    GPoint p(event.mouseButton.x, event.mouseButton.y);
    int index_search = this->point_searcher(p);
    if (index_search != -1 && event.mouseButton.button == sf::Mouse::Left && this->current_tool == 1) {
      this->points.push_back(p);
    }
    if (index_search == -1) {
      this->points.push_back(p);
    } else {
      p = this->points[index_search];
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 2) {
      this->live_stack.push_back(p);
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 3) {
      this->live_stack.push_back(p);
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 4) {
      this->live_stack.push_back(p);
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 5) {
      this->live_stack.push_back(p);
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
  }

  if (this->current_tool == 4 && this->live_stack.size() >= 2) {
    this->points.push_back(GPoint(
      (this->live_stack[0].x_pos + this->live_stack[1].x_pos) / 2,
      (this->live_stack[0].y_pos + this->live_stack[1].y_pos) / 2
    ));
    this->live_stack.clear();
  }

  if (this->current_tool == 5 && this->live_stack.size() >= 3) {
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










