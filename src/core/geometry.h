#pragma once

#include "primitives/point.h"
#include "primitives/line.h"
#include "primitives/circle.h"
#include "primitives/conic.h"
#include "primitives/cubic.h"

#include "protocol/protocol.h"

#include "../../gui_tools/src/Gui/Gui.hpp"
#include "../../json_manager/json/single_include/nlohmann/json.hpp"

using json = nlohmann::json; 

class GeometryVisual {
 private:
  std::vector<GPoint> live_stack;
  std::vector<GLine> live_stack_lines;
  std::vector<GCircle> live_stack_circles;

  void delete_object(std::string type, int index);

  int X_MENU_BORDER;

 public:
  std::vector<GPoint> points;
  std::vector<GLine> lines;
  std::vector<GCircle> circles;
  std::vector<GConic> conics;
  std::vector<GCubic> cubics;


  int current_tool = 0;

  bool isDragging = false;
  bool refresh_geo_genie_on_release = false;
  int follower = -1;
  int selected_point = -1;
  int selected_line = -1;
  int selected_circle = -1;
  int inversion_circle_request = -1;
  int rename_point_request = -1;

  Protocol protocol;

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
  void draw(sf::RenderWindow& window, const sf::Font *font = nullptr);
  void rebuild();
  void delete_point(int index);
  void delete_line(int index);
  void delete_circle(int index);
  void hide_geo_genie();
  int take_inversion_request();
  int take_rename_point_request();
  void show_all();
  void build_inversion(const GeometryVisual &source, int inversion_circle);

  void save_configuration(std::string &filepath);
  void load_configuration(std::string &filepath);

  GeometryVisual(int _MENU_BORDER) {
    X_MENU_BORDER = _MENU_BORDER;
  }
};













