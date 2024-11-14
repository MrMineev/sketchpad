#pragma once

#include "../../../json_manager/json/single_include/nlohmann/json.hpp"

using json = nlohmann::json; 

class Protocol {
 private:
  json protocol;

 public:
  void new_point(int pos, long double x, long double y);
  void new_circumcircle(int pos, int x, int y, int z);
  void new_incenter(int pos, int x, int y, int z);
  void new_excenter(int pos, int x, int y, int z);
  void new_inter_lc(int pos1, int pos2, int x, int y);
  void new_midpoint(int pos, int x, int y);
  void new_perp_normal(int pos, int x, int y);
  void new_line_intersection(int pos, int x, int y);
  void new_line(int pos, int x, int y, bool state);
  void new_circle(int pos, int x, int y);

  std::string get_string_format();

  Protocol() {
    protocol = {
      {"Point", {}},
      {"Line", {}},
      {"Circle", {}},
      {"order", {}}
    };
  }
};
