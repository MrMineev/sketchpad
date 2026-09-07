#include "protocol.h"

#include <algorithm>
#include <map>
#include <set>
#include <vector>

typedef long double ld;

using json = nlohmann::json; 

static void apply_default_style(const std::string &type, json &object, bool overwrite) {
  json color = {0, 0, 0};
  if (type == "Point") color = {255, 0, 0};
  if (type == "Line") {
    color = object.value("version", 0) == 2 ? json({238, 130, 238}) : json({0, 0, 255});
  }
  if (type == "Circle") color = {0, 255, 0};
  if (type == "Conic") color = {0, 255, 255};
  if (overwrite || !object.contains("color")) object["color"] = color;
  if (overwrite || !object.contains("thickness")) object["thickness"] = 1.f;
  if (type != "Point" && (overwrite || !object.contains("dashed"))) {
    object["dashed"] = type == "Line" && object.value("version", 0) == 2;
  }
  if (overwrite || !object.contains("style_custom")) object["style_custom"] = false;
}

std::string Protocol::next_point_label() const {
  std::set<std::string> used;
  if (this->protocol.contains("Point") && this->protocol["Point"].is_array()) {
    for (const json &point : this->protocol["Point"]) {
      if (point.is_object() && point.contains("label")) used.insert(point["label"].get<std::string>());
    }
  }
  for (int index = 0;; ++index) {
    int value = index + 1;
    std::string label;
    while (value > 0) {
      --value;
      label.insert(label.begin(), static_cast<char>('A' + value % 26));
      value /= 26;
    }
    if (!used.count(label)) return label;
  }
}

void Protocol::initialize_point_metadata(int pos) {
  json &point = this->protocol["Point"][pos];
  if (!point.contains("label")) point["label"] = this->next_point_label();
  if (!point.contains("visible")) point["visible"] = true;
  if (!point.contains("label_visible")) point["label_visible"] = true;
  apply_default_style("Point", point, false);
}

void Protocol::ensure_metadata() {
  if (this->protocol["Point"].is_array()) {
    for (size_t i = 0; i < this->protocol["Point"].size(); ++i) {
      if (this->protocol["Point"][i].is_object()) this->initialize_point_metadata(i);
    }
  }
  const std::vector<std::string> categories = {"Line", "Circle", "Conic", "Cubic", "Text"};
  for (const std::string &category : categories) {
    if (!this->protocol[category].is_array()) continue;
    for (json &object : this->protocol[category]) {
      if (!object.is_object()) continue;
      if (!object.contains("visible")) object["visible"] = true;
      if (category == "Line" || category == "Circle" || category == "Conic") {
        apply_default_style(category, object, false);
        if (category == "Circle" && !object.value("style_custom", false) &&
            object["color"] == json({0, 128, 0})) {
          object["color"] = {0, 255, 0};
        }
      }
    }
  }
}

void Protocol::set_visibility(std::string type, int pos, bool visible) {
  if (!this->protocol.contains(type) || !this->protocol[type].is_array() ||
      pos < 0 || pos >= this->protocol[type].size() || !this->protocol[type][pos].is_object()) return;
  this->protocol[type][pos]["visible"] = visible;
}

void Protocol::set_style(std::string type, int pos, int red, int green, int blue,
                         float thickness, bool dashed) {
  if (!this->protocol.contains(type) || !this->protocol[type].is_array() ||
      pos < 0 || pos >= this->protocol[type].size() || !this->protocol[type][pos].is_object()) return;
  json &object = this->protocol[type][pos];
  object["color"] = {
    std::clamp(red, 0, 255), std::clamp(green, 0, 255), std::clamp(blue, 0, 255)
  };
  object["thickness"] = std::clamp(thickness, 1.f, 10.f);
  object["style_custom"] = true;
  if (type != "Point") object["dashed"] = dashed;
}

void Protocol::reset_style(std::string type, int pos) {
  if (!this->protocol.contains(type) || !this->protocol[type].is_array() ||
      pos < 0 || pos >= this->protocol[type].size() || !this->protocol[type][pos].is_object()) return;
  apply_default_style(type, this->protocol[type][pos], true);
}

void Protocol::set_point_label_visibility(int pos, bool visible) {
  if (!this->protocol["Point"].is_array() || pos < 0 || pos >= this->protocol["Point"].size() ||
      !this->protocol["Point"][pos].is_object()) return;
  this->protocol["Point"][pos]["label_visible"] = visible;
}

