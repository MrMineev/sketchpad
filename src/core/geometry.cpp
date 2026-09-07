#include "geometry.h"
#include "alg.h"
#include "../../gui_tools/src/Gui/Gui.hpp"
#include "../parser/parse.h"

#include "../geo_genie/searcher.h"
#include "../geo_genie/property.h"

#include <string>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <unordered_map>

typedef long double ld;

using json = nlohmann::json; 

const long double EPS = 10;

const int CIRCLE_ASCII_LOC = 48;

static sf::Color protocol_color(const json &object, sf::Color fallback) {
  if (!object.contains("color") || !object["color"].is_array() || object["color"].size() < 3) {
    return fallback;
  }
  return sf::Color(
    object["color"][0].get<int>(), object["color"][1].get<int>(), object["color"][2].get<int>()
  );
}

struct AnnotationRun {
  sf::String value;
  unsigned int size;
  float offset;
  bool italic;
};

static std::string latex_group(const std::string &source, size_t &position) {
  if (position >= source.size()) return "";
  if (source[position] == '\\') {
    const size_t start = position++;
    while (position < source.size() && std::isalpha(static_cast<unsigned char>(source[position]))) ++position;
    return source.substr(start, position - start);
  }
  if (source[position] != '{') return std::string(1, source[position++]);
  ++position;
  const size_t start = position;
  int depth = 1;
  while (position < source.size() && depth > 0) {
    if (source[position] == '{') ++depth;
    if (source[position] == '}') --depth;
    ++position;
  }
  return source.substr(start, position - start - (depth == 0 ? 1 : 0));
}

static sf::String decode_latex(const std::string &source) {
  static const std::unordered_map<std::string, sf::Uint32> symbols = {
    {"alpha", 0x03B1}, {"beta", 0x03B2}, {"gamma", 0x03B3}, {"delta", 0x03B4},
    {"epsilon", 0x03B5}, {"theta", 0x03B8}, {"lambda", 0x03BB}, {"mu", 0x03BC},
    {"pi", 0x03C0}, {"rho", 0x03C1}, {"sigma", 0x03C3}, {"phi", 0x03C6},
    {"omega", 0x03C9}, {"Gamma", 0x0393}, {"Delta", 0x0394}, {"Theta", 0x0398},
    {"Lambda", 0x039B}, {"Sigma", 0x03A3}, {"Phi", 0x03A6}, {"Omega", 0x03A9},
    {"angle", 0x2220}, {"perp", 0x27C2}, {"parallel", 0x2225}, {"cdot", 0x00B7},
    {"times", 0x00D7}, {"neq", 0x2260}, {"leq", 0x2264}, {"geq", 0x2265},
    {"approx", 0x2248}, {"infty", 0x221E}, {"pm", 0x00B1}, {"circ", 0x00B0},
    {"sum", 0x2211}, {"prod", 0x220F}, {"int", 0x222B}, {"cup", 0x222A},
    {"cap", 0x2229}, {"subset", 0x2282}, {"in", 0x2208}, {"sim", 0x223C},
    {"cong", 0x2245}, {"rightarrow", 0x2192}, {"leftarrow", 0x2190},
    {"leftrightarrow", 0x2194}, {"Rightarrow", 0x21D2}, {"Leftrightarrow", 0x21D4}
  };
  sf::String result;
  for (size_t i = 0; i < source.size();) {
    if (source[i] != '\\') {
      if (source[i] != '{' && source[i] != '}') result += static_cast<sf::Uint32>(source[i]);
      ++i;
      continue;
    }
    ++i;
    const size_t command_start = i;
    while (i < source.size() && std::isalpha(static_cast<unsigned char>(source[i]))) ++i;
    const std::string command = source.substr(command_start, i - command_start);
    if (command == "frac") {
      const sf::String numerator = decode_latex(latex_group(source, i));
      const sf::String denominator = decode_latex(latex_group(source, i));
      result += "(";
      result += numerator;
      result += ")/(";
      result += denominator;
      result += ")";
    } else if (command == "sqrt") {
      result += static_cast<sf::Uint32>(0x221A);
      result += "(";
      result += decode_latex(latex_group(source, i));
      result += ")";
    } else if (command == "text") {
      result += sf::String(latex_group(source, i));
    } else {
      const auto symbol = symbols.find(command);
      if (symbol != symbols.end()) {
        result += symbol->second;
      } else if (!command.empty()) {
        result += sf::String(command);
      } else if (i < source.size()) {
        result += static_cast<sf::Uint32>(source[i++]);
      }
    }
  }
  return result;
}

static std::vector<AnnotationRun> math_runs(const std::string &source) {
  std::vector<AnnotationRun> runs;
  std::string regular;
  auto flush = [&]() {
    if (regular.empty()) return;
    runs.push_back({decode_latex(regular), 18, 0, true});
    regular.clear();
  };
  for (size_t i = 0; i < source.size();) {
    if (source[i] == '^' || source[i] == '_') {
      const char kind = source[i++];
      flush();
      runs.push_back({decode_latex(latex_group(source, i)), 13, kind == '^' ? -7.f : 8.f, true});
    } else {
      regular.push_back(source[i++]);
    }
  }
  flush();
  return runs;
}

static void append_stroke_segment(sf::VertexArray &vertices, sf::Vector2f start,
                                  sf::Vector2f end, float thickness, sf::Color color) {
  const sf::Vector2f direction = end - start;
  const float length = sqrt(direction.x * direction.x + direction.y * direction.y);
  if (length < 1e-5f) return;
  const sf::Vector2f normal(-direction.y / length * thickness / 2,
                            direction.x / length * thickness / 2);
  vertices.append(sf::Vertex(start + normal, color));
  vertices.append(sf::Vertex(end + normal, color));
  vertices.append(sf::Vertex(end - normal, color));
  vertices.append(sf::Vertex(start - normal, color));
}

static void append_styled_line(sf::VertexArray &vertices, sf::Vector2f start,
                               sf::Vector2f end, float thickness, sf::Color color,
                               bool dashed, float zoom) {
  const sf::Vector2f direction = end - start;
  const float length = sqrt(direction.x * direction.x + direction.y * direction.y);
  if (length < 1e-5f) return;
  if (!dashed) {
    append_stroke_segment(vertices, start, end, thickness, color);
    return;
  }
  const sf::Vector2f unit = direction / length;
  const float dash = 12 * zoom;
  const float gap = 8 * zoom;
  for (float offset = 0; offset < length; offset += dash + gap) {
    append_stroke_segment(
      vertices, start + unit * offset, start + unit * std::min(length, offset + dash),
      thickness, color
    );
  }
}

static std::vector<AnnotationRun> annotation_runs(const std::string &source) {
  std::vector<AnnotationRun> runs;
  size_t position = 0;
  while (position < source.size()) {
    const size_t math_start = source.find('$', position);
    if (math_start == std::string::npos) {
      runs.push_back({sf::String(source.substr(position)), 18, 0, false});
      break;
    }
    if (math_start > position) {
      runs.push_back({sf::String(source.substr(position, math_start - position)), 18, 0, false});
    }
    const size_t math_end = source.find('$', math_start + 1);
    if (math_end == std::string::npos) {
      runs.push_back({sf::String(source.substr(math_start)), 18, 0, false});
      break;
    }
    std::vector<AnnotationRun> parsed = math_runs(source.substr(math_start + 1, math_end - math_start - 1));
    runs.insert(runs.end(), parsed.begin(), parsed.end());
    position = math_end + 1;
  }
  if (runs.empty()) runs.push_back({sf::String(source), 18, 0, false});
  return runs;
}

void GeometryVisual::save_configuration(std::string &filepath) {
  std::ofstream file(filepath);
  std::string s = protocol.get_string_format();
  file << s << std::endl;
}

void GeometryVisual::load_configuration(std::string &filepath) {
  protocol.load_data(filepath);
  this->rebuild();
}

