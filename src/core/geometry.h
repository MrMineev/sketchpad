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
  std::vector<GLine> live_stack_lines;
  std::vector<GCircle> live_stack_circles;

  int X_MENU_BORDER;

 public:
  int current_tool = 0;

  std::string protocol = "";

  std::string new_point(int pos, long double x, long double y);
  std::pair<std::pair<int, std::pair<int, int>>, GPoint> point_searcher(GPoint p);
  void handleEvent(const sf::Event& event, sf::RenderWindow& window, gui::Menu& menu);
  void draw(sf::RenderWindow& window);
  GeometryVisual(int _MENU_BORDER) {
    X_MENU_BORDER = _MENU_BORDER;
  }
};
