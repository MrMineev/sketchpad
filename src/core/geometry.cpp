#include "geometry.h"
#include "alg.h"
#include "../../gui_tools/src/Gui/Gui.hpp"

void GeometryVisual::handleEvent(const sf::Event& event, sf::RenderWindow& window, gui::Menu& menu) {
  if (event.type == sf::Event::MouseButtonPressed) {
    GPoint p(event.mouseButton.x, event.mouseButton.y);
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 1) {
      this->points.push_back(p);
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 2) {
      this->live_stack.push_back(p);
      this->points.push_back(p);
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 3) {
      this->live_stack.push_back(p);
      this->points.push_back(p);
    }
  }
  
  if (this->current_tool == 2 && this->live_stack.size() >= 2) {
    this->lines.push_back(GLine(
      this->live_stack[0].x_pos,
      this->live_stack[0].y_pos,
      this->live_stack[1].x_pos,
      this->live_stack[1].y_pos
    ));
    live_stack.clear();
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
    live_stack.clear();
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