void GeometryVisual::rebuild() {
  this->protocol.ensure_metadata();
  this->lines.clear();
  this->circles.clear();
  this->points.clear();
  this->conics.clear();
  this->cubics.clear();
  this->texts.clear();

  for (auto &obj : protocol.get_order()) {
    string return_type = obj.first;
    int index = obj.second;
    // std::cout << "THIS IS " << return_type << " " << index << std::endl;

    json information_command = protocol.get_info(return_type, index);

    std::string command_type = information_command["func"].template get<std::string>();

    if (command_type == "newPoint") {
      ld px = information_command["location"][0];
      ld py = information_command["location"][1];
      this->points.push_back(GPoint(px, py));
    } else if (command_type == "newPointOnLine") {
      ld line_index = information_command["args"][0];
      ld point_ratio = information_command["args"][1];
      AlgGeom::Point p = AlgGeom::CoreGeometryTools::get_point_on_line_via_ratio(
        AlgGeom::Point(
          this->lines[line_index].x1,
          this->lines[line_index].y1
        ),
        AlgGeom::Point(
          this->lines[line_index].x2,
          this->lines[line_index].y2
        ),
        point_ratio
      );
      this->points.push_back(GPoint(p.x, p.y));
    } else if (command_type == "newPointOnCircle") {
      const int circle_index = information_command["args"][0];
      const ld angle = information_command["args"][1];
      this->points.push_back(GPoint(
        this->circles[circle_index].x_pos + this->circles[circle_index].radius * cos(angle),
        this->circles[circle_index].y_pos + this->circles[circle_index].radius * sin(angle)
      ));
    } else if (command_type == "newPointOnConic") {
      const int conic_index = information_command["args"][0];
      const GConic &conic = this->conics[conic_index];
      AlgGeom::Point point(information_command["location"][0], information_command["location"][1]);
      AlgGeom::Point projected;
      if (AlgGeom::CoreGeometryTools::project_point_to_conic(
            point, AlgGeom::Conic(conic.A, conic.B, conic.C, conic.D, conic.E, conic.F), projected)) {
        point = projected;
      }
      this->points.push_back(GPoint(point.x, point.y));
    } else if (command_type == "newLine") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      int p3 = information_command["version"];
      this->lines.push_back(
        GLine(
          this->points[p1].x_pos, this->points[p1].y_pos,
          this->points[p2].x_pos, this->points[p2].y_pos,
          p3
        )
      );
    } else if (command_type == "newCircle") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      this->circles.push_back(
        GCircle(
          this->points[p1].x_pos, this->points[p1].y_pos,
          AlgGeom::CoreGeometryTools::dist_points(
            AlgGeom::Point(this->points[p1].x_pos, this->points[p1].y_pos),
            AlgGeom::Point(this->points[p2].x_pos, this->points[p2].y_pos)
          )
        )
      );
    } else if (command_type == "midpoint") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      AlgGeom::Point new_loc = AlgGeom::CoreGeometryTools::midpoint(
        AlgGeom::Point(this->points[p1].x_pos, this->points[p1].y_pos),
        AlgGeom::Point(this->points[p2].x_pos, this->points[p2].y_pos)
      );
      this->points.push_back(GPoint(new_loc.x, new_loc.y));
    } else if (command_type == "circumcircle") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      int p3 = information_command["args"][2];
      AlgGeom::Line l1 = AlgGeom::CoreGeometryTools::perp_bisector(
        AlgGeom::Point(this->points[p1].x_pos, this->points[p1].y_pos),
        AlgGeom::Point(this->points[p2].x_pos, this->points[p2].y_pos)
      );
      AlgGeom::Line l2 = AlgGeom::CoreGeometryTools::perp_bisector(
        AlgGeom::Point(this->points[p2].x_pos, this->points[p2].y_pos),
        AlgGeom::Point(this->points[p3].x_pos, this->points[p3].y_pos)
      );
      AlgGeom::Point p = AlgGeom::CoreGeometryTools::inter_lines(l1, l2);
      this->circles.push_back(GCircle(
        p.x, p.y, AlgGeom::CoreGeometryTools::dist_points(
          p, AlgGeom::Point(this->points[p1].x_pos, this->points[p1].y_pos)
        )
      ));
    } else if (command_type == "interLL") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      AlgGeom::Line l1 = AlgGeom::Line(
        AlgGeom::Point(this->lines[p1].x1, this->lines[p1].y1),
        AlgGeom::Point(this->lines[p1].x2, this->lines[p1].y2)
      );
      AlgGeom::Line l2 = AlgGeom::Line(
        AlgGeom::Point(this->lines[p2].x1, this->lines[p2].y1),
        AlgGeom::Point(this->lines[p2].x2, this->lines[p2].y2)
      );
      AlgGeom::Point new_loc = AlgGeom::CoreGeometryTools::inter_lines(l1, l2);
      this->points.push_back(GPoint(new_loc.x, new_loc.y));
    } else if (command_type == "interLC") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      AlgGeom::Line l1 = AlgGeom::Line(
        AlgGeom::Point(this->lines[p1].x1, this->lines[p1].y1),
        AlgGeom::Point(this->lines[p1].x2, this->lines[p1].y2)
      );
      AlgGeom::Circle c2 = AlgGeom::Circle(
        AlgGeom::Point(this->circles[p2].x_pos, this->circles[p2].y_pos),
        this->circles[p2].radius
      );
      pair<AlgGeom::Point, AlgGeom::Point> new_loc = AlgGeom::CoreGeometryTools::inter_lc(l1, c2);
      this->points.push_back(
        GPoint(new_loc.first.x, new_loc.first.y)
      );
      this->points.push_back(
        GPoint(new_loc.second.x, new_loc.second.y)
      );
    } else if (command_type == "perpNormal") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      AlgGeom::Line l = AlgGeom::CoreGeometryTools::perp_normal(
        AlgGeom::Point(this->points[p1].x_pos, this->points[p1].y_pos),
        AlgGeom::Line(
          AlgGeom::Point(this->lines[p2].x1, this->lines[p2].y1),
          AlgGeom::Point(this->lines[p2].x2, this->lines[p2].y2)
        )
      );
      pair<pair<ld, ld>, pair<ld, ld>> gen = l.gen_points();
      this->lines.push_back(GLine(
        gen.first.first, gen.first.second, gen.second.first, gen.second.second, 1
      ));
    } else if (command_type == "parallel") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      AlgGeom::Line l = AlgGeom::CoreGeometryTools::parallel_line(
        AlgGeom::Point(this->points[p1].x_pos, this->points[p1].y_pos),
        AlgGeom::Line(
          AlgGeom::Point(this->lines[p2].x1, this->lines[p2].y1),
          AlgGeom::Point(this->lines[p2].x2, this->lines[p2].y2)
        )
      );
      pair<pair<ld, ld>, pair<ld, ld>> gen = l.gen_points();
      this->lines.push_back(GLine(
        gen.first.first, gen.first.second, gen.second.first, gen.second.second, 1
      ));
    } else if (command_type == "new_incenter") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      int p3 = information_command["args"][2];
      AlgGeom::Point new_loc = AlgGeom::CoreGeometryTools::incenter(
        AlgGeom::Point(this->points[p1].x_pos, this->points[p1].y_pos),
        AlgGeom::Point(this->points[p2].x_pos, this->points[p2].y_pos),
        AlgGeom::Point(this->points[p3].x_pos, this->points[p3].y_pos)
      );
      // this->points[p] = GPoint(new_loc.x, new_loc.y);
      this->points.push_back(GPoint(new_loc.x, new_loc.y));
    } else if (command_type == "newCircumcenter" || command_type == "newTriangleCenter") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      int p3 = information_command["args"][2];
      AlgGeom::Point new_loc;
      const bool valid = command_type == "newCircumcenter"
        ? AlgGeom::CoreGeometryTools::circumcenter(
            AlgGeom::Point(this->points[p1].x_pos, this->points[p1].y_pos),
            AlgGeom::Point(this->points[p2].x_pos, this->points[p2].y_pos),
            AlgGeom::Point(this->points[p3].x_pos, this->points[p3].y_pos), new_loc
          )
        : AlgGeom::CoreGeometryTools::triangle_center(
            information_command["args"][3],
            AlgGeom::Point(this->points[p1].x_pos, this->points[p1].y_pos),
            AlgGeom::Point(this->points[p2].x_pos, this->points[p2].y_pos),
            AlgGeom::Point(this->points[p3].x_pos, this->points[p3].y_pos), new_loc
          );
      if (!valid) new_loc = AlgGeom::Point(this->points[p1].x_pos, this->points[p1].y_pos);
      this->points.push_back(GPoint(new_loc.x, new_loc.y));
    } else if (command_type == "new_excenter") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      int p3 = information_command["args"][2];
      AlgGeom::Point new_loc = AlgGeom::CoreGeometryTools::excenter(
        AlgGeom::Point(this->points[p1].x_pos, this->points[p1].y_pos),
        AlgGeom::Point(this->points[p2].x_pos, this->points[p2].y_pos),
        AlgGeom::Point(this->points[p3].x_pos, this->points[p3].y_pos)
      );
      // this->points[p] = GPoint(new_loc.x, new_loc.y);
      this->points.push_back(GPoint(new_loc.x, new_loc.y));
    } else if (command_type == "newConic") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      int p3 = information_command["args"][2];
      int p4 = information_command["args"][3];
      int p5 = information_command["args"][4];
      AlgGeom::Conic c = AlgGeom::CoreGeometryTools::fitConicThrough5(
        AlgGeom::Point(this->points[p1].x_pos, this->points[p1].y_pos),
        AlgGeom::Point(this->points[p2].x_pos, this->points[p2].y_pos),
        AlgGeom::Point(this->points[p3].x_pos, this->points[p3].y_pos),
        AlgGeom::Point(this->points[p4].x_pos, this->points[p4].y_pos),
        AlgGeom::Point(this->points[p5].x_pos, this->points[p5].y_pos)
      );
      this->conics.push_back(GConic(c.a, c.b, c.c, c.d, c.e, c.f));
    } else if (command_type == "newRectangularHyperbola") {
      int center = information_command["args"][0];
      int p1 = information_command["args"][1];
      int p2 = information_command["args"][2];
      AlgGeom::Conic conic;
      if (AlgGeom::CoreGeometryTools::rectangular_hyperbola(
            convert_gpoint(this->points[center]), convert_gpoint(this->points[p1]),
            convert_gpoint(this->points[p2]), conic)) {
        this->conics.push_back(GConic(conic.a, conic.b, conic.c, conic.d, conic.e, conic.f));
      } else {
        this->conics.push_back(GConic(1, 0, -1, 0, 0, -1));
      }
    } else if (command_type == "newCubic") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      int p3 = information_command["args"][2];
      int p4 = information_command["args"][3];
      int p5 = information_command["args"][4];
      int p6 = information_command["args"][5];
      int p7 = information_command["args"][6];
      int p8 = information_command["args"][7];
      int p9 = information_command["args"][8];
      AlgGeom::Cubic c = AlgGeom::CoreGeometryTools::fitCubicThrough9(
        AlgGeom::Point(this->points[p1].x_pos, this->points[p1].y_pos),
        AlgGeom::Point(this->points[p2].x_pos, this->points[p2].y_pos),
        AlgGeom::Point(this->points[p3].x_pos, this->points[p3].y_pos),
        AlgGeom::Point(this->points[p4].x_pos, this->points[p4].y_pos),
        AlgGeom::Point(this->points[p5].x_pos, this->points[p5].y_pos),
        AlgGeom::Point(this->points[p6].x_pos, this->points[p6].y_pos),
        AlgGeom::Point(this->points[p7].x_pos, this->points[p7].y_pos),
        AlgGeom::Point(this->points[p8].x_pos, this->points[p8].y_pos),
        AlgGeom::Point(this->points[p9].x_pos, this->points[p9].y_pos)
      );
      this->cubics.push_back(GCubic(c.a, c.b, c.c, c.d, c.e, c.f, c.g, c.h, c.i, c.j));
    } else if (command_type == "newText") {
      this->texts.push_back(GTextAnnotation(
        information_command["location"][0], information_command["location"][1],
        information_command["content"]
      ));
    } else if (command_type == "newAngleBisector") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      int p3 = information_command["args"][2];
      AlgGeom::Line l = AlgGeom::CoreGeometryTools::angle_bisector(
        AlgGeom::Point(this->points[0].x_pos, this->points[0].y_pos),
        AlgGeom::Point(this->points[1].x_pos, this->points[1].y_pos),
        AlgGeom::Point(this->points[2].x_pos, this->points[2].y_pos)
      );
      pair<pair<ld, ld>, pair<ld, ld>> gen = l.gen_points();
      this->lines.push_back(GLine(
        gen.first.first, gen.first.second, gen.second.first, gen.second.second,
        1
      ));
    } else if (command_type == "newReflectLineOverLine") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      AlgGeom::Line l1 = AlgGeom::Line(
        AlgGeom::Point(this->lines[p1].x1, this->lines[p1].y1),
        AlgGeom::Point(this->lines[p1].x2, this->lines[p1].y2)
      );
      AlgGeom::Line l2 = AlgGeom::Line(
        AlgGeom::Point(this->lines[p2].x1, this->lines[p2].y1),
        AlgGeom::Point(this->lines[p2].x2, this->lines[p2].y2)
      );

      AlgGeom::Line l = AlgGeom::CoreGeometryTools::reflect_line_over_line(l1, l2);
      pair<pair<ld, ld>, pair<ld, ld>> gen = l.gen_points();
      this->lines.push_back(GLine(
        gen.first.first, gen.first.second, gen.second.first, gen.second.second, 1
      ));
    } else if (command_type == "newIsogonalConjugate") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      int p3 = information_command["args"][2];
      int p4 = information_command["args"][3];
      AlgGeom::Point new_loc = AlgGeom::CoreGeometryTools::isogonal_conjugate(
        AlgGeom::Point(this->points[p1].x_pos, this->points[p1].y_pos),
        AlgGeom::Point(this->points[p2].x_pos, this->points[p2].y_pos),
        AlgGeom::Point(this->points[p3].x_pos, this->points[p3].y_pos),
        AlgGeom::Point(this->points[p4].x_pos, this->points[p4].y_pos)
      );
      // this->points[p] = GPoint(new_loc.x, new_loc.y);
      this->points.push_back(GPoint(new_loc.x, new_loc.y));
    } else if (command_type == "newReflectPointOverLine") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      AlgGeom::Point p = AlgGeom::CoreGeometryTools::reflect_point_over_line(
        AlgGeom::Point(this->points[p1].x_pos, this->points[p1].y_pos),
        AlgGeom::Line(
          AlgGeom::Point(this->lines[p2].x1, this->lines[p2].y1),
          AlgGeom::Point(this->lines[p2].x2, this->lines[p2].y2)
        )
      );
      this->points.push_back(GPoint(
        p.x, p.y
      ));
    } else if (command_type == "newProjectPointOntoLine") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      AlgGeom::Point p = AlgGeom::CoreGeometryTools::project_point_to_line(
        AlgGeom::Point(this->points[p1].x_pos, this->points[p1].y_pos),
        AlgGeom::Line(
          AlgGeom::Point(this->lines[p2].x1, this->lines[p2].y1),
          AlgGeom::Point(this->lines[p2].x2, this->lines[p2].y2)
        )
      );
      this->points.push_back(GPoint(p.x, p.y));
    } else if (command_type == "newReflectPointOverPoint") {
      int p1 = information_command["args"][0];
      int p2 = information_command["args"][1];
      AlgGeom::Point p = AlgGeom::CoreGeometryTools::reflect_point_over_point(
        AlgGeom::Point(this->points[p1].x_pos, this->points[p1].y_pos),
        AlgGeom::Point(this->points[p2].x_pos, this->points[p2].y_pos)
      );
      this->points.push_back(GPoint(p.x, p.y));
    } else if (command_type == "newCenter") {
      const int source = information_command["args"][0];
      const std::string source_type = information_command["source_type"];
      AlgGeom::Point center;
      bool valid = false;
      if (source_type == "Circle") {
        center = AlgGeom::Point(this->circles[source].x_pos, this->circles[source].y_pos);
        valid = true;
      } else if (source_type == "Conic") {
        const GConic &conic = this->conics[source];
        valid = AlgGeom::CoreGeometryTools::conic_center(
          AlgGeom::Conic(conic.A, conic.B, conic.C, conic.D, conic.E, conic.F), center
        );
      }
      if (!valid) center = AlgGeom::Point(0, 0);
      this->points.push_back(GPoint(center.x, center.y));
    } else {
      std::cerr << "[ERROR]: unknown command type => " << command_type << std::endl;
    }
  }

  for (int i = 0; i < this->points.size(); ++i) {
    this->points[i].index = i;
    const json &point = this->protocol.protocol["Point"][i];
    this->points[i].label = point.value("label", "");
    this->points[i].visible = point.value("visible", true);
    this->points[i].label_visible = point.value("label_visible", true);
    this->points[i].color = protocol_color(point, sf::Color::Red);
    this->points[i].thickness = point.value("thickness", 1.f);
  }
  for (int i = 0; i < this->lines.size(); ++i) {
    this->lines[i].index = i;
    const json &line = this->protocol.protocol["Line"][i];
    this->lines[i].visible = line.value("visible", true);
    this->lines[i].color = protocol_color(line, sf::Color::Blue);
    this->lines[i].thickness = line.value("thickness", 1.f);
    this->lines[i].dashed = line.value("dashed", false);
  }
  for (int i = 0; i < this->circles.size(); ++i) {
    this->circles[i].index = i;
    const json &circle = this->protocol.protocol["Circle"][i];
    this->circles[i].visible = circle.value("visible", true);
    this->circles[i].color = protocol_color(circle, sf::Color::Green);
    this->circles[i].thickness = circle.value("thickness", 1.f);
    this->circles[i].dashed = circle.value("dashed", false);
  }
  for (int i = 0; i < this->conics.size(); ++i) {
    this->conics[i].index = i;
    const json &conic = this->protocol.protocol["Conic"][i];
    this->conics[i].visible = conic.value("visible", true);
    this->conics[i].color = protocol_color(conic, sf::Color::Cyan);
    this->conics[i].thickness = conic.value("thickness", 1.f);
    this->conics[i].dashed = conic.value("dashed", false);
  }
  for (int i = 0; i < this->texts.size(); ++i) {
    this->texts[i].index = i;
    this->texts[i].visible = this->protocol.protocol["Text"][i].value("visible", true);
  }
}

