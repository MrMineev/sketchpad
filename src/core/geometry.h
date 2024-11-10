#pragma once

#include "primitives/point.h"
#include "primitives/line.h"
#include "primitives/circle.h"

#include "../../gui_tools/src/Gui/Gui.hpp"

class GeometryVisual {
 private:
  std::vector<GPoint> points;
  std::vector<GLine> lines;
  std::vector<GCircle> circles;

  std::vector<GPoint> live_stack;

 public:
  int current_tool = 0;

  int point_searcher(GPoint p);
  void handleEvent(const sf::Event& event, sf::RenderWindow& window, gui::Menu& menu);
  void draw(sf::RenderWindow& window);
  GeometryVisual() {}
};