void Protocol::set_point_label(int pos, std::string label) {
  if (!this->protocol["Point"].is_array() || pos < 0 || pos >= this->protocol["Point"].size() ||
      !this->protocol["Point"][pos].is_object() || label.empty()) return;
  this->protocol["Point"][pos]["label"] = label;
}

void Protocol::show_all() {
  this->ensure_metadata();
  const std::vector<std::string> categories = {"Point", "Line", "Circle", "Conic", "Cubic", "Text"};
  for (const std::string &category : categories) {
    if (!this->protocol[category].is_array()) continue;
    for (json &object : this->protocol[category]) {
      if (!object.is_object()) continue;
      object["visible"] = true;
      if (category == "Point") object["label_visible"] = true;
    }
  }
}

void Protocol::new_point(int pos, ld px, ld py) {
  this->protocol["Point"][pos] = {
    {"func", "newPoint"},
    {"type", "Point"},
    {"location", {px, py}}
  };
  this->initialize_point_metadata(pos);
  this->protocol["order"].push_back({"Point", pos});
}

void Protocol::new_point_on_line(int pos, int line_index, long double ratio) {
  this->protocol["Point"][pos] = {
    {"func", "newPointOnLine"},
    {"type", "Point"},
    {"args", {line_index, ratio}}
  };
  this->initialize_point_metadata(pos);
  this->protocol["order"].push_back({"Point", pos});
}

void Protocol::new_point_on_circle(int pos, int circle_index, long double angle) {
  this->protocol["Point"][pos] = {
    {"func", "newPointOnCircle"},
    {"type", "Point"},
    {"args", {circle_index, angle}}
  };
  this->initialize_point_metadata(pos);
  this->protocol["order"].push_back({"Point", pos});
}