void GeometryVisual::delete_object(std::string type, int index) {
  this->protocol.delete_obj(type, index);
  this->live_stack.clear();
  this->live_stack_lines.clear();
  this->live_stack_circles.clear();
  this->selected_point = -1;
  this->selected_line = -1;
  this->selected_circle = -1;
  this->selected_text = -1;
  this->follower = -1;
  this->text_follower = -1;
  this->isDragging = false;
  this->isDraggingText = false;
  this->refresh_geo_genie_on_release = false;
  this->rebuild();
}

void GeometryVisual::delete_point(int index) {
  if (index < 0 || index >= this->points.size()) return;
  this->delete_object("Point", index);
}

void GeometryVisual::delete_line(int index) {
  if (index < 0 || index >= this->lines.size()) return;
  this->delete_object("Line", index);
}

void GeometryVisual::delete_circle(int index) {
  if (index < 0 || index >= this->circles.size()) return;
  this->delete_object("Circle", index);
}

void GeometryVisual::hide_geo_genie() {
  this->protocol.delete_searcher_objects();
  this->live_stack.clear();
  this->live_stack_lines.clear();
  this->live_stack_circles.clear();
  this->selected_point = -1;
  this->selected_line = -1;
  this->selected_circle = -1;
  this->selected_text = -1;
  this->follower = -1;
  this->text_follower = -1;
  this->isDragging = false;
  this->isDraggingText = false;
  this->refresh_geo_genie_on_release = false;
  this->rebuild();
}

int GeometryVisual::take_inversion_request() {
  const int request = this->inversion_circle_request;
  this->inversion_circle_request = -1;
  return request;
}

int GeometryVisual::take_rename_point_request() {
  const int request = this->rename_point_request;
  this->rename_point_request = -1;
  return request;
}

bool GeometryVisual::take_text_request(sf::Vector2f &position) {
  if (!this->text_request_pending) return false;
  position = this->text_request_position;
  this->text_request_pending = false;
  return true;
}

bool GeometryVisual::take_style_request(ObjectStyleRequest &request) {
  if (!this->style_request_pending) return false;
  request = this->style_request;
  this->style_request_pending = false;
  return true;
}

std::vector<int> GeometryVisual::take_triangle_center_request() {
  std::vector<int> request = this->triangle_center_request;
  this->triangle_center_request.clear();
  return request;
}

void GeometryVisual::show_all() {
  this->protocol.show_all();
  this->current_tool = 0;
  this->rebuild();
}

void GeometryVisual::build_inversion(const GeometryVisual &source, int inversion_circle) {
  this->points.clear();
  this->lines.clear();
  this->circles.clear();
  this->conics.clear();
  this->cubics.clear();
  this->texts.clear();
  if (inversion_circle < 0 || inversion_circle >= source.circles.size()) return;

  const GCircle &base = source.circles[inversion_circle];
  const AlgGeom::Point center(base.x_pos, base.y_pos);
  const ld radius_squared = static_cast<ld>(base.radius) * base.radius;
  if (radius_squared < AlgGeom::EPS) return;

  auto is_searcher = [&](const std::string &category, int index) {
    if (!source.protocol.protocol.contains(category)) return false;
    const json &objects = source.protocol.protocol[category];
    return objects.is_array() && index >= 0 && index < objects.size() && objects[index].is_object() &&
           objects[index].value("searcher", false);
  };
  auto add_line = [&](AlgGeom::Point point, AlgGeom::Point direction, bool visible,
                      sf::Color color, float thickness, bool dashed) {
    const ld length = sqrt(direction.x * direction.x + direction.y * direction.y);
    if (length < AlgGeom::EPS) return;
    direction.x /= length;
    direction.y /= length;
    GLine line(
      point.x - direction.x * 5000, point.y - direction.y * 5000,
      point.x + direction.x * 5000, point.y + direction.y * 5000, 0
    );
    line.line_type = 1;
    line.visible = visible;
    line.color = color;
    line.thickness = thickness;
    line.dashed = dashed;
    line.index = this->lines.size();
    this->lines.push_back(line);
  };

  for (int i = 0; i < source.points.size(); ++i) {
    if (is_searcher("Point", i)) continue;
    const AlgGeom::Point point(source.points[i].x_pos, source.points[i].y_pos);
    if (AlgGeom::CoreGeometryTools::dist_points(point, center) < AlgGeom::EPS) continue;
    const AlgGeom::Point inverted = AlgGeom::CoreGeometryTools::inversion_point(
      point, AlgGeom::Circle(center, base.radius)
    );
    GPoint result(inverted.x, inverted.y);
    result.label = source.points[i].label;
    result.visible = source.points[i].visible;
    result.label_visible = source.points[i].label_visible;
    result.color = source.points[i].color;
    result.thickness = source.points[i].thickness;
    result.index = this->points.size();
    this->points.push_back(result);
  }

  for (int i = 0; i < source.lines.size(); ++i) {
    if (source.lines[i].line_type == 0 || is_searcher("Line", i)) continue;
    const AlgGeom::Line line(
      AlgGeom::Point(source.lines[i].x1, source.lines[i].y1),
      AlgGeom::Point(source.lines[i].x2, source.lines[i].y2)
    );
    const ld norm = sqrt(line.a * line.a + line.b * line.b);
    if (norm < AlgGeom::EPS) continue;
    const ld distance = abs(line.a * center.x + line.b * center.y + line.c) / norm;
    if (distance < AlgGeom::EPS) {
      add_line(
        center, AlgGeom::Point(-line.b, line.a), source.lines[i].visible,
        source.lines[i].color, source.lines[i].thickness, source.lines[i].dashed
      );
    } else {
      const AlgGeom::Point closest = AlgGeom::CoreGeometryTools::project_point_to_line(center, line);
      const AlgGeom::Point image = AlgGeom::CoreGeometryTools::inversion_point(
        closest, AlgGeom::Circle(center, base.radius)
      );
      const AlgGeom::Point inverted_center = AlgGeom::CoreGeometryTools::midpoint(center, image);
      this->circles.push_back(GCircle(
        inverted_center.x, inverted_center.y,
        AlgGeom::CoreGeometryTools::dist_points(inverted_center, center)
      ));
      this->circles.back().visible = source.lines[i].visible;
      this->circles.back().color = source.lines[i].color;
      this->circles.back().thickness = source.lines[i].thickness;
      this->circles.back().dashed = source.lines[i].dashed;
      this->circles.back().index = this->circles.size() - 1;
    }
  }

  for (int i = 0; i < source.circles.size(); ++i) {
    if (is_searcher("Circle", i)) continue;
    const AlgGeom::Point offset(
      source.circles[i].x_pos - center.x,
      source.circles[i].y_pos - center.y
    );
    const ld source_radius = source.circles[i].radius;
    const ld denominator = offset.x * offset.x + offset.y * offset.y - source_radius * source_radius;
    if (abs(denominator) < AlgGeom::EPS) {
      const ld offset_squared = offset.x * offset.x + offset.y * offset.y;
      if (offset_squared < AlgGeom::EPS) continue;
      const AlgGeom::Point point(
        center.x + offset.x * radius_squared / (2 * offset_squared),
        center.y + offset.y * radius_squared / (2 * offset_squared)
      );
      add_line(
        point, AlgGeom::Point(-offset.y, offset.x), source.circles[i].visible,
        source.circles[i].color, source.circles[i].thickness, source.circles[i].dashed
      );
    } else {
      const ld factor = radius_squared / denominator;
      this->circles.push_back(GCircle(
        center.x + factor * offset.x,
        center.y + factor * offset.y,
        abs(factor) * source_radius
      ));
      this->circles.back().visible = source.circles[i].visible;
      this->circles.back().color = source.circles[i].color;
      this->circles.back().thickness = source.circles[i].thickness;
      this->circles.back().dashed = source.circles[i].dashed;
      this->circles.back().index = this->circles.size() - 1;
    }
  }
}

