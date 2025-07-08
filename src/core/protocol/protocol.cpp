#include "protocol.h"

typedef long double ld;

using json = nlohmann::json; 

void Protocol::new_point(int pos, ld px, ld py) {
  this->protocol["Point"][pos] = {
    {"func", "newPoint"},
    {"type", "Point"},
    {"location", {px, py}}
  };
  this->protocol["order"].push_back({"Point", pos});
}

void Protocol::new_point_on_line(int pos, int line_index, long double ratio) {
  this->protocol["Point"][pos] = {
    {"func", "newPointOnLine"},
    {"type", "Point"},
    {"args", {line_index, ratio}}
  };
  this->protocol["order"].push_back({"Point", pos});
}

void Protocol::new_circumcircle(int pos, int x, int y, int z) {
  this->protocol["Circle"][pos] = {
    {"func", "circumcircle"},
    {"type", "Circle"},
    {"args", {x, y, z}}
  };
  this->protocol["order"].push_back({"Circle", pos});
}

void Protocol::new_incenter(int pos, int x, int y, int z) {
  this->protocol["Point"][pos] = {
    {"func", "new_incenter"},
    {"type", "Point"},
    {"args", {x, y, z}}
  };
  this->protocol["order"].push_back({"Point", pos});
}

void Protocol::new_excenter(int pos, int x, int y, int z) {
  this->protocol["Point"][pos] = {
    {"func", "new_excenter"},
    {"type", "Point"},
    {"args", {x, y, z}}
  };
  this->protocol["order"].push_back({"Point", pos});
}

void Protocol::new_inter_lc(int pos1, int pos2, int x, int y) {
  this->protocol["Point"][pos1] = {
    {"func", "interLC"},
    {"type", "Point"},
    {"args", {x, y}},
    {"version", 1}
  };
  this->protocol["Point"][pos2] = {
    {"func", "interLC"},
    {"type", "Point"},
    {"args", {x, y}},
    {"version", 2}
  };
  this->protocol["order"].push_back({"Point", pos1});
  this->protocol["order"].push_back({"Point", pos2});
}

void Protocol::new_midpoint(int pos, int x, int y) {
  this->protocol["Point"][pos] = {
    {"func", "midpoint"},
    {"type", "Point"},
    {"args", {x, y}}
  };
  this->protocol["order"].push_back({"Point", pos});
}

void Protocol::new_perp_normal(int pos, int x, int y) {
  this->protocol["Line"][pos] = {
    {"func", "perpNormal"},
    {"type", "Line"},
    {"args", {x, y}}
  };
  this->protocol["order"].push_back({"Line", pos});
}

void Protocol::new_parallel(int pos, int x, int y) {
  this->protocol["Line"][pos] = {
    {"func", "parallel"},
    {"type", "Line"},
    {"args", {x, y}}
  };
  this->protocol["order"].push_back({"Line", pos});
}

void Protocol::new_inter_ll(int pos, int x, int y) {
  this->protocol["Point"][pos] = {
    {"func", "interLL"},
    {"type", "Point"},
    {"args", {x, y}}
  };
  this->protocol["order"].push_back({"Point", pos});
}

void Protocol::new_line(int pos, int x, int y, int state) {
  this->protocol["Line"][pos] = {
    {"func", "newLine"},
    {"type", "Line"},
    {"args", {x, y}},
    {"version", state}
  };
  this->protocol["order"].push_back({"Line", pos});
}

void Protocol::new_circle(int pos, int x, int y) {
  this->protocol["Circle"][pos] = {
    {"func", "newCircle"},
    {"type", "Circle"},
    {"args", {x, y}}
  };
  this->protocol["order"].push_back({"Circle", pos});
}

void Protocol::new_conic(int pos, int x1, int x2, int x3, int x4, int x5) {
  this->protocol["Conic"][pos] = {
    {"func", "newConic"},
    {"type", "Conic"},
    {"args", {x1, x2, x3, x4, x5}}
  };
  this->protocol["order"].push_back({"Conic", pos});
}

void Protocol::new_cubic(int pos, int x1, int x2, int x3, int x4, int x5, int x6, int x7, int x8, int x9) {
  this->protocol["Cubic"][pos] = {
    {"func", "newCubic"},
    {"type", "Cubic"},
    {"args", {x1, x2, x3, x4, x5, x6, x7, x8, x9}}
  };
  this->protocol["order"].push_back({"Cubic", pos});
}

std::string Protocol::get_string_format() {
  return this->protocol.dump();
}

void Protocol::save_data() {
  std::ofstream file("output.json");
  std::string s = this->get_string_format();
  file << s << std::endl;
}

std::vector<std::pair<std::string, int>> Protocol::get_order() {
  return this->protocol["order"];
}

json Protocol::get_info(std::string &t, int index) {
  return this->protocol[t][index];
}

void Protocol::load_data(std::string &pathway) {
  std::ifstream f(pathway);
  this->protocol = json::parse(f);
  std::cout << "PROTOCOL LOADED (protocol)!" << std::endl;
}

bool Protocol::is_point_def_by_func(int pos, std::string func) {
  return (this->protocol["Point"][pos]["func"] == func);
}

json Protocol::get_point_info(int pos) {
  return this->protocol["Point"][pos];
}

void Protocol::edit_position(int follower, ld px, ld py) {
  this->protocol["Point"][follower]["location"][0] = px;
  this->protocol["Point"][follower]["location"][1] = py;
}
























