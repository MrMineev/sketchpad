#include "protocol.h"

typedef long double ld;

using json = nlohmann::json; 

void Protocol::new_point(int pos, ld px, ld py) {
  this->protocol["Point"][pos] = {
    {"type", "Point"},
    {"location", {px, py}}
  };
  this->protocol["order"].push_back({"Point", pos});
}

void Protocol::new_circumcircle(int pos, int x, int y, int z) {
  this->protocol["Circle"][pos] = {
    {"type", "Circle"},
    {"args", {x, y, z}}
  };
  this->protocol["order"].push_back({"Circle", pos});
}

void Protocol::new_incenter(int pos, int x, int y, int z) {
  this->protocol["Point"][pos] = {
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
    {"type", "Point"},
    {"args", {x, y}},
    {"version", 1}
  };
  this->protocol["Point"][pos2] = {
    {"type", "Point"},
    {"args", {x, y}},
    {"version", 2}
  };
  this->protocol["order"].push_back({"Point", pos1});
  this->protocol["order"].push_back({"Point", pos2});
}

void Protocol::new_midpoint(int pos, int x, int y) {
  this->protocol["Point"][pos] = {
    {"type", "Point"},
    {"args", {x, y}}
  };
  this->protocol["order"].push_back({"Point", pos});
}

void Protocol::new_perp_normal(int pos, int x, int y) {
  this->protocol["Line"][pos] = {
    {"type", "Line"},
    {"args", {x, y}}
  };
  this->protocol["order"].push_back({"Line", pos});
}

void Protocol::new_line_intersection(int pos, int x, int y) {
  this->protocol["Point"][pos] = {
    {"type", "Point"},
    {"args", {x, y}}
  };
  this->protocol["order"].push_back({"Point", pos});
}

void Protocol::new_line(int pos, int x, int y, bool state) {
  this->protocol["Line"][pos] = {
    {"type", "Line"},
    {"args", {x, y}},
    {"version", state}
  };
}

void Protocol::new_circle(int pos, int x, int y) {
  this->protocol["Circle"][pos] = {
    {"type", "Circle"},
    {"args", {x, y}}
  };
  this->protocol["order"].push_back({"Circle", pos});
}

std::string Protocol::get_string_format() {
  std::string res = "";
  for (auto [t, pos] : this->protocol["order"]) {
    if (t == "Point") {
      json point_definition = this->protocol["Point"][pos];
    } else if (t == "Line") {
      json line_definition = this->protocol["Line"][pos];
    } else {
      json circle_definition = this->protocol["Circle"][pos];
    }
  }
}
