int GeometryVisual::conic_searcher(GPoint point) const {
  const ld tolerance = EPS * this->camera_zoom;
  for (int i = 0; i < this->conics.size(); ++i) {
    const GConic &conic = this->conics[i];
    if (!conic.visible) continue;
    const ld value = conic.A * point.x_pos * point.x_pos + conic.B * point.x_pos * point.y_pos +
                     conic.C * point.y_pos * point.y_pos + conic.D * point.x_pos +
                     conic.E * point.y_pos + conic.F;
    const ld gradient_x = 2 * conic.A * point.x_pos + conic.B * point.y_pos + conic.D;
    const ld gradient_y = conic.B * point.x_pos + 2 * conic.C * point.y_pos + conic.E;
    const ld gradient = sqrt(gradient_x * gradient_x + gradient_y * gradient_y);
    if (gradient > AlgGeom::EPS && abs(value) / gradient <= tolerance) return i;
  }
  return -1;
}

int GeometryVisual::text_searcher(GPoint point) const {
  for (int i = static_cast<int>(this->texts.size()) - 1; i >= 0; --i) {
    const GTextAnnotation &text = this->texts[i];
    if (!text.visible) continue;
    if (point.x_pos >= text.bounds_left - 5 &&
        point.x_pos <= text.bounds_left + text.bounds_width + 5 &&
        point.y_pos >= text.bounds_top - 5 &&
        point.y_pos <= text.bounds_top + text.bounds_height + 5) return i;
  }
  return -1;
}

void GeometryVisual::initialize_camera(const sf::RenderWindow &window) {
  if (this->camera_initialized) return;
  this->camera_view = window.getView();
  this->camera_initialized = true;
}

void GeometryVisual::resize_camera(unsigned int width, unsigned int height) {
  if (!this->camera_initialized || width == 0 || height == 0) return;
  const sf::Vector2f old_size = this->camera_view.getSize();
  const sf::Vector2f top_left = this->camera_view.getCenter() - old_size / 2.f;
  const sf::Vector2f new_size(width * this->camera_zoom, height * this->camera_zoom);
  this->camera_view.setSize(new_size);
  this->camera_view.setCenter(top_left + new_size / 2.f);
}

bool GeometryVisual::handleCameraEvent(const sf::Event& event, sf::RenderWindow& window) {
  this->initialize_camera(window);

  if (event.type == sf::Event::MouseWheelScrolled) {
    const sf::Vector2i pixel(event.mouseWheelScroll.x, event.mouseWheelScroll.y);
    if (pixel.x <= this->X_MENU_BORDER || pixel.y <= 36) return false;
    const sf::Vector2f before = window.mapPixelToCoords(pixel, this->camera_view);
    const float requested_zoom = this->camera_zoom * std::pow(0.9f, event.mouseWheelScroll.delta);
    const float new_zoom = std::clamp(requested_zoom, 0.1f, 10.f);
    this->camera_view.zoom(new_zoom / this->camera_zoom);
    this->camera_zoom = new_zoom;
    const sf::Vector2f after = window.mapPixelToCoords(pixel, this->camera_view);
    this->camera_view.move(before - after);
    return true;
  }

  if (event.type == sf::Event::MouseWheelMoved) {
    const sf::Vector2i pixel(event.mouseWheel.x, event.mouseWheel.y);
    if (pixel.x <= this->X_MENU_BORDER || pixel.y <= 36) return false;
    const sf::Vector2f before = window.mapPixelToCoords(pixel, this->camera_view);
    const float requested_zoom = this->camera_zoom * std::pow(0.9f, event.mouseWheel.delta);
    const float new_zoom = std::clamp(requested_zoom, 0.1f, 10.f);
    this->camera_view.zoom(new_zoom / this->camera_zoom);
    this->camera_zoom = new_zoom;
    const sf::Vector2f after = window.mapPixelToCoords(pixel, this->camera_view);
    this->camera_view.move(before - after);
    return true;
  }

  if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left &&
      this->current_tool == 0 && event.mouseButton.x > this->X_MENU_BORDER && event.mouseButton.y > 36) {
    if (this->protocol.has_searcher_objects() && event.mouseButton.x >= window.getSize().x - 44 &&
        event.mouseButton.y >= 48 && event.mouseButton.y <= 80) return false;
    const sf::Vector2f world = window.mapPixelToCoords(
      sf::Vector2i(event.mouseButton.x, event.mouseButton.y), this->camera_view
    );
    const GPoint world_point(world.x, world.y);
    const auto result = this->point_searcher(world_point);
    const auto indexes = result.first;
    const int conic_index = this->conic_searcher(world_point);
    const int text_index = this->text_searcher(world_point);
    if (indexes.first == -1 && indexes.second.first == -1 && indexes.second.second == -1 &&
        conic_index == -1 && text_index == -1) {
      this->isPanning = true;
      this->pan_last_pixel = sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
      this->selected_point = -1;
      this->selected_line = -1;
      this->selected_circle = -1;
      return true;
    }
  }

  if (event.type == sf::Event::MouseMoved && this->isPanning) {
    const sf::Vector2i pixel(event.mouseMove.x, event.mouseMove.y);
    const sf::Vector2f previous = window.mapPixelToCoords(this->pan_last_pixel, this->camera_view);
    const sf::Vector2f current = window.mapPixelToCoords(pixel, this->camera_view);
    this->camera_view.move(previous - current);
    this->pan_last_pixel = pixel;
    return true;
  }

  if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left &&
      this->isPanning) {
    this->isPanning = false;
    return true;
  }
  return false;
}

pair<pair<int, pair<int, int>>, GPoint> GeometryVisual::point_searcher(GPoint p) {
  const ld selection_epsilon = EPS * this->camera_zoom;
  for (int i = 0; i < this->points.size(); i++) {
    if (!this->points[i].visible) continue;
    if (AlgGeom::CoreGeometryTools::dist_points(
      AlgGeom::Point(p.x_pos, p.y_pos),
      AlgGeom::Point(this->points[i].x_pos, this->points[i].y_pos)
    ) <= selection_epsilon) {
      return make_pair(make_pair(i, make_pair(-1, -1)), this->points[i]);
    }
  }
  for (int i = 0; i < this->lines.size(); i++) {
    if (!this->lines[i].visible) continue;
    // TODO DIFFERENTIATE BETWEEN LINE AND SEGMENT
    const ld dx = this->lines[i].x2 - this->lines[i].x1;
    const ld dy = this->lines[i].y2 - this->lines[i].y1;
    const ld length_squared = dx * dx + dy * dy;
    if (length_squared == 0) continue;
    const ld projection = ((p.x_pos - this->lines[i].x1) * dx +
                           (p.y_pos - this->lines[i].y1) * dy) / length_squared;
    if (this->lines[i].line_type == 0 && (projection < 0 || projection > 1)) continue;
    if (AlgGeom::CoreGeometryTools::dist_point_to_line(
      AlgGeom::Point(p.x_pos, p.y_pos),
      AlgGeom::Line(
        AlgGeom::Point(this->lines[i].x1, this->lines[i].y1),
        AlgGeom::Point(this->lines[i].x2, this->lines[i].y2)
      )
    ) <= selection_epsilon) {
      AlgGeom::Point new_loc = AlgGeom::CoreGeometryTools::project_point_to_line(
        AlgGeom::Point(p.x_pos, p.y_pos),
        AlgGeom::Line(
          AlgGeom::Point(this->lines[i].x1, this->lines[i].y1),
          AlgGeom::Point(this->lines[i].x2, this->lines[i].y2)
        )
      );
      return make_pair(make_pair(-1, make_pair(i, -1)), GPoint(new_loc.x, new_loc.y));
    }
  }
  for (int i = 0; i < this->circles.size(); i++) {
    if (!this->circles[i].visible) continue;
    ld dist = abs(this->circles[i].radius - AlgGeom::CoreGeometryTools::dist_points(
      AlgGeom::Point(p.x_pos, p.y_pos),
      AlgGeom::Point(
        this->circles[i].x_pos, this->circles[i].y_pos
      )
    ));
    if (dist <= selection_epsilon) {
      AlgGeom::Point new_loc = AlgGeom::CoreGeometryTools::project_point_to_circle(
        AlgGeom::Point(p.x_pos, p.y_pos),
        AlgGeom::Circle(
          AlgGeom::Point(this->circles[i].x_pos, this->circles[i].y_pos),
          this->circles[i].radius
        )
      );
      return make_pair(make_pair(-1, make_pair(-1, i)), GPoint(new_loc.x, new_loc.y));
    }
  }
  return make_pair(make_pair(-1, make_pair(-1, -1)), p);
}