void Protocol::new_point_on_conic(int pos, int conic_index, ld x, ld y) {
  this->protocol["Point"][pos] = {
    {"func", "newPointOnConic"},
    {"type", "Point"},
    {"args", {conic_index}},
    {"location", {x, y}}
  };
  this->initialize_point_metadata(pos);
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

void Protocol::new_angle_bisector(int pos, int x, int y, int z) {
  this->protocol["Line"][pos] = {
    {"func", "newAngleBisector"},
    {"type", "Line"},
    {"args", {x, y, z}},
    {"version", 1}
  };
  this->protocol["order"].push_back({"Line", pos});
}

void Protocol::new_reflect_line_over_line(int pos, int x, int y) {
  this->protocol["Line"][pos] = {
    {"func", "newReflectLineOverLine"},
    {"type", "Line"},
    {"args", {x, y}}
  };
  this->protocol["order"].push_back({"Line", pos});
}

void Protocol::new_isogonal_conjugate(int pos, int x, int y, int z, int w) {
  this->protocol["Point"][pos] = {
    {"func", "newIsogonalConjugate"},
    {"type", "Point"},
    {"args", {x, y, z, w}},
    {"version", 1}
  };
  this->initialize_point_metadata(pos);
  this->protocol["order"].push_back({"Point", pos});
}

void Protocol::new_reflect_point_over_line(int pos, int x, int y) {
  this->protocol["Point"][pos] = {
    {"func", "newReflectPointOverLine"},
    {"type", "Point"},
    {"args", {x, y}}
  };
  this->initialize_point_metadata(pos);
  this->protocol["order"].push_back({"Point", pos});
}

void Protocol::new_reflect_point_over_point(int pos, int x, int y) {
  this->protocol["Point"][pos] = {
    {"func", "newReflectPointOverPoint"},
    {"type", "Point"},
    {"args", {x, y}}
  };
  this->initialize_point_metadata(pos);
  this->protocol["order"].push_back({"Point", pos});
}

void Protocol::new_project_point_onto_line(int pos, int point, int line) {
  this->protocol["Point"][pos] = {
    {"func", "newProjectPointOntoLine"},
    {"type", "Point"},
    {"args", {point, line}}
  };
  this->initialize_point_metadata(pos);
  this->protocol["order"].push_back({"Point", pos});
}

void Protocol::new_incenter(int pos, int x, int y, int z) {
  this->protocol["Point"][pos] = {
    {"func", "new_incenter"},
    {"type", "Point"},
    {"args", {x, y, z}}
  };
  this->initialize_point_metadata(pos);
  this->protocol["order"].push_back({"Point", pos});
}

void Protocol::new_circumcenter(int pos, int x, int y, int z) {
  this->protocol["Point"][pos] = {
    {"func", "newCircumcenter"},
    {"type", "Point"},
    {"args", {x, y, z}}
  };
  this->initialize_point_metadata(pos);
  this->protocol["order"].push_back({"Point", pos});
}

void Protocol::new_triangle_center(int pos, int x, int y, int z, int number) {
  this->protocol["Point"][pos] = {
    {"func", "newTriangleCenter"},
    {"type", "Point"},
    {"args", {x, y, z, number}}
  };
  this->initialize_point_metadata(pos);
  this->protocol["order"].push_back({"Point", pos});
}

void Protocol::new_excenter(int pos, int x, int y, int z) {
  this->protocol["Point"][pos] = {
    {"func", "new_excenter"},
    {"type", "Point"},
    {"args", {x, y, z}}
  };
  this->initialize_point_metadata(pos);
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
  this->initialize_point_metadata(pos1);
  this->initialize_point_metadata(pos2);
  this->protocol["order"].push_back({"Point", pos1});
  this->protocol["order"].push_back({"Point", pos2});
}

void Protocol::new_midpoint(int pos, int x, int y) {
  this->protocol["Point"][pos] = {
    {"func", "midpoint"},
    {"type", "Point"},
    {"args", {x, y}}
  };
  this->initialize_point_metadata(pos);
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
  this->initialize_point_metadata(pos);
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

void Protocol::new_rectangular_hyperbola(int pos, int center, int p1, int p2) {
  this->protocol["Conic"][pos] = {
    {"func", "newRectangularHyperbola"},
    {"type", "Conic"},
    {"args", {center, p1, p2}}
  };
  this->protocol["order"].push_back({"Conic", pos});
}

void Protocol::new_center(int pos, std::string source_type, int source) {
  this->protocol["Point"][pos] = {
    {"func", "newCenter"},
    {"type", "Point"},
    {"source_type", source_type},
    {"args", {source}}
  };
  this->initialize_point_metadata(pos);
  this->protocol["order"].push_back({"Point", pos});
}

void Protocol::new_cubic(int pos, int x1, int x2, int x3, int x4, int x5, int x6, int x7, int x8, int x9) {
  this->protocol["Cubic"][pos] = {
    {"func", "newCubic"},
    {"type", "Cubic"},
    {"args", {x1, x2, x3, x4, x5, x6, x7, x8, x9}}
  };
  this->protocol["order"].push_back({"Cubic", pos});
}

void Protocol::new_text(int pos, ld x, ld y, std::string content) {
  this->protocol["Text"][pos] = {
    {"func", "newText"},
    {"type", "Text"},
    {"location", {x, y}},
    {"content", content},
    {"visible", true}
  };
  this->protocol["order"].push_back({"Text", pos});
}

std::string Protocol::get_string_format() {
  this->ensure_metadata();
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
  this->ensure_metadata();
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

void Protocol::edit_text_position(int pos, ld x, ld y) {
  if (!this->protocol["Text"].is_array() || pos < 0 || pos >= this->protocol["Text"].size()) return;
  this->protocol["Text"][pos]["location"] = {x, y};
}

void Protocol::delete_obj(std::string start_cat, int pos) {
  const std::vector<std::string> categories = {"Point", "Line", "Circle", "Conic", "Cubic", "Text"};

  auto dependency_type = [](const json &value, size_t arg) -> std::string {
    if (!value.contains("func")) return "";
    const std::string func = value["func"];

    if (func == "newCenter") return arg == 0 ? value.value("source_type", "") : "";
    if (func == "newPointOnLine") return arg == 0 ? "Line" : "";
    if (func == "newPointOnCircle") return arg == 0 ? "Circle" : "";
    if (func == "newPointOnConic") return arg == 0 ? "Conic" : "";
    if (func == "interLL" || func == "newReflectLineOverLine") return "Line";
    if (func == "interLC") return arg == 0 ? "Line" : "Circle";
    if (func == "newReflectPointOverLine" || func == "newProjectPointOntoLine" ||
        func == "perpNormal" || func == "parallel") {
      return arg == 0 ? "Point" : "Line";
    }
    if (func == "newTriangleCenter") return arg < 3 ? "Point" : "";
    if (func == "newLine" || func == "newCircle" || func == "midpoint" ||
        func == "newReflectPointOverPoint" ||
        func == "circumcircle" || func == "new_incenter" || func == "newCircumcenter" ||
        func == "new_excenter" ||
        func == "newIsogonalConjugate" || func == "newConic" ||
        func == "newRectangularHyperbola" || func == "newCubic" ||
        func == "newAngleBisector") {
      return "Point";
    }
    return "";
  };

  auto entries = [&](const std::string &category) {
    std::vector<std::pair<int, json>> result;
    if (!this->protocol.contains(category)) return result;
    const json &container = this->protocol[category];
    if (container.is_array()) {
      for (size_t i = 0; i < container.size(); ++i) {
        if (container[i].is_object()) result.emplace_back(static_cast<int>(i), container[i]);
      }
    } else if (container.is_object()) {
      for (const auto &[key, value] : container.items()) {
        try {
          if (value.is_object()) result.emplace_back(std::stoi(key), value);
        } catch (...) {
        }
      }
      std::sort(result.begin(), result.end(), [](const auto &a, const auto &b) {
        return a.first < b.first;
      });
    }
    return result;
  };

  std::vector<std::pair<std::string, int>> queue = {{start_cat, pos}};
  std::vector<std::string> deleted;
  for (size_t qi = 0; qi < queue.size(); ++qi) {
    const auto [current_category, current_index] = queue[qi];
    const std::string current_key = current_category + ":" + std::to_string(current_index);
    if (std::find(deleted.begin(), deleted.end(), current_key) != deleted.end()) continue;
    deleted.push_back(current_key);

    for (const std::string &category : categories) {
      for (const auto &[index, value] : entries(category)) {
        if (!value.contains("args") || !value["args"].is_array()) continue;
        for (size_t arg = 0; arg < value["args"].size(); ++arg) {
          if (dependency_type(value, arg) == current_category &&
              value["args"][arg].is_number_integer() &&
              value["args"][arg].get<int>() == current_index) {
            queue.emplace_back(category, index);
            break;
          }
        }
      }
    }
  }

  std::map<std::string, std::map<int, int>> remapped_indices;
  for (const std::string &category : categories) {
    json compacted = json::array();
    for (const auto &[old_index, value] : entries(category)) {
      const std::string key = category + ":" + std::to_string(old_index);
      if (std::find(deleted.begin(), deleted.end(), key) != deleted.end()) continue;
      remapped_indices[category][old_index] = static_cast<int>(compacted.size());
      compacted.push_back(value);
    }
    this->protocol[category] = compacted.empty() ? json(nullptr) : compacted;
  }

  for (const std::string &category : categories) {
    json &container = this->protocol[category];
    if (!container.is_array()) continue;
    for (json &value : container) {
      if (!value.contains("args") || !value["args"].is_array()) continue;
      for (size_t arg = 0; arg < value["args"].size(); ++arg) {
        const std::string referenced_category = dependency_type(value, arg);
        if (referenced_category.empty() || !value["args"][arg].is_number_integer()) continue;
        const int old_index = value["args"][arg].get<int>();
        value["args"][arg] = remapped_indices[referenced_category].at(old_index);
      }
    }
  }

  json new_order = json::array();
  for (const json &entry : this->protocol["order"]) {
    const std::string category = entry[0];
    const int old_index = entry[1];
    const std::string key = category + ":" + std::to_string(old_index);
    if (std::find(deleted.begin(), deleted.end(), key) == deleted.end()) {
      new_order.push_back({category, remapped_indices[category].at(old_index)});
    }
  }
  this->protocol["order"] = new_order;
}

bool Protocol::has_searcher_objects() const {
  const std::vector<std::string> categories = {"Point", "Line", "Circle", "Conic", "Cubic", "Text"};
  for (const std::string &category : categories) {
    if (!this->protocol.contains(category)) continue;
    const json &objects = this->protocol[category];
    if (objects.is_array()) {
      for (const json &object : objects) {
        if (object.is_object() && object.value("searcher", false)) return true;
      }
    } else if (objects.is_object()) {
      for (const auto &[key, object] : objects.items()) {
        if (object.is_object() && object.value("searcher", false)) return true;
      }
    }
  }
  return false;
}

void Protocol::delete_searcher_objects() {
  const std::vector<std::string> categories = {"Point", "Line", "Circle", "Conic", "Cubic", "Text"};
  while (this->has_searcher_objects()) {
    bool removed = false;
    for (const std::string &category : categories) {
      const json &objects = this->protocol[category];
      int index = -1;
      if (objects.is_array()) {
        for (size_t i = 0; i < objects.size(); ++i) {
          if (objects[i].is_object() && objects[i].value("searcher", false)) {
            index = static_cast<int>(i);
            break;
          }
        }
      } else if (objects.is_object()) {
        for (const auto &[key, object] : objects.items()) {
          if (object.is_object() && object.value("searcher", false)) {
            try {
              index = std::stoi(key);
            } catch (...) {
            }
            break;
          }
        }
      }
      if (index != -1) {
        this->delete_obj(category, index);
        removed = true;
        break;
      }
    }
    if (!removed) break;
  }
}

