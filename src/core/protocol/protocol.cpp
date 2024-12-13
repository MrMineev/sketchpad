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
    {"func", "incenter"},
    {"type", "Point"},
    {"args", {x, y, z}}
  };
  this->protocol["order"].push_back({"Point", pos});
}

void Protocol::new_excenter(int pos, int x, int y, int z) {
  this->protocol["Point"][pos] = {
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

void Protocol::new_inter_ll(int pos, int x, int y) {
  this->protocol["Point"][pos] = {
    {"func", "interLL"},
    {"type", "Point"},
    {"args", {x, y}}
  };
  this->protocol["order"].push_back({"Point", pos});
}

void Protocol::new_line(int pos, int x, int y, bool state) {
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

void Protocol::edit_position(int follower, ld px, ld py) {
  this->protocol["Point"][follower]["location"][0] = px;
  this->protocol["Point"][follower]["location"][1] = py;
}
























