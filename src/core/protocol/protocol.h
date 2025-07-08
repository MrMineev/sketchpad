#pragma once

#include "../../../json_manager/json/single_include/nlohmann/json.hpp"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

typedef long double ld;

class Protocol {
 private:

 public:
  json protocol;
  
  bool is_point_def_by_func(int pos, std::string func);
  json get_point_info(int pos);
  void new_point(int pos, long double x, long double y);
  void new_point_on_line(int pos, int line_index, long double ratio);
  void new_circumcircle(int pos, int x, int y, int z);
  void new_incenter(int pos, int x, int y, int z);
  void new_excenter(int pos, int x, int y, int z);
  void new_inter_lc(int pos1, int pos2, int x, int y);
  void new_midpoint(int pos, int x, int y);
  void new_perp_normal(int pos, int x, int y);
  void new_parallel(int pos, int x, int y);
  void new_inter_ll(int pos, int x, int y);
  void new_line(int pos, int x, int y, int state); // state = 2 (dashed)
  void new_circle(int pos, int x, int y);
  void new_conic(int pos, int x1, int x2, int x3, int x4, int x5);
  void new_cubic(int pos, int x1, int x2, int x3, int x4, int x5, int x6, int x7, int x8, int x9);
  void edit_position(int pos, ld x, ld y);

  std::vector<std::pair<std::string, int>> get_order();
  json get_info(std::string &s, int index);

  std::string get_string_format();
  void save_data();
  void load_data(std::string &pathway);

  Protocol() {
    std::cout << "Initialized!" << std::endl;
    protocol = {
      {"Point", {}},
      {"Line", {}},
      {"Circle", {}},
      {"Conic", {}},
      {"Cubic", {}},
      {"order", {}}
    };
  }
};