void GeometryVisual::handleEvent(const sf::Event& event, sf::RenderWindow& window, gui::Menu& menu) {
  if (event.type == sf::Event::MouseButtonPressed) {
    const float close_x = window.getSize().x - 44.f;
    if (event.mouseButton.button == sf::Mouse::Left && this->protocol.has_searcher_objects() &&
        event.mouseButton.x >= close_x && event.mouseButton.x <= close_x + 32 &&
        event.mouseButton.y >= 48 && event.mouseButton.y <= 80) {
      this->hide_geo_genie();
      return;
    }
    if (event.mouseButton.x <= this->X_MENU_BORDER) {
      this->selected_point = -1;
      this->selected_line = -1;
      this->selected_circle = -1;
      this->selected_text = -1;
      this->isDragging = false;
      this->isDraggingText = false;
      this->refresh_geo_genie_on_release = false;
      return;
    }

    this->initialize_camera(window);
    const sf::Vector2f world = window.mapPixelToCoords(
      sf::Vector2i(event.mouseButton.x, event.mouseButton.y), this->camera_view
    );
    GPoint _p(world.x, world.y);
    auto [_indexes, p] = this->point_searcher(_p);
    auto [index_search, _object_search] = _indexes;
    auto [line_search, circle_search] = _object_search;
    const int conic_search = this->conic_searcher(_p);
    const int text_search = this->text_searcher(_p);

    if (event.mouseButton.button == sf::Mouse::Right) {
      if (index_search != -1) {
        this->style_request = {"Point", index_search, {event.mouseButton.x, event.mouseButton.y}};
      } else if (line_search != -1) {
        this->style_request = {"Line", line_search, {event.mouseButton.x, event.mouseButton.y}};
      } else if (circle_search != -1) {
        this->style_request = {"Circle", circle_search, {event.mouseButton.x, event.mouseButton.y}};
      } else if (conic_search != -1) {
        this->style_request = {"Conic", conic_search, {event.mouseButton.x, event.mouseButton.y}};
      } else {
        return;
      }
      this->style_request_pending = true;
      return;
    }

    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 0) {
      this->selected_text = text_search;
      this->selected_point = text_search == -1 ? index_search : -1;
      this->selected_line = text_search == -1 && index_search == -1 ? line_search : -1;
      this->selected_circle = text_search == -1 && index_search == -1 && line_search == -1
        ? circle_search : -1;
      if (text_search != -1) {
        this->isDraggingText = true;
        this->text_follower = text_search;
        this->text_drag_offset = sf::Vector2f(
          world.x - this->texts[text_search].x, world.y - this->texts[text_search].y
        );
        this->isDragging = false;
        this->follower = -1;
      } else if (index_search != -1 &&
                 (this->protocol.is_point_def_by_func(index_search, "newPoint") ||
                  this->protocol.is_point_def_by_func(index_search, "newPointOnLine") ||
                  this->protocol.is_point_def_by_func(index_search, "newPointOnCircle") ||
                  this->protocol.is_point_def_by_func(index_search, "newPointOnConic"))) {
        this->isDragging = true;
        this->follower = index_search;
        this->isDraggingText = false;
        this->text_follower = -1;
        this->refresh_geo_genie_on_release = this->protocol.has_searcher_objects();
        if (this->refresh_geo_genie_on_release) {
          this->protocol.delete_searcher_objects();
          this->rebuild();
        }
      } else {
        this->isDragging = false;
        this->follower = -1;
        this->isDraggingText = false;
        this->text_follower = -1;
        this->refresh_geo_genie_on_release = false;
      }
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 1) {
      if (index_search == -1) {
        if (line_search != -1) {
          this->points.push_back(p);
          ld ratio_value = AlgGeom::CoreGeometryTools::get_point_on_line_ratio(
            AlgGeom::Point(
              this->lines[line_search].x1,
              this->lines[line_search].y1
            ),
            AlgGeom::Point(
              this->lines[line_search].x2,
              this->lines[line_search].y2
            ),
            AlgGeom::Point(p.x_pos, p.y_pos)
          );
          protocol.new_point_on_line(this->points.size() - 1, line_search, ratio_value);
        } else if (circle_search != -1) {
          this->points.push_back(p);
          const ld angle = atan2(
            p.y_pos - this->circles[circle_search].y_pos,
            p.x_pos - this->circles[circle_search].x_pos
          );
          protocol.new_point_on_circle(this->points.size() - 1, circle_search, angle);
        } else if (conic_search != -1) {
          const GConic &conic = this->conics[conic_search];
          AlgGeom::Point projected;
          if (AlgGeom::CoreGeometryTools::project_point_to_conic(
                AlgGeom::Point(world.x, world.y),
                AlgGeom::Conic(conic.A, conic.B, conic.C, conic.D, conic.E, conic.F), projected)) {
            this->points.push_back(GPoint(projected.x, projected.y));
            protocol.new_point_on_conic(
              this->points.size() - 1, conic_search, projected.x, projected.y
            );
          }
        } else {
          this->points.push_back(p);
          protocol.new_point(this->points.size() - 1, p.x_pos, p.y_pos);
        }
      }
    }
    
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 2) {
      p.index = index_search;
      if (index_search == -1) {
        p.index = this->points.size();
        this->points.push_back(p);
        // protocol += new_point(this->points.size() - 1, p.x_pos, p.y_pos);
        protocol.new_point(this->points.size() - 1, p.x_pos, p.y_pos);
      }
      this->live_stack.push_back(p);
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 3) {
      p.index = index_search;
      if (index_search == -1) {
        p.index = this->points.size();
        this->points.push_back(p);
        // protocol += new_point(this->points.size() - 1, p.x_pos, p.y_pos);
        protocol.new_point(this->points.size() - 1, p.x_pos, p.y_pos);
      }
      this->live_stack.push_back(p);
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 4) {
      p.index = index_search;
      if (index_search == -1) {
        p.index = this->points.size();
        this->points.push_back(p);
        // protocol += new_point(this->points.size() - 1, p.x_pos, p.y_pos);
        protocol.new_point(this->points.size() - 1, p.x_pos, p.y_pos);
      }
      this->live_stack.push_back(p);
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 5) {
      p.index = index_search;
      if (index_search == -1) {
        p.index = this->points.size();
        this->points.push_back(p);
        // protocol += new_point(this->points.size() - 1, p.x_pos, p.y_pos);
        protocol.new_point(this->points.size() - 1, p.x_pos, p.y_pos);
      }
      this->live_stack.push_back(p);
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 6) {
      p.index = index_search;
      if (index_search == -1) {
        p.index = this->points.size();
        this->points.push_back(p);
        // protocol += new_point(this->points.size() - 1, p.x_pos, p.y_pos);
        protocol.new_point(this->points.size() - 1, p.x_pos, p.y_pos);
      }
      this->live_stack.push_back(p);
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 7) {
      this->live_stack_lines.push_back(this->lines[line_search]);
      this->live_stack_lines[this->live_stack_lines.size() - 1].index = line_search;
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 8) {
      if (this->live_stack_lines.size() == 1) {
        p.index = index_search;
        this->live_stack.push_back(p);
      } else {
        if (line_search != -1) {
          this->lines[line_search].index = line_search;
          this->live_stack_lines.push_back(this->lines[line_search]);
        } else {
          p.index = index_search;
          this->live_stack.push_back(p);
        }
      }
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 9 && circle_search != -1) {
      this->inversion_circle_request = circle_search;
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 10) {
      p.index = index_search;
      this->live_stack.push_back(p);
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 11) {
      p.index = index_search;
      this->live_stack.push_back(p);
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 12) {
      if (this->live_stack_lines.size() == 1) {
        this->live_stack_circles.push_back(this->circles[circle_search]);
        this->live_stack_circles[this->live_stack_circles.size() - 1].index = circle_search;
      } else {
        if (line_search != -1) {
          this->lines[line_search].index = line_search;
          this->live_stack_lines.push_back(this->lines[line_search]);
        } else {
          this->live_stack_circles.push_back(this->circles[circle_search]);
          this->live_stack_circles[this->live_stack_circles.size() - 1].index = circle_search;
        }
      }
    }

    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 13) {
      GeoProp::GeoProperties something = GeoGenie::search(&protocol);
      rebuild();
    }

    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 14) {
      if (this->live_stack_lines.size() == 1) {
        p.index = index_search;
        this->live_stack.push_back(p);
      } else {
        if (line_search != -1) {
          this->lines[line_search].index = line_search;
          this->live_stack_lines.push_back(this->lines[line_search]);
        } else {
          p.index = index_search;
          this->live_stack.push_back(p);
        }
      }
    }

    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 15) {
      if (this->live_stack.size() <= 5) {
        p.index = index_search;
        this->live_stack.push_back(p);
      }
    }

    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 16) {
      

      if (this->live_stack.size() <= 9) {
        p.index = index_search;
        this->live_stack.push_back(p);
      }
    }

    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 17) {
      if (this->live_stack.size() <= 3) {
        p.index = index_search;
        this->live_stack.push_back(p);
      }
    }

    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 18) {
      if (this->live_stack_lines.size() <= 2) {
         if (line_search != -1) {
          this->lines[line_search].index = line_search;
          this->live_stack_lines.push_back(this->lines[line_search]);
         }
      }
    }

    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 19) {
      if (this->live_stack.size() <= 4) {
        p.index = index_search;
        this->live_stack.push_back(p);
      }
    }

    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 20) {
      cout << "click!" << endl;
      if (this->live_stack_lines.size() <= 1) {
        if (line_search != -1) {
          cout << "added line!" << endl;
          this->lines[line_search].index = line_search;
          this->live_stack_lines.push_back(this->lines[line_search]);
        } else if (this->live_stack.size() <= 1) {
          cout << "added point!" << endl;
          p.index = index_search;
          this->live_stack.push_back(p);
        }
      }
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 21 && index_search != -1 &&
        this->live_stack.size() < 2) {
      p.index = index_search;
      this->live_stack.push_back(p);
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 22) {
      if (line_search != -1 && this->live_stack_lines.empty()) {
        this->lines[line_search].index = line_search;
        this->live_stack_lines.push_back(this->lines[line_search]);
      } else if (index_search != -1 && this->live_stack.empty()) {
        p.index = index_search;
        this->live_stack.push_back(p);
      }
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 23 && index_search != -1 &&
        this->live_stack.size() < 3) {
      p.index = index_search;
      this->live_stack.push_back(p);
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 24 && index_search != -1 &&
        this->live_stack.size() < 3) {
      p.index = index_search;
      this->live_stack.push_back(p);
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 25 && index_search != -1 &&
        this->live_stack.size() < 3) {
      p.index = index_search;
      this->live_stack.push_back(p);
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 26) {
      AlgGeom::Point center;
      std::string source_type;
      int source = -1;
      if (circle_search != -1) {
        center = AlgGeom::Point(this->circles[circle_search].x_pos, this->circles[circle_search].y_pos);
        source_type = "Circle";
        source = circle_search;
      } else if (conic_search != -1) {
        const GConic &conic = this->conics[conic_search];
        if (AlgGeom::CoreGeometryTools::conic_center(
              AlgGeom::Conic(conic.A, conic.B, conic.C, conic.D, conic.E, conic.F), center)) {
          source_type = "Conic";
          source = conic_search;
        }
      }
      if (source != -1) {
        this->points.push_back(GPoint(center.x, center.y));
        this->protocol.new_center(this->points.size() - 1, source_type, source);
      }
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 27) {
      this->text_request_position = world;
      this->text_request_pending = true;
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 28) {
      if (text_search != -1) {
        this->protocol.set_visibility("Text", text_search, false);
      } else if (index_search != -1) {
        this->protocol.set_visibility("Point", index_search, false);
      } else if (line_search != -1) {
        this->protocol.set_visibility("Line", line_search, false);
      } else if (circle_search != -1) {
        this->protocol.set_visibility("Circle", circle_search, false);
      } else if (conic_search != -1) {
        this->protocol.set_visibility("Conic", conic_search, false);
      }
      this->selected_point = -1;
      this->selected_line = -1;
      this->selected_circle = -1;
      this->selected_text = -1;
      this->rebuild();
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 29 && index_search != -1) {
      this->protocol.set_point_label_visibility(index_search, false);
      this->rebuild();
    }
    if (event.mouseButton.button == sf::Mouse::Left && this->current_tool == 30 && index_search != -1) {
      this->rename_point_request = index_search;
    }
  }
  if (event.type == sf::Event::MouseButtonReleased) {
    if (event.mouseButton.button == sf::Mouse::Left) {
      isDragging = false;
      this->isDraggingText = false;
      this->text_follower = -1;
      if (this->refresh_geo_genie_on_release) {
        GeoGenie::search(&this->protocol);
        this->refresh_geo_genie_on_release = false;
        this->rebuild();
      }
    }
  }

  if (isDragging) {
    const sf::Vector2i mousePixel = sf::Mouse::getPosition(window);
    const sf::Vector2f mousePos = window.mapPixelToCoords(mousePixel, this->camera_view);

    if (this->protocol.is_point_def_by_func(follower, "newPointOnLine")) {
      int line_index = this->protocol.get_point_info(follower)["args"][0];
      AlgGeom::Point new_loc = AlgGeom::CoreGeometryTools::project_point_to_line(
        AlgGeom::Point(mousePos.x, mousePos.y),
        AlgGeom::Line(
          AlgGeom::Point(this->lines[line_index].x1, this->lines[line_index].y1),
          AlgGeom::Point(this->lines[line_index].x2, this->lines[line_index].y2)
        )
      );
      ld ratio_value = AlgGeom::CoreGeometryTools::get_point_on_line_ratio(
        AlgGeom::Point(
          this->lines[line_index].x1,
          this->lines[line_index].y1
        ),
        AlgGeom::Point(
          this->lines[line_index].x2,
          this->lines[line_index].y2
        ),
        new_loc
      );
      this->protocol.protocol["Point"][follower]["args"][1] = ratio_value;
    } else if (this->protocol.is_point_def_by_func(follower, "newPointOnCircle")) {
      const int circle_index = this->protocol.get_point_info(follower)["args"][0];
      this->protocol.protocol["Point"][follower]["args"][1] = atan2(
        mousePos.y - this->circles[circle_index].y_pos,
        mousePos.x - this->circles[circle_index].x_pos
      );
    } else if (this->protocol.is_point_def_by_func(follower, "newPointOnConic")) {
      const int conic_index = this->protocol.get_point_info(follower)["args"][0];
      const GConic &conic = this->conics[conic_index];
      AlgGeom::Point projected;
      if (AlgGeom::CoreGeometryTools::project_point_to_conic(
            AlgGeom::Point(mousePos.x, mousePos.y),
            AlgGeom::Conic(conic.A, conic.B, conic.C, conic.D, conic.E, conic.F), projected)) {
        this->protocol.protocol["Point"][follower]["location"] = {projected.x, projected.y};
      }
    } else {
      this->protocol.edit_position(follower, mousePos.x, mousePos.y);
    }
    // this->points[follower] = GPoint(mousePos.x, mousePos.y);
    // std::cout << "data = " << protocol.get_string_format() << std::endl;

    this->rebuild();
  }

  if (this->isDraggingText && this->text_follower >= 0 && this->text_follower < this->texts.size()) {
    const sf::Vector2i mouse_pixel = sf::Mouse::getPosition(window);
    const sf::Vector2f mouse = window.mapPixelToCoords(mouse_pixel, this->camera_view);
    this->protocol.edit_text_position(
      this->text_follower, mouse.x - this->text_drag_offset.x, mouse.y - this->text_drag_offset.y
    );
    this->rebuild();
  }
  
  if (this->current_tool == 2 && this->live_stack.size() >= 2) {
    this->lines.push_back(GLine(
      this->live_stack[0].x_pos,
      this->live_stack[0].y_pos,
      this->live_stack[1].x_pos,
      this->live_stack[1].y_pos,
      0
    ));

    protocol.new_line(this->lines.size() - 1, this->live_stack[0].index, this->live_stack[1].index, 0);

    // protocol += GeometryVisual::new_line(this->lines.size() - 1, this->live_stack[0].index, this->live_stack[1].index, true);

    this->live_stack.clear();
    this->live_stack_lines.clear();
    this->live_stack_circles.clear();
  }

  if (this->current_tool == 3 && this->live_stack.size() >= 2) {
    this->circles.push_back(GCircle(
      this->live_stack[0].x_pos,
      this->live_stack[0].y_pos,
      AlgGeom::CoreGeometryTools::dist_points(
        AlgGeom::Point(
          this->live_stack[0].x_pos,
          this->live_stack[0].y_pos
        ),
        AlgGeom::Point(
          this->live_stack[1].x_pos,
          this->live_stack[1].y_pos
        )
      )
    ));

    protocol.new_circle(this->circles.size() - 1, this->live_stack[0].index, this->live_stack[1].index);
    
    // protocol += GeometryVisual::new_circle(this->circles.size() - 1, this->live_stack[0].index, this->live_stack[1].index);

    this->live_stack.clear();
    this->live_stack_lines.clear();
    this->live_stack_circles.clear();
  }

  if (this->current_tool == 4 && this->live_stack.size() >= 2) {
    this->points.push_back(GPoint(
      (this->live_stack[0].x_pos + this->live_stack[1].x_pos) / 2,
      (this->live_stack[0].y_pos + this->live_stack[1].y_pos) / 2
    ));

    protocol.new_midpoint(this->points.size() - 1, this->live_stack[0].index, this->live_stack[1].index);

    // protocol += GeometryVisual::new_midpoint(this->points.size() - 1, this->live_stack[0].index, this->live_stack[1].index);

    this->live_stack.clear();
    this->live_stack_lines.clear();
    this->live_stack_circles.clear();
  }

  if (this->current_tool == 5 && this->live_stack.size() >= 3) {
    AlgGeom::Line lllll(
      AlgGeom::Point(this->live_stack[0].x_pos, this->live_stack[0].y_pos),
      AlgGeom::Point(this->live_stack[1].x_pos, this->live_stack[1].y_pos)
    );
    AlgGeom::Line l1 = AlgGeom::CoreGeometryTools::perp_bisector(
      AlgGeom::Point(this->live_stack[0].x_pos, this->live_stack[0].y_pos),
      AlgGeom::Point(this->live_stack[1].x_pos, this->live_stack[1].y_pos)
    );
    AlgGeom::Line l2 = AlgGeom::CoreGeometryTools::perp_bisector(
      AlgGeom::Point(this->live_stack[1].x_pos, this->live_stack[1].y_pos),
      AlgGeom::Point(this->live_stack[2].x_pos, this->live_stack[2].y_pos)
    );
    AlgGeom::Point p = AlgGeom::CoreGeometryTools::inter_lines(l1, l2);
    this->circles.push_back(GCircle(
      p.x, p.y, AlgGeom::CoreGeometryTools::dist_points(
        p, AlgGeom::Point(this->live_stack[0].x_pos, this->live_stack[0].y_pos)
      )
    ));

    protocol.new_circumcircle(this->circles.size() - 1, this->live_stack[0].index, this->live_stack[1].index, this->live_stack[2].index);

    // protocol += GeometryVisual::new_circumcircle(this->circles.size() - 1, this->live_stack[0].index, this->live_stack[1].index, this->live_stack[2].index);

    this->live_stack.clear();
    this->live_stack_lines.clear();
    this->live_stack_circles.clear();
  }

  if (this->current_tool == 6 && this->live_stack.size() >= 2) {
    this->lines.push_back(GLine(
      this->live_stack[0].x_pos,
      this->live_stack[0].y_pos,
      this->live_stack[1].x_pos,
      this->live_stack[1].y_pos,
      1 
    ));

    protocol.new_line(this->lines.size() - 1, this->live_stack[0].index, this->live_stack[1].index, 1);

    // protocol += GeometryVisual::new_line(this->lines.size() - 1, this->live_stack[0].index, this->live_stack[1].index, false);

    this->live_stack.clear();
    this->live_stack_lines.clear();
    this->live_stack_circles.clear();
  }

  if (this->current_tool == 7 && this->live_stack_lines.size() >= 2) {
    AlgGeom::Point p = AlgGeom::CoreGeometryTools::inter_lines(
      AlgGeom::Line(
        AlgGeom::Point(this->live_stack_lines[0].x1, this->live_stack_lines[0].y1),
        AlgGeom::Point(this->live_stack_lines[0].x2, this->live_stack_lines[0].y2)
      ),
      AlgGeom::Line(
        AlgGeom::Point(this->live_stack_lines[1].x1, this->live_stack_lines[1].y1),
        AlgGeom::Point(this->live_stack_lines[1].x2, this->live_stack_lines[1].y2)
      )
    );
    this->points.push_back(GPoint(p.x, p.y));

    protocol.new_inter_ll(this->points.size() - 1, this->live_stack_lines[0].index, this->live_stack_lines[1].index);

    // protocol += GeometryVisual::new_line_intersection(this->points.size() - 1, this->live_stack_lines[0].index, this->live_stack_lines[1].index);

    this->live_stack.clear();
    this->live_stack_lines.clear();
    this->live_stack_circles.clear();
  }

  if (this->current_tool == 8 && this->live_stack_lines.size() == 1 && this->live_stack.size() == 1) {
    AlgGeom::Line l = AlgGeom::CoreGeometryTools::perp_normal(
      AlgGeom::Point(this->live_stack[0].x_pos, this->live_stack[0].y_pos),
      AlgGeom::Line(
        AlgGeom::Point(this->live_stack_lines[0].x1, this->live_stack_lines[0].y1),
        AlgGeom::Point(this->live_stack_lines[0].x2, this->live_stack_lines[0].y2)
      )
    );
    pair<pair<ld, ld>, pair<ld, ld>> gen = l.gen_points();
    this->lines.push_back(GLine(
      gen.first.first, gen.first.second, gen.second.first, gen.second.second,
      1
    ));

    protocol.new_perp_normal(this->lines.size() - 1, this->live_stack[0].index, this->live_stack_lines[0].index);

    // protocol += GeometryVisual::new_perp_normal(this->lines.size() - 1, this->live_stack[0].index, this->live_stack_lines[0].index);

    this->live_stack.clear();
    this->live_stack_lines.clear();
    this->live_stack_circles.clear();
  }

  if (this->current_tool == 10 && this->live_stack.size() >= 3) {
    AlgGeom::Point p = AlgGeom::CoreGeometryTools::incenter(
      AlgGeom::Point(this->live_stack[0].x_pos, this->live_stack[0].y_pos),
      AlgGeom::Point(this->live_stack[1].x_pos, this->live_stack[1].y_pos),
      AlgGeom::Point(this->live_stack[2].x_pos, this->live_stack[2].y_pos)
    );
    this->points.push_back(
      GPoint(p.x, p.y)
    );

    protocol.new_incenter(this->points.size() - 1, this->live_stack[0].index, this->live_stack[1].index, this->live_stack[2].index);

    // protocol += GeometryVisual::new_incenter(this->points.size() - 1, this->live_stack[0].index, this->live_stack[1].index, this->live_stack[2].index);

    this->live_stack.clear();
    this->live_stack_lines.clear();
    this->live_stack_circles.clear();
  }

  if (this->current_tool == 11 && this->live_stack.size() >= 3) {
    AlgGeom::Point p = AlgGeom::CoreGeometryTools::excenter(
      AlgGeom::Point(this->live_stack[0].x_pos, this->live_stack[0].y_pos),
      AlgGeom::Point(this->live_stack[1].x_pos, this->live_stack[1].y_pos),
      AlgGeom::Point(this->live_stack[2].x_pos, this->live_stack[2].y_pos)
    );
    this->points.push_back(
      GPoint(p.x, p.y)
    );

    protocol.new_excenter(this->points.size() - 1, this->live_stack[0].index, this->live_stack[1].index, this->live_stack[2].index);

    // protocol += GeometryVisual::new_excenter(this->points.size() - 1, this->live_stack[0].index, this->live_stack[1].index, this->live_stack[2].index);

    this->live_stack.clear();
    this->live_stack_lines.clear();
    this->live_stack_circles.clear();
  }

  if (this->current_tool == 12 && this->live_stack_lines.size() >= 1 && this->live_stack_circles.size() >= 1) {
    pair<AlgGeom::Point, AlgGeom::Point> p = AlgGeom::CoreGeometryTools::inter_lc(
      AlgGeom::Line(
        AlgGeom::Point(this->live_stack_lines[0].x1, this->live_stack_lines[0].y1),
        AlgGeom::Point(this->live_stack_lines[0].x2, this->live_stack_lines[0].y2)
      ),
      AlgGeom::Circle(
        AlgGeom::Point(
          this->live_stack_circles[0].x_pos, this->live_stack_circles[0].y_pos
        ),
        this->live_stack_circles[0].radius
      )
    );
    this->points.push_back(
      GPoint(p.first.x, p.first.y)
    );
    this->points.push_back(
      GPoint(p.second.x, p.second.y)
    );

    protocol.new_inter_lc(this->points.size() - 1, this->points.size() - 2, this->live_stack_lines[0].index, this->live_stack_circles[1].index);

    // protocol += GeometryVisual::new_inter_lc(this->points.size() - 1, this->points.size() - 2, this->live_stack_lines[0].index, this->live_stack_circles[1].index);

    this->live_stack.clear();
    this->live_stack_lines.clear();
    this->live_stack_circles.clear();
  }

  if (this->current_tool == 14 && this->live_stack_lines.size() == 1 && this->live_stack.size() == 1) {
    cout << "CREATING NEW" << endl;
    AlgGeom::Line l = AlgGeom::CoreGeometryTools::parallel_line(
      AlgGeom::Point(this->live_stack[0].x_pos, this->live_stack[0].y_pos),
      AlgGeom::Line(
        AlgGeom::Point(this->live_stack_lines[0].x1, this->live_stack_lines[0].y1),
        AlgGeom::Point(this->live_stack_lines[0].x2, this->live_stack_lines[0].y2)
      )
    );
    pair<pair<ld, ld>, pair<ld, ld>> gen = l.gen_points();
    this->lines.push_back(GLine(
      gen.first.first, gen.first.second, gen.second.first, gen.second.second,
      1
    ));

    protocol.new_parallel(this->lines.size() - 1, this->live_stack[0].index, this->live_stack_lines[0].index);

    // protocol += GeometryVisual::new_perp_normal(this->lines.size() - 1, this->live_stack[0].index, this->live_stack_lines[0].index);

    this->live_stack.clear();
    this->live_stack_lines.clear();
    this->live_stack_circles.clear();
  }

  if (this->current_tool == 15 && this->live_stack.size() == 5) {
    cout << "new Conic!" << endl;
    AlgGeom::Conic c = AlgGeom::CoreGeometryTools::fitConicThrough5(
      AlgGeom::Point(this->live_stack[0].x_pos, this->live_stack[0].y_pos),
      AlgGeom::Point(this->live_stack[1].x_pos, this->live_stack[1].y_pos),
      AlgGeom::Point(this->live_stack[2].x_pos, this->live_stack[2].y_pos),
      AlgGeom::Point(this->live_stack[3].x_pos, this->live_stack[3].y_pos),
      AlgGeom::Point(this->live_stack[4].x_pos, this->live_stack[4].y_pos)
    );
    cout << "points " << this->live_stack[0].x_pos << " " << this->live_stack[0].y_pos << endl;
    cout << "points " << this->live_stack[1].x_pos << " " << this->live_stack[1].y_pos << endl;
cout << "points " << this->live_stack[2].x_pos << " " << this->live_stack[2].y_pos << endl;
cout << "points " << this->live_stack[3].x_pos << " " << this->live_stack[3].y_pos << endl;
cout << "points " << this->live_stack[4].x_pos << " " << this->live_stack[4].y_pos << endl;
    cout << "Coefficients: " << c.a << " " << c.b << " " << c.c << " " << c.d << " " << c.e << " " << c.f << endl;

    this->conics.push_back(GConic(
        c.a, c.b, c.c, c.d, c.e, c.f
    ));
    protocol.new_conic(this->conics.size() - 1,
        this->live_stack[0].index,
        this->live_stack[1].index,
        this->live_stack[2].index,
        this->live_stack[3].index,
        this->live_stack[4].index
    );

    this->live_stack.clear();
  }

  if (this->current_tool == 16 && this->live_stack.size() == 9) {
    cout << "new Cubic!" << endl;
    AlgGeom::Cubic c = AlgGeom::CoreGeometryTools::fitCubicThrough9(
      AlgGeom::Point(this->live_stack[0].x_pos, this->live_stack[0].y_pos),
      AlgGeom::Point(this->live_stack[1].x_pos, this->live_stack[1].y_pos),
      AlgGeom::Point(this->live_stack[2].x_pos, this->live_stack[2].y_pos),
      AlgGeom::Point(this->live_stack[3].x_pos, this->live_stack[3].y_pos),
      AlgGeom::Point(this->live_stack[4].x_pos, this->live_stack[4].y_pos),
      AlgGeom::Point(this->live_stack[5].x_pos, this->live_stack[5].y_pos),
      AlgGeom::Point(this->live_stack[6].x_pos, this->live_stack[6].y_pos),
      AlgGeom::Point(this->live_stack[7].x_pos, this->live_stack[7].y_pos),
      AlgGeom::Point(this->live_stack[8].x_pos, this->live_stack[8].y_pos)
    );
    this->cubics.push_back(GCubic(
        c.a, c.b, c.c, c.d, c.e, c.f, c.g, c.h, c.i, c.j
    ));
    protocol.new_cubic(this->cubics.size() - 1,
        this->live_stack[0].index,
        this->live_stack[1].index,
        this->live_stack[2].index,
        this->live_stack[3].index,
        this->live_stack[4].index,
        this->live_stack[5].index,
        this->live_stack[6].index,
        this->live_stack[7].index,
        this->live_stack[8].index
    );
    this->live_stack.clear();
  }

  if (this->current_tool == 17 && this->live_stack.size() == 3) {
    AlgGeom::Line l = AlgGeom::CoreGeometryTools::angle_bisector(
      AlgGeom::Point(this->live_stack[0].x_pos, this->live_stack[0].y_pos),
      AlgGeom::Point(this->live_stack[1].x_pos, this->live_stack[1].y_pos),
      AlgGeom::Point(this->live_stack[2].x_pos, this->live_stack[2].y_pos)
    );
    pair<pair<ld, ld>, pair<ld, ld>> gen = l.gen_points();
    this->lines.push_back(GLine(
      gen.first.first, gen.first.second, gen.second.first, gen.second.second,
      1
    ));

    protocol.new_angle_bisector(
      this->lines.size() - 1,
      this->live_stack[0].index,
      this->live_stack[1].index,
      this->live_stack[2].index
    );

    this->live_stack.clear();
  }

  if (this->current_tool == 18 && this->live_stack_lines.size() == 2) {
    AlgGeom::Line l1(
        AlgGeom::Point(this->live_stack_lines[0].x1, this->live_stack_lines[0].y1),
        AlgGeom::Point(this->live_stack_lines[0].x2, this->live_stack_lines[0].y2)
    );
    AlgGeom::Line l2(
        AlgGeom::Point(this->live_stack_lines[1].x1, this->live_stack_lines[1].y1),
        AlgGeom::Point(this->live_stack_lines[1].x2, this->live_stack_lines[1].y2)
    );
    AlgGeom::Line l = AlgGeom::CoreGeometryTools::reflect_line_over_line(l1, l2);
    pair<pair<ld, ld>, pair<ld, ld>> gen = l.gen_points();
    this->lines.push_back(GLine(
      gen.first.first, gen.first.second, gen.second.first, gen.second.second,
      1
    ));

    protocol.new_reflect_line_over_line(
        this->lines.size() - 1,
        this->live_stack_lines[0].index,
        this->live_stack_lines[1].index
    );

    this->live_stack_lines.clear();
  }

  if (this->current_tool == 19 && this->live_stack.size() == 4) {
    AlgGeom::Point p = AlgGeom::CoreGeometryTools::isogonal_conjugate(
      AlgGeom::Point(this->live_stack[0].x_pos, this->live_stack[0].y_pos),
      AlgGeom::Point(this->live_stack[1].x_pos, this->live_stack[1].y_pos),
      AlgGeom::Point(this->live_stack[2].x_pos, this->live_stack[2].y_pos),
      AlgGeom::Point(this->live_stack[3].x_pos, this->live_stack[3].y_pos)
    );
    this->points.push_back(
      GPoint(p.x, p.y)
    );

    protocol.new_isogonal_conjugate(this->points.size() - 1, this->live_stack[0].index, this->live_stack[1].index, this->live_stack[2].index, this->live_stack[3].index);

    this->live_stack.clear();
  }

  if (this->current_tool == 20 && this->live_stack.size() == 1 && this->live_stack_lines.size() == 1) {
    cout << "Reflecting!" << endl;
    AlgGeom::Point p = AlgGeom::CoreGeometryTools::reflect_point_over_line(
      AlgGeom::Point(this->live_stack[0].x_pos, this->live_stack[0].y_pos),
      AlgGeom::Line(
        AlgGeom::Point(this->live_stack_lines[0].x1, this->live_stack_lines[0].y1),
        AlgGeom::Point(this->live_stack_lines[0].x2, this->live_stack_lines[0].y2)
      )
    );
    this->points.push_back(
      GPoint(p.x, p.y)
    );
    protocol.new_reflect_point_over_line(
      this->points.size() - 1,
      this->live_stack[0].index,
      this->live_stack_lines[0].index
    );

    this->live_stack.clear();
    this->live_stack_lines.clear();
  }

  if (this->current_tool == 21 && this->live_stack.size() == 2) {
    const AlgGeom::Point reflected = AlgGeom::CoreGeometryTools::reflect_point_over_point(
      convert_gpoint(this->live_stack[0]), convert_gpoint(this->live_stack[1])
    );
    this->points.push_back(GPoint(reflected.x, reflected.y));
    this->protocol.new_reflect_point_over_point(
      this->points.size() - 1, this->live_stack[0].index, this->live_stack[1].index
    );
    this->live_stack.clear();
  }

  if (this->current_tool == 22 && this->live_stack.size() == 1 &&
      this->live_stack_lines.size() == 1) {
    const AlgGeom::Point projected = AlgGeom::CoreGeometryTools::project_point_to_line(
      convert_gpoint(this->live_stack[0]),
      AlgGeom::Line(
        AlgGeom::Point(this->live_stack_lines[0].x1, this->live_stack_lines[0].y1),
        AlgGeom::Point(this->live_stack_lines[0].x2, this->live_stack_lines[0].y2)
      )
    );
    this->points.push_back(GPoint(projected.x, projected.y));
    this->protocol.new_project_point_onto_line(
      this->points.size() - 1, this->live_stack[0].index, this->live_stack_lines[0].index
    );
    this->live_stack.clear();
    this->live_stack_lines.clear();
  }

  if (this->current_tool == 23 && this->live_stack.size() == 3) {
    AlgGeom::Point center;
    if (AlgGeom::CoreGeometryTools::circumcenter(
          convert_gpoint(this->live_stack[0]), convert_gpoint(this->live_stack[1]),
          convert_gpoint(this->live_stack[2]), center)) {
      this->points.push_back(GPoint(center.x, center.y));
      this->protocol.new_circumcenter(
        this->points.size() - 1, this->live_stack[0].index,
        this->live_stack[1].index, this->live_stack[2].index
      );
    }
    this->live_stack.clear();
  }

  if (this->current_tool == 24 && this->live_stack.size() == 3) {
    this->triangle_center_request = {
      this->live_stack[0].index, this->live_stack[1].index, this->live_stack[2].index
    };
    this->live_stack.clear();
  }

  if (this->current_tool == 25 && this->live_stack.size() == 3) {
    AlgGeom::Conic conic;
    if (AlgGeom::CoreGeometryTools::rectangular_hyperbola(
          convert_gpoint(this->live_stack[0]), convert_gpoint(this->live_stack[1]),
          convert_gpoint(this->live_stack[2]), conic)) {
      this->conics.push_back(GConic(conic.a, conic.b, conic.c, conic.d, conic.e, conic.f));
      this->protocol.new_rectangular_hyperbola(
        this->conics.size() - 1, this->live_stack[0].index,
        this->live_stack[1].index, this->live_stack[2].index
      );
    }
    this->live_stack.clear();
  }

  if (event.type == sf::Event::KeyPressed &&
      (event.key.code == sf::Keyboard::Delete || event.key.code == sf::Keyboard::BackSpace) &&
      this->current_tool == 0) {
    if (this->selected_text != -1) {
      this->delete_object("Text", this->selected_text);
    } else if (this->selected_point != -1) {
      this->delete_point(this->selected_point);
    } else if (this->selected_line != -1) {
      this->delete_line(this->selected_line);
    } else if (this->selected_circle != -1) {
      this->delete_circle(this->selected_circle);
    }
  }
}

