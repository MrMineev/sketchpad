#pragma once

#include "primitives/point.h"
#include "primitives/line.h"
#include "primitives/circle.h"

#include "../../gui_tools/src/Gui/Gui.hpp"

class GeometryVisual {
 private:
  std::vector<Point> points;
  std::vector<Line> lines;
  std::vector<Circle> circles;

  std::vector<Point> live_stack;

 public:
  int current_tool = 0;

  void handleEvent(const sf::Event& event, sf::RenderWindow& window, gui::Menu& menu);
  void draw(sf::RenderWindow& window);
  GeometryVisual() {}
};
