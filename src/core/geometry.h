#pragma once

#include "primitives/point.h"
#include "primitives/line.h"
#include "primitives/circle.h"

#include "../../gui_tools/src/Gui/Gui.hpp"
#include "../../json_manager/json/single_include/nlohmann/json.hpp"

using json = nlohmann::json; 

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

  bool isDragging = false;
  int follower = -1;

  json protocol;

  std::string new_point(int pos);
  std::string new_circumcircle(int pos, int x, int y, int z);
  std::string new_incenter(int pos, int x, int y, int z);
  std::string new_excenter(int pos, int x, int y, int z);
  std::string new_inter_lc(int pos1, int pos2, int x, int y);
  std::string new_midpoint(int pos, int x, int y);
  std::string new_perp_normal(int pos, int x, int y);
  std::string new_line_intersection(int pos, int x, int y);
  std::string new_line(int pos, int x, int y, bool state);
  std::string new_circle(int pos, int x, int y);

  std::pair<std::pair<int, std::pair<int, int>>, GPoint> point_searcher(GPoint p);
  void handleEvent(const sf::Event& event, sf::RenderWindow& window, gui::Menu& menu);
  void draw(sf::RenderWindow& window);
  void rebuild();

  GeometryVisual(int _MENU_BORDER) {
    X_MENU_BORDER = _MENU_BORDER;
    protocol = {};
  }
};