void GeometryVisual::draw(sf::RenderWindow& window, const sf::Font *font) {
  this->initialize_camera(window);
  const sf::View previous_view = window.getView();
  window.setView(this->camera_view);
  for (int i = 0; i < this->points.size(); ++i) {
    if (!this->points[i].visible) continue;
    const float radius = 4 + this->points[i].thickness;
    this->points[i].shape.setRadius(radius);
    this->points[i].shape.setPosition(this->points[i].x_pos - radius, this->points[i].y_pos - radius);
    this->points[i].shape.setFillColor(this->points[i].color);
    this->points[i].shape.setOutlineColor(sf::Color::Black);
    this->points[i].shape.setOutlineThickness(i == this->selected_point ? 3 : 0);
    this->points[i].draw(window);
  }
  for (int i = 0; i < this->lines.size(); ++i) {
    GLine &line = this->lines[i];
    if (!line.visible) continue;
    const sf::Color line_color = i == this->selected_line ? sf::Color::Black : line.color;
    if (abs(line.thickness - 1.f) < 1e-5f && line.dashed == (line.line_type == 2)) {
      for (size_t vertex = 0; vertex < line.line.getVertexCount(); ++vertex) {
        line.line[vertex].color = line_color;
      }
      line.draw(window);
      continue;
    }
    sf::Vector2f start(line.x1, line.y1);
    sf::Vector2f end(line.x2, line.y2);
    if (line.line_type != 0) {
      sf::Vector2f direction = end - start;
      const float length = sqrt(direction.x * direction.x + direction.y * direction.y);
      if (length < 1e-5f) continue;
      direction /= length;
      const sf::Vector2f midpoint = (start + end) / 2.f;
      start = midpoint - direction * 5000.f;
      end = midpoint + direction * 5000.f;
    }
    sf::VertexArray stroke(sf::Quads);
    append_styled_line(
      stroke, start, end, line.thickness * this->camera_zoom,
      line_color,
      line.dashed, this->camera_zoom
    );
    window.draw(stroke);
  }
  for (int i = 0; i < this->circles.size(); ++i) {
    GCircle &circle = this->circles[i];
    if (!circle.visible) continue;
    const sf::Color color = i == this->selected_circle ? sf::Color::Black : circle.color;
    const float thickness = std::max(circle.thickness, i == this->selected_circle ? 3.f : 1.f);
    if (!circle.dashed) {
      circle.shape.setRadius(circle.radius);
      circle.shape.setPosition(circle.x_pos - circle.radius, circle.y_pos - circle.radius);
      circle.shape.setFillColor(sf::Color::Transparent);
      circle.shape.setOutlineColor(color);
      circle.shape.setOutlineThickness(thickness);
      circle.draw(window);
    } else {
      sf::VertexArray stroke(sf::Quads);
      const int segments = 160;
      for (int segment = 0; segment < segments; ++segment) {
        if (segment % 10 >= 6) continue;
        const float a1 = 2 * 3.14159265358979323846f * segment / segments;
        const float a2 = 2 * 3.14159265358979323846f * (segment + 1) / segments;
        append_stroke_segment(
          stroke,
          sf::Vector2f(circle.x_pos + circle.radius * cos(a1), circle.y_pos + circle.radius * sin(a1)),
          sf::Vector2f(circle.x_pos + circle.radius * cos(a2), circle.y_pos + circle.radius * sin(a2)),
          thickness * this->camera_zoom, color
        );
      }
      window.draw(stroke);
    }
  }
  for (GConic &conic : this->conics) {
    if (!conic.visible) continue;
    if (abs(conic.thickness - 1.f) < 1e-5f && !conic.dashed) {
      for (size_t vertex = 0; vertex < conic.branch1.getVertexCount(); ++vertex) {
        conic.branch1[vertex].color = conic.color;
      }
      for (size_t vertex = 0; vertex < conic.branch2.getVertexCount(); ++vertex) {
        conic.branch2[vertex].color = conic.color;
      }
      conic.draw(window);
      continue;
    }
    sf::VertexArray stroke(sf::Quads);
    auto append_branch = [&](const sf::VertexArray &branch) {
      for (size_t i = 0; i + 1 < branch.getVertexCount(); i += 2) {
        if (conic.dashed && (i / 2 / 12) % 2 == 1) continue;
        append_stroke_segment(
          stroke, branch[i].position, branch[i + 1].position,
          conic.thickness * this->camera_zoom, conic.color
        );
      }
    };
    append_branch(conic.branch1);
    append_branch(conic.branch2);
    window.draw(stroke);
  }
  for (auto& cubic : this->cubics) {
    cubic.draw(window);
  }
  if (font != nullptr) {
    for (GTextAnnotation &annotation : this->texts) {
      if (!annotation.visible) continue;
      std::vector<sf::Text> rendered;
      float cursor = annotation.x;
      float left = annotation.x;
      float top = annotation.y - 7;
      float right = annotation.x;
      float bottom = annotation.y + 24;
      for (const AnnotationRun &run : annotation_runs(annotation.content)) {
        sf::Text text;
        text.setFont(*font);
        text.setString(run.value);
        text.setCharacterSize(run.size);
        text.setStyle(run.italic ? sf::Text::Italic : sf::Text::Regular);
        text.setFillColor(sf::Color::Black);
        text.setPosition(cursor, annotation.y + run.offset);
        const sf::FloatRect bounds = text.getGlobalBounds();
        left = std::min(left, bounds.left);
        top = std::min(top, bounds.top);
        right = std::max(right, bounds.left + bounds.width);
        bottom = std::max(bottom, bounds.top + bounds.height);
        cursor = text.findCharacterPos(text.getString().getSize()).x;
        rendered.push_back(text);
      }
      annotation.bounds_left = left;
      annotation.bounds_top = top;
      annotation.bounds_width = std::max(20.f, right - left);
      annotation.bounds_height = std::max(28.f, bottom - top);
      sf::RectangleShape box(sf::Vector2f(annotation.bounds_width + 10, annotation.bounds_height + 10));
      box.setPosition(annotation.bounds_left - 5, annotation.bounds_top - 5);
      box.setFillColor(sf::Color(255, 255, 255, 220));
      box.setOutlineColor(this->selected_text == annotation.index ? sf::Color::Black : sf::Color(120, 120, 120));
      box.setOutlineThickness(this->selected_text == annotation.index ? 2 : 1);
      window.draw(box);
      for (const sf::Text &text : rendered) window.draw(text);
    }
    for (int i = 0; i < this->points.size(); ++i) {
      const GPoint &point = this->points[i];
      std::string point_label = point.label;
      if (point_label.empty() && this->protocol.protocol["Point"].is_array() &&
          i < this->protocol.protocol["Point"].size()) {
        point_label = this->protocol.protocol["Point"][i].value("label", "");
      }
      if (!point.visible || !point.label_visible || point_label.empty()) continue;
      sf::Text label;
      label.setFont(*font);
      label.setString(point_label);
      label.setCharacterSize(14);
      label.setFillColor(sf::Color::Black);
      label.setPosition(point.x_pos + 7, point.y_pos - 19);
      window.draw(label);
    }
  }
  window.setView(previous_view);
  if (this->protocol.has_searcher_objects()) {
    const float x = window.getSize().x - 44.f;
    sf::RectangleShape close_button(sf::Vector2f(32, 32));
    close_button.setPosition(x, 48);
    close_button.setFillColor(sf::Color::White);
    close_button.setOutlineColor(sf::Color::Black);
    close_button.setOutlineThickness(1);
    window.draw(close_button);

    sf::VertexArray cross(sf::Lines, 4);
    cross[0] = sf::Vertex(sf::Vector2f(x + 8, 56), sf::Color::Black);
    cross[1] = sf::Vertex(sf::Vector2f(x + 24, 72), sf::Color::Black);
    cross[2] = sf::Vertex(sf::Vector2f(x + 24, 56), sf::Color::Black);
    cross[3] = sf::Vertex(sf::Vector2f(x + 8, 72), sf::Color::Black);
    window.draw(cross);
  }
}










