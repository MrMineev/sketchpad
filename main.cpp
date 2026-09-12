#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <memory>
#include <set>

#include <SFML/Graphics.hpp>

#include "src/view/toolbar.h"
#include "gui_tools/src/Gui/Gui.hpp"
#include "src/core/geometry.h"
#include "src/core/alg.h"
#include "json_manager/json/single_include/nlohmann/json.hpp"

using namespace std;

typedef long long ll;
typedef long double ld;
typedef vector<ll> vll;
typedef vector<vll> vvll;
typedef vector<vvll> vvvll;

const ll SCREEN_X = 1200;
const ll SCREEN_Y = 800;
const ll MENU_BAR_X = 250;
const float TAB_HEIGHT = 36;
const float TAB_WIDTH = 160;
const std::vector<sf::Color> STYLE_COLORS = {
  sf::Color::Black, sf::Color::White, sf::Color::Red, sf::Color(255, 140, 0),
  sf::Color::Yellow, sf::Color::Green, sf::Color::Cyan, sf::Color::Blue,
  sf::Color(128, 0, 180), sf::Color(255, 105, 180), sf::Color(110, 110, 110),
  sf::Color(140, 85, 35)
};

sf::Vector2f style_panel_position(const sf::RenderWindow &window, const ObjectStyleRequest &request) {
  return sf::Vector2f(
    std::clamp(
      static_cast<float>(request.screen_position.x), 260.f,
      std::max(260.f, window.getSize().x - 300.f)
    ),
    std::clamp(
      static_cast<float>(request.screen_position.y), 40.f,
      std::max(40.f, window.getSize().y - 350.f)
    )
  );
}

struct SketchTab {
  std::unique_ptr<GeometryVisual> geometry;
  std::string title;
  int id = -1;
  int source_id = -1;
  json inversion_circle;
  std::string source_signature;

  bool is_linked() const {
    return source_id != -1;
  }
};

int find_tab_by_id(const std::vector<SketchTab> &tabs, int id) {
  for (int i = 0; i < tabs.size(); ++i) {
    if (tabs[i].id == id) return i;
  }
  return -1;
}

int find_inversion_circle(const Protocol &protocol, const json &definition) {
  if (!protocol.protocol.contains("Circle") || !protocol.protocol["Circle"].is_array()) return -1;
  const json &circles = protocol.protocol["Circle"];
  for (size_t i = 0; i < circles.size(); ++i) {
    if (circles[i] == definition) return static_cast<int>(i);
  }
  return -1;
}

void refresh_linked_tab(std::vector<SketchTab> &tabs, int index) {
  SketchTab &tab = tabs[index];
  if (!tab.is_linked()) return;
  const int source_index = find_tab_by_id(tabs, tab.source_id);
  if (source_index == -1) return;
  GeometryVisual &source = *tabs[source_index].geometry;
  const std::string signature = source.protocol.get_string_format();
  if (signature == tab.source_signature) return;
  tab.geometry->build_inversion(source, find_inversion_circle(source.protocol, tab.inversion_circle));
  tab.source_signature = signature;
}

const Protocol &visible_protocol(const std::vector<SketchTab> &tabs, int active_tab) {
  const SketchTab &tab = tabs[active_tab];
  if (!tab.is_linked()) return tab.geometry->protocol;
  const int source_index = find_tab_by_id(tabs, tab.source_id);
  return source_index == -1 ? tab.geometry->protocol : tabs[source_index].geometry->protocol;
}

void close_tab_tree(std::vector<SketchTab> &tabs, int id) {
  std::set<int> closing = {id};
  bool changed = true;
  while (changed) {
    changed = false;
    for (const SketchTab &tab : tabs) {
      if (tab.is_linked() && closing.count(tab.source_id) && closing.insert(tab.id).second) {
        changed = true;
      }
    }
  }
  tabs.erase(std::remove_if(tabs.begin(), tabs.end(), [&](const SketchTab &tab) {
    return closing.count(tab.id) != 0;
  }), tabs.end());
}

std::vector<std::string> protocol_preview_lines(const Protocol &protocol) {
  const std::string formatted = protocol.protocol.dump(2);
  std::vector<std::string> lines;
  size_t start = 0;
  while (start <= formatted.size()) {
    const size_t newline = formatted.find('\n', start);
    std::string line = formatted.substr(start, newline == std::string::npos ? newline : newline - start);
    while (line.size() > 64) {
      lines.push_back(line.substr(0, 64));
      line = "  " + line.substr(64);
    }
    lines.push_back(line);
    if (newline == std::string::npos) break;
    start = newline + 1;
  }
  return lines;
}

signed main() {
  std::srand(static_cast<unsigned int>(std::time(nullptr)));

  sf::RenderWindow window(sf::VideoMode(SCREEN_X, SCREEN_Y), "Sketchpad");
  // Declare menu
  gui::Menu menu(window);
  menu.setPosition(10, 10);
  gui::Theme::loadFont("gui_assets/demo/tahoma.ttf");
  gui::Theme::loadTexture("gui_assets/demo/texture-default.png");

  sf::Font font;
  if (!font.loadFromFile("gui_assets/demo/tahoma.ttf")) {
    std::cerr << "Error loading font!" << std::endl;
    return -1;
  }

  gui::TextBox *command_prompt = new gui::TextBox();
  menu.add(command_prompt);

  sf::Color backgroundColor = sf::Color(252, 242, 172);
  std::vector<SketchTab> tabs;
  SketchTab original;
  original.geometry = std::make_unique<GeometryVisual>(MENU_BAR_X);
  original.title = "Diagram 1";
  original.id = 0;
  tabs.push_back(std::move(original));
  int active_tab = 0;
  int next_tab_id = 1;
  int dragged_tab = -1;
  int rename_title_tab_id = -1;
  std::string rename_title_text;
  int text_annotation_tab = -1;
  sf::Vector2f text_annotation_position;
  int style_tab = -1;
  ObjectStyleRequest style_target;
  std::string text_annotation_value;
  bool protocol_preview = false;
  int protocol_scroll = 0;
  int rename_tab = -1;
  int rename_point = -1;
  std::string rename_text;
  int triangle_center_tab = -1;
  std::vector<int> triangle_center_vertices;
  std::string triangle_center_text;
  bool triangle_center_error = false;
  ToolView toolbar(&menu, tabs[0].geometry.get(), SCREEN_X, SCREEN_Y, MENU_BAR_X);

  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      bool tab_event = false;
      if (event.type == sf::Event::Resized) {
        const unsigned int width = std::max(1u, event.size.width);
        const unsigned int height = std::max(1u, event.size.height);
        window.setView(sf::View(sf::FloatRect(0, 0, width, height)));
        toolbar.resize(height);
        for (SketchTab &tab : tabs) tab.geometry->resize_camera(width, height);
        tab_event = true;
      }
      if (style_tab != -1) {
        tab_event = true;
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
          style_tab = -1;
        } else if (event.type == sf::Event::MouseButtonPressed &&
                   event.mouseButton.button == sf::Mouse::Left) {
          const sf::Vector2f panel = style_panel_position(window, style_target);
          const sf::Vector2f mouse(event.mouseButton.x, event.mouseButton.y);
          if (mouse.x < panel.x || mouse.x > panel.x + 280 ||
              mouse.y < panel.y || mouse.y > panel.y + 330 ||
              (mouse.x >= panel.x + 250 && mouse.y <= panel.y + 32)) {
            style_tab = -1;
          } else {
            GeometryVisual &geometry = *tabs[style_tab].geometry;
            json &object = geometry.protocol.protocol[style_target.type][style_target.index];
            auto apply = [&](sf::Color color, float thickness, bool dashed) {
              geometry.protocol.set_style(
                style_target.type, style_target.index, color.r, color.g, color.b, thickness, dashed
              );
              geometry.rebuild();
            };
            if (mouse.x >= panel.x + 15 && mouse.x <= panel.x + 115 &&
                mouse.y >= panel.y + 42 && mouse.y <= panel.y + 70) {
              geometry.protocol.reset_style(style_target.type, style_target.index);
              geometry.rebuild();
            } else if (mouse.y >= panel.y + 82 && mouse.y <= panel.y + 157) {
              const int column = static_cast<int>((mouse.x - panel.x - 12) / 43);
              const int row = static_cast<int>((mouse.y - panel.y - 82) / 38);
              const int color_index = row * 6 + column;
              if (column >= 0 && column < 6 && row >= 0 && row < 2 &&
                  color_index < STYLE_COLORS.size()) {
                apply(
                  STYLE_COLORS[color_index], object.value("thickness", 1.f),
                  object.value("dashed", false)
                );
              }
            } else if (mouse.y >= panel.y + 202 && mouse.y <= panel.y + 234) {
              const json color = object.value("color", json({0, 0, 0}));
              const sf::Color current(
                color[0].get<int>(), color[1].get<int>(), color[2].get<int>()
              );
              float thickness = object.value("thickness", 1.f);
              if (mouse.x >= panel.x + 15 && mouse.x <= panel.x + 60) thickness -= 1;
              if (mouse.x >= panel.x + 205 && mouse.x <= panel.x + 250) thickness += 1;
              apply(current, thickness, object.value("dashed", false));
            } else if (style_target.type != "Point" && mouse.y >= panel.y + 265 &&
                       mouse.y <= panel.y + 299) {
              const json color = object.value("color", json({0, 0, 0}));
              const sf::Color current(
                color[0].get<int>(), color[1].get<int>(), color[2].get<int>()
              );
              if (mouse.x >= panel.x + 15 && mouse.x <= panel.x + 125) {
                apply(current, object.value("thickness", 1.f), false);
              } else if (mouse.x >= panel.x + 145 && mouse.x <= panel.x + 255) {
                apply(current, object.value("thickness", 1.f), true);
              }
            }
          }
        }
      } else if (rename_title_tab_id != -1) {
        tab_event = true;
        if (event.type == sf::Event::TextEntered && event.text.unicode >= 32 &&
            event.text.unicode < 127 && rename_title_text.size() < 40) {
          rename_title_text.push_back(static_cast<char>(event.text.unicode));
        }
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::BackSpace &&
            !rename_title_text.empty()) {
          rename_title_text.pop_back();
        }
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter &&
            !rename_title_text.empty()) {
          const int index = find_tab_by_id(tabs, rename_title_tab_id);
          if (index != -1) tabs[index].title = rename_title_text;
          rename_title_tab_id = -1;
          rename_title_text.clear();
        } else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
          rename_title_tab_id = -1;
          rename_title_text.clear();
        }
      } else if (text_annotation_tab != -1) {
        tab_event = true;
        if (event.type == sf::Event::TextEntered && event.text.unicode >= 32 &&
            event.text.unicode < 127 && text_annotation_value.size() < 200) {
          text_annotation_value.push_back(static_cast<char>(event.text.unicode));
        }
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::BackSpace &&
            !text_annotation_value.empty()) {
          text_annotation_value.pop_back();
        }
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter &&
            !text_annotation_value.empty()) {
          GeometryVisual &geometry = *tabs[text_annotation_tab].geometry;
          geometry.protocol.new_text(
            geometry.texts.size(), text_annotation_position.x,
            text_annotation_position.y, text_annotation_value
          );
          geometry.rebuild();
          text_annotation_tab = -1;
          text_annotation_value.clear();
        } else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
          text_annotation_tab = -1;
          text_annotation_value.clear();
        }
      } else if (!triangle_center_vertices.empty()) {
        tab_event = true;
        if (event.type == sf::Event::TextEntered && event.text.unicode >= '0' && event.text.unicode <= '9' &&
            triangle_center_text.size() < 3) {
          triangle_center_text.push_back(static_cast<char>(event.text.unicode));
          triangle_center_error = false;
        }
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::BackSpace &&
            !triangle_center_text.empty()) {
          triangle_center_text.pop_back();
          triangle_center_error = false;
        }
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter) {
          const int number = triangle_center_text.empty() ? 0 : std::stoi(triangle_center_text);
          GeometryVisual &geometry = *tabs[triangle_center_tab].geometry;
          AlgGeom::Point center;
          const bool valid = number >= 1 && number <= 100 &&
            AlgGeom::CoreGeometryTools::triangle_center(
              number,
              AlgGeom::Point(geometry.points[triangle_center_vertices[0]].x_pos,
                             geometry.points[triangle_center_vertices[0]].y_pos),
              AlgGeom::Point(geometry.points[triangle_center_vertices[1]].x_pos,
                             geometry.points[triangle_center_vertices[1]].y_pos),
              AlgGeom::Point(geometry.points[triangle_center_vertices[2]].x_pos,
                             geometry.points[triangle_center_vertices[2]].y_pos), center
            );
          if (valid) {
            geometry.protocol.new_triangle_center(
              geometry.points.size(), triangle_center_vertices[0], triangle_center_vertices[1],
              triangle_center_vertices[2], number
            );
            geometry.rebuild();
            triangle_center_tab = -1;
            triangle_center_vertices.clear();
            triangle_center_text.clear();
            triangle_center_error = false;
          } else {
            triangle_center_error = true;
          }
        } else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
          triangle_center_tab = -1;
          triangle_center_vertices.clear();
          triangle_center_text.clear();
          triangle_center_error = false;
        }
      } else if (rename_point != -1) {
        tab_event = true;
        if (event.type == sf::Event::TextEntered && event.text.unicode >= 32 && event.text.unicode < 127) {
          rename_text.push_back(static_cast<char>(event.text.unicode));
        }
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::BackSpace &&
            !rename_text.empty()) {
          rename_text.pop_back();
        }
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter) {
          tabs[rename_tab].geometry->protocol.set_point_label(rename_point, rename_text);
          tabs[rename_tab].geometry->rebuild();
          rename_tab = -1;
          rename_point = -1;
          rename_text.clear();
        } else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
          rename_tab = -1;
          rename_point = -1;
          rename_text.clear();
        }
      }
      const float protocol_button_x = window.getSize().x - 44.f;
      const float new_tab_button_x = window.getSize().x - 84.f;
      const float duplicate_tab_button_x = window.getSize().x - 124.f;
      if (!tab_event && event.type == sf::Event::MouseButtonPressed &&
          event.mouseButton.button == sf::Mouse::Left && event.mouseButton.y >= 2 &&
          event.mouseButton.y <= 34 && event.mouseButton.x >= new_tab_button_x &&
          event.mouseButton.x <= new_tab_button_x + 32) {
        SketchTab tab;
        tab.geometry = std::make_unique<GeometryVisual>(MENU_BAR_X);
        tab.title = "Diagram " + std::to_string(next_tab_id + 1);
        tab.id = next_tab_id++;
        tabs.push_back(std::move(tab));
        active_tab = tabs.size() - 1;
        protocol_scroll = 0;
        toolbar.geomv = tabs[active_tab].geometry.get();
        tab_event = true;
      } else if (!tab_event && event.type == sf::Event::MouseButtonPressed &&
                 event.mouseButton.button == sf::Mouse::Left && event.mouseButton.y >= 2 &&
                 event.mouseButton.y <= 34 && event.mouseButton.x >= duplicate_tab_button_x &&
                 event.mouseButton.x <= duplicate_tab_button_x + 32) {
        SketchTab duplicate;
        duplicate.geometry = std::make_unique<GeometryVisual>(MENU_BAR_X);
        duplicate.title = tabs[active_tab].title + " Copy";
        duplicate.id = next_tab_id++;
        if (tabs[active_tab].is_linked()) {
          duplicate.source_id = tabs[active_tab].source_id;
          duplicate.inversion_circle = tabs[active_tab].inversion_circle;
          const int source_index = find_tab_by_id(tabs, duplicate.source_id);
          if (source_index != -1) {
            GeometryVisual &source = *tabs[source_index].geometry;
            duplicate.source_signature = source.protocol.get_string_format();
            duplicate.geometry->build_inversion(
              source, find_inversion_circle(source.protocol, duplicate.inversion_circle)
            );
          }
        } else {
          duplicate.geometry->protocol = tabs[active_tab].geometry->protocol;
          duplicate.geometry->rebuild();
        }
        tabs.push_back(std::move(duplicate));
        active_tab = tabs.size() - 1;
        protocol_scroll = 0;
        toolbar.geomv = tabs[active_tab].geometry.get();
        tab_event = true;
      } else if (!tab_event && event.type == sf::Event::MouseButtonPressed &&
                 event.mouseButton.button == sf::Mouse::Left &&
                 event.mouseButton.x >= protocol_button_x && event.mouseButton.x <= protocol_button_x + 32 &&
                 event.mouseButton.y >= 2 && event.mouseButton.y <= 34) {
        protocol_preview = !protocol_preview;
        protocol_scroll = 0;
        tab_event = true;
      } else if (!tab_event && event.type == sf::Event::MouseButtonPressed &&
                 event.mouseButton.button == sf::Mouse::Left &&
                 event.mouseButton.y >= 0 && event.mouseButton.y <= TAB_HEIGHT &&
                 event.mouseButton.x >= MENU_BAR_X) {
        const int selected_tab = (event.mouseButton.x - MENU_BAR_X) / TAB_WIDTH;
        if (selected_tab >= 0 && selected_tab < tabs.size()) {
          const float tab_x = MENU_BAR_X + selected_tab * TAB_WIDTH;
          const float local_x = event.mouseButton.x - tab_x;
          if (local_x >= TAB_WIDTH - 22) {
            const int old_active_id = tabs[active_tab].id;
            close_tab_tree(tabs, tabs[selected_tab].id);
            if (tabs.empty()) {
              SketchTab tab;
              tab.geometry = std::make_unique<GeometryVisual>(MENU_BAR_X);
              tab.title = "Diagram " + std::to_string(next_tab_id + 1);
              tab.id = next_tab_id++;
              tabs.push_back(std::move(tab));
            }
            const int preserved_active = find_tab_by_id(tabs, old_active_id);
            active_tab = preserved_active == -1
              ? std::min(selected_tab, static_cast<int>(tabs.size()) - 1)
              : preserved_active;
            dragged_tab = -1;
            protocol_scroll = 0;
            toolbar.geomv = tabs[active_tab].geometry.get();
          } else if (local_x >= TAB_WIDTH - 44) {
            active_tab = selected_tab;
            rename_title_tab_id = tabs[selected_tab].id;
            rename_title_text = tabs[selected_tab].title;
            protocol_scroll = 0;
            toolbar.geomv = tabs[active_tab].geometry.get();
          } else {
            active_tab = selected_tab;
            dragged_tab = selected_tab;
            protocol_scroll = 0;
            toolbar.geomv = tabs[active_tab].geometry.get();
          }
        }
        tab_event = true;
      }
      if (!tab_event && dragged_tab != -1 && event.type == sf::Event::MouseMoved) {
        const int target = std::clamp(
          static_cast<int>((event.mouseMove.x - MENU_BAR_X) / TAB_WIDTH),
          0, static_cast<int>(tabs.size()) - 1
        );
        if (target != dragged_tab) {
          SketchTab moved = std::move(tabs[dragged_tab]);
          tabs.erase(tabs.begin() + dragged_tab);
          tabs.insert(tabs.begin() + target, std::move(moved));
          dragged_tab = target;
          active_tab = target;
          toolbar.geomv = tabs[active_tab].geometry.get();
        }
        tab_event = true;
      }
      if (!tab_event && dragged_tab != -1 && event.type == sf::Event::MouseButtonReleased &&
          event.mouseButton.button == sf::Mouse::Left) {
        dragged_tab = -1;
        tab_event = true;
      }
      if (!tab_event && protocol_preview &&
          (event.type == sf::Event::MouseWheelScrolled || event.type == sf::Event::MouseWheelMoved)) {
        const float panel_width = std::min(
          520.f, static_cast<float>(window.getSize().x - MENU_BAR_X)
        );
        const float panel_x = window.getSize().x - panel_width;
        const int mouse_x = event.type == sf::Event::MouseWheelScrolled
          ? event.mouseWheelScroll.x : event.mouseWheel.x;
        const int mouse_y = event.type == sf::Event::MouseWheelScrolled
          ? event.mouseWheelScroll.y : event.mouseWheel.y;
        if (mouse_x >= panel_x && mouse_y >= TAB_HEIGHT) {
          const float delta = event.type == sf::Event::MouseWheelScrolled
            ? event.mouseWheelScroll.delta : event.mouseWheel.delta;
          protocol_scroll = std::max(0, protocol_scroll - static_cast<int>(delta * 3));
          const int line_count = protocol_preview_lines(visible_protocol(tabs, active_tab)).size();
          protocol_scroll = std::min(protocol_scroll, std::max(0, line_count - 1));
          tab_event = true;
        }
      }
      if (!tab_event && protocol_preview && event.type == sf::Event::KeyPressed &&
          event.key.code == sf::Keyboard::Escape) {
        protocol_preview = false;
        tab_event = true;
      }
      if (!tab_event && protocol_preview && event.type == sf::Event::MouseButtonPressed &&
          event.mouseButton.x >= window.getSize().x - 520 && event.mouseButton.y >= TAB_HEIGHT) {
        tab_event = true;
      }
      if (!tab_event && toolbar.handleEvent(event)) {
        tab_event = true;
      }
      if (!tab_event && tabs[active_tab].geometry->handleCameraEvent(event, window)) {
        tab_event = true;
      }

      if (!tab_event && !tabs[active_tab].is_linked()) {
        menu.onEvent(event);
        tabs[active_tab].geometry->handleEvent(event, window, menu);
        ObjectStyleRequest requested_style;
        if (tabs[active_tab].geometry->take_style_request(requested_style)) {
          style_tab = active_tab;
          style_target = requested_style;
        }
        sf::Vector2f text_position;
        if (tabs[active_tab].geometry->take_text_request(text_position)) {
          text_annotation_tab = active_tab;
          text_annotation_position = text_position;
          text_annotation_value.clear();
        }
        const int rename_request = tabs[active_tab].geometry->take_rename_point_request();
        if (rename_request >= 0 && rename_request < tabs[active_tab].geometry->points.size()) {
          rename_tab = active_tab;
          rename_point = rename_request;
          rename_text = tabs[active_tab].geometry->points[rename_request].label;
          if (rename_text.empty()) {
            rename_text = tabs[active_tab].geometry->protocol.protocol["Point"][rename_request]
              .value("label", "");
          }
        }
        const std::vector<int> center_request = tabs[active_tab].geometry->take_triangle_center_request();
        if (center_request.size() == 3) {
          triangle_center_tab = active_tab;
          triangle_center_vertices = center_request;
          triangle_center_text.clear();
          triangle_center_error = false;
        }
        const int request = tabs[active_tab].geometry->take_inversion_request();
        if (request >= 0 && request < tabs[active_tab].geometry->circles.size()) {
          GeometryVisual &source = *tabs[active_tab].geometry;
          SketchTab inversion;
          inversion.geometry = std::make_unique<GeometryVisual>(MENU_BAR_X);
          inversion.title = "Inversion " + std::to_string(next_tab_id + 1);
          inversion.id = next_tab_id++;
          inversion.source_id = tabs[active_tab].id;
          inversion.inversion_circle = source.protocol.protocol["Circle"][request];
          inversion.source_signature = source.protocol.get_string_format();
          inversion.geometry->build_inversion(source, request);
          tabs.push_back(std::move(inversion));
          active_tab = tabs.size() - 1;
          toolbar.geomv = tabs[active_tab].geometry.get();
        }
      }

      if (!tab_event && !tabs[active_tab].is_linked() && event.type == sf::Event::KeyPressed &&
          event.key.code == sf::Keyboard::Enter) {
        sf::String s = command_prompt->getText();
        cout << "string = " << s.toAnsiString() << endl;
        command_prompt->clearSelectedText();
      }

      if (event.type == sf::Event::Closed) {
        window.close();
      }
    }

    for (int i = 0; i < tabs.size(); ++i) refresh_linked_tab(tabs, i);

    tabs[active_tab].geometry->draw(window, &font);
    toolbar.draw_tools(window);

    sf::Text text;
    text.setFont(font);
    text.setString("");
    // text.setString(geomv.protocol);
    text.setCharacterSize(12);
    text.setFillColor(sf::Color::Black);
    text.setPosition(10, 380);
    window.draw(text);

    sf::RectangleShape tab_bar(sf::Vector2f(window.getSize().x - MENU_BAR_X, TAB_HEIGHT));
    tab_bar.setPosition(MENU_BAR_X, 0);
    tab_bar.setFillColor(sf::Color(225, 225, 225));
    window.draw(tab_bar);
    for (int i = 0; i < tabs.size(); ++i) {
      sf::RectangleShape tab(sf::Vector2f(TAB_WIDTH, TAB_HEIGHT));
      tab.setPosition(MENU_BAR_X + i * TAB_WIDTH, 0);
      tab.setFillColor(i == active_tab ? sf::Color::White : sf::Color(205, 205, 205));
      tab.setOutlineColor(sf::Color::Black);
      tab.setOutlineThickness(1);
      window.draw(tab);
      sf::Text title;
      title.setFont(font);
      const std::string displayed_title = tabs[i].title.size() > 13
        ? tabs[i].title.substr(0, 11) + "..." : tabs[i].title;
      title.setString(displayed_title);
      title.setCharacterSize(14);
      title.setFillColor(sf::Color::Black);
      const float tab_x = MENU_BAR_X + i * TAB_WIDTH;
      title.setPosition(tab_x + 10, 9);
      window.draw(title);
      sf::VertexArray rename_icon(sf::Lines, 2);
      rename_icon[0] = sf::Vertex(sf::Vector2f(tab_x + TAB_WIDTH - 39, 24), sf::Color::Black);
      rename_icon[1] = sf::Vertex(sf::Vector2f(tab_x + TAB_WIDTH - 27, 12), sf::Color::Black);
      window.draw(rename_icon);
      sf::VertexArray close_icon(sf::Lines, 4);
      close_icon[0] = sf::Vertex(sf::Vector2f(tab_x + TAB_WIDTH - 17, 12), sf::Color::Black);
      close_icon[1] = sf::Vertex(sf::Vector2f(tab_x + TAB_WIDTH - 7, 24), sf::Color::Black);
      close_icon[2] = sf::Vertex(sf::Vector2f(tab_x + TAB_WIDTH - 7, 12), sf::Color::Black);
      close_icon[3] = sf::Vertex(sf::Vector2f(tab_x + TAB_WIDTH - 17, 24), sf::Color::Black);
      window.draw(close_icon);
    }

    if (protocol_preview) {
      const float panel_width = std::min(520.f, static_cast<float>(window.getSize().x - MENU_BAR_X));
      const float panel_x = window.getSize().x - panel_width;
      sf::RectangleShape panel(sf::Vector2f(panel_width, window.getSize().y - TAB_HEIGHT));
      panel.setPosition(panel_x, TAB_HEIGHT);
      panel.setFillColor(sf::Color(28, 31, 38, 245));
      panel.setOutlineColor(sf::Color::Black);
      panel.setOutlineThickness(1);
      window.draw(panel);

      const int linked_source = tabs[active_tab].is_linked()
        ? find_tab_by_id(tabs, tabs[active_tab].source_id) : -1;
      const int protocol_tab = linked_source == -1 ? active_tab : linked_source;
      sf::Text heading;
      heading.setFont(font);
      heading.setString("Protocol - " + tabs[protocol_tab].title);
      heading.setCharacterSize(16);
      heading.setFillColor(sf::Color::White);
      heading.setPosition(panel_x + 14, TAB_HEIGHT + 10);
      window.draw(heading);

      const std::vector<std::string> lines = protocol_preview_lines(visible_protocol(tabs, active_tab));
      const int visible_lines = std::max(1, static_cast<int>((window.getSize().y - TAB_HEIGHT - 50) / 16));
      protocol_scroll = std::min(protocol_scroll, std::max(0, static_cast<int>(lines.size()) - visible_lines));
      std::string content;
      for (int i = protocol_scroll; i < lines.size() && i < protocol_scroll + visible_lines; ++i) {
        content += lines[i] + "\n";
      }
      sf::Text protocol_text;
      protocol_text.setFont(font);
      protocol_text.setString(content);
      protocol_text.setCharacterSize(12);
      protocol_text.setFillColor(sf::Color(225, 230, 235));
      protocol_text.setPosition(panel_x + 14, TAB_HEIGHT + 38);
      window.draw(protocol_text);
    }

    const float new_tab_button_x = window.getSize().x - 84.f;
    sf::RectangleShape new_tab_button(sf::Vector2f(32, 32));
    new_tab_button.setPosition(new_tab_button_x, 2);
    new_tab_button.setFillColor(sf::Color::White);
    new_tab_button.setOutlineColor(sf::Color::Black);
    new_tab_button.setOutlineThickness(1);
    window.draw(new_tab_button);
    sf::VertexArray plus(sf::Lines, 4);
    plus[0] = sf::Vertex(sf::Vector2f(new_tab_button_x + 8, 18), sf::Color::Black);
    plus[1] = sf::Vertex(sf::Vector2f(new_tab_button_x + 24, 18), sf::Color::Black);
    plus[2] = sf::Vertex(sf::Vector2f(new_tab_button_x + 16, 10), sf::Color::Black);
    plus[3] = sf::Vertex(sf::Vector2f(new_tab_button_x + 16, 26), sf::Color::Black);
    window.draw(plus);

    const float duplicate_tab_button_x = window.getSize().x - 124.f;
    sf::RectangleShape duplicate_back(sf::Vector2f(14, 14));
    duplicate_back.setPosition(duplicate_tab_button_x + 7, 9);
    duplicate_back.setFillColor(sf::Color::White);
    duplicate_back.setOutlineColor(sf::Color::Black);
    duplicate_back.setOutlineThickness(1);
    sf::RectangleShape duplicate_front(sf::Vector2f(14, 14));
    duplicate_front.setPosition(duplicate_tab_button_x + 12, 14);
    duplicate_front.setFillColor(sf::Color::White);
    duplicate_front.setOutlineColor(sf::Color::Black);
    duplicate_front.setOutlineThickness(1);
    sf::RectangleShape duplicate_button(sf::Vector2f(32, 32));
    duplicate_button.setPosition(duplicate_tab_button_x, 2);
    duplicate_button.setFillColor(sf::Color::White);
    duplicate_button.setOutlineColor(sf::Color::Black);
    duplicate_button.setOutlineThickness(1);
    window.draw(duplicate_button);
    window.draw(duplicate_back);
    window.draw(duplicate_front);

    const float protocol_button_x = window.getSize().x - 44.f;
    sf::RectangleShape protocol_button(sf::Vector2f(32, 32));
    protocol_button.setPosition(protocol_button_x, 2);
    protocol_button.setFillColor(protocol_preview ? sf::Color(180, 210, 245) : sf::Color::White);
    protocol_button.setOutlineColor(sf::Color::Black);
    protocol_button.setOutlineThickness(1);
    window.draw(protocol_button);
    sf::VertexArray strips(sf::Lines, 6);
    for (int i = 0; i < 3; ++i) {
      strips[i * 2] = sf::Vertex(sf::Vector2f(protocol_button_x + 8, 11 + i * 7), sf::Color::Black);
      strips[i * 2 + 1] = sf::Vertex(sf::Vector2f(protocol_button_x + 24, 11 + i * 7), sf::Color::Black);
    }
    window.draw(strips);

    if (style_tab != -1) {
      const sf::Vector2f panel_position = style_panel_position(window, style_target);
      const json &style = tabs[style_tab].geometry->protocol.protocol[style_target.type][style_target.index];
      sf::RectangleShape panel(sf::Vector2f(280, 330));
      panel.setPosition(panel_position);
      panel.setFillColor(sf::Color(245, 245, 245));
      panel.setOutlineColor(sf::Color::Black);
      panel.setOutlineThickness(2);
      window.draw(panel);
      sf::Text title;
      title.setFont(font);
      title.setString(style_target.type + " style");
      title.setCharacterSize(17);
      title.setFillColor(sf::Color::Black);
      title.setPosition(panel_position.x + 14, panel_position.y + 10);
      window.draw(title);
      sf::VertexArray close(sf::Lines, 4);
      close[0] = sf::Vertex(panel_position + sf::Vector2f(254, 10), sf::Color::Black);
      close[1] = sf::Vertex(panel_position + sf::Vector2f(270, 26), sf::Color::Black);
      close[2] = sf::Vertex(panel_position + sf::Vector2f(270, 10), sf::Color::Black);
      close[3] = sf::Vertex(panel_position + sf::Vector2f(254, 26), sf::Color::Black);
      window.draw(close);
      sf::RectangleShape reset(sf::Vector2f(100, 28));
      reset.setPosition(panel_position + sf::Vector2f(15, 42));
      reset.setFillColor(sf::Color(220, 220, 220));
      reset.setOutlineColor(sf::Color::Black);
      reset.setOutlineThickness(1);
      window.draw(reset);
      sf::Text reset_text("Default", font, 14);
      reset_text.setFillColor(sf::Color::Black);
      reset_text.setPosition(panel_position + sf::Vector2f(36, 47));
      window.draw(reset_text);
      for (int i = 0; i < STYLE_COLORS.size(); ++i) {
        sf::RectangleShape swatch(sf::Vector2f(32, 30));
        swatch.setPosition(panel_position + sf::Vector2f(12 + (i % 6) * 43, 82 + (i / 6) * 38));
        swatch.setFillColor(STYLE_COLORS[i]);
        swatch.setOutlineColor(sf::Color::Black);
        swatch.setOutlineThickness(1);
        window.draw(swatch);
      }
      sf::Text thickness_label("Thickness", font, 15);
      thickness_label.setFillColor(sf::Color::Black);
      thickness_label.setPosition(panel_position + sf::Vector2f(15, 174));
      window.draw(thickness_label);
      sf::RectangleShape minus(sf::Vector2f(45, 32));
      minus.setPosition(panel_position + sf::Vector2f(15, 202));
      minus.setFillColor(sf::Color(220, 220, 220));
      minus.setOutlineColor(sf::Color::Black);
      minus.setOutlineThickness(1);
      window.draw(minus);
      sf::RectangleShape plus_button(sf::Vector2f(45, 32));
      plus_button.setPosition(panel_position + sf::Vector2f(205, 202));
      plus_button.setFillColor(sf::Color(220, 220, 220));
      plus_button.setOutlineColor(sf::Color::Black);
      plus_button.setOutlineThickness(1);
      window.draw(plus_button);
      sf::Text minus_text("-", font, 22);
      minus_text.setFillColor(sf::Color::Black);
      minus_text.setPosition(panel_position + sf::Vector2f(32, 202));
      window.draw(minus_text);
      sf::Text plus_text("+", font, 20);
      plus_text.setFillColor(sf::Color::Black);
      plus_text.setPosition(panel_position + sf::Vector2f(219, 203));
      window.draw(plus_text);
      sf::Text thickness_value(std::to_string(static_cast<int>(style.value("thickness", 1.f))), font, 18);
      thickness_value.setFillColor(sf::Color::Black);
      thickness_value.setPosition(panel_position + sf::Vector2f(132, 207));
      window.draw(thickness_value);
      if (style_target.type != "Point") {
        const bool dashed = style.value("dashed", false);
        sf::RectangleShape solid(sf::Vector2f(110, 34));
        solid.setPosition(panel_position + sf::Vector2f(15, 265));
        solid.setFillColor(dashed ? sf::Color(220, 220, 220) : sf::Color(170, 205, 245));
        solid.setOutlineColor(sf::Color::Black);
        solid.setOutlineThickness(1);
        window.draw(solid);
        sf::RectangleShape dashed_button(sf::Vector2f(110, 34));
        dashed_button.setPosition(panel_position + sf::Vector2f(145, 265));
        dashed_button.setFillColor(dashed ? sf::Color(170, 205, 245) : sf::Color(220, 220, 220));
        dashed_button.setOutlineColor(sf::Color::Black);
        dashed_button.setOutlineThickness(1);
        window.draw(dashed_button);
        sf::Text solid_text("Solid", font, 14);
        solid_text.setFillColor(sf::Color::Black);
        solid_text.setPosition(panel_position + sf::Vector2f(50, 273));
        window.draw(solid_text);
        sf::Text dashed_text("Dashed", font, 14);
        dashed_text.setFillColor(sf::Color::Black);
        dashed_text.setPosition(panel_position + sf::Vector2f(174, 273));
        window.draw(dashed_text);
      }
    } else if (rename_title_tab_id != -1) {
      const float prompt_x = (window.getSize().x - 420.f) / 2;
      const float prompt_y = (window.getSize().y - 120.f) / 2;
      sf::RectangleShape prompt(sf::Vector2f(420, 120));
      prompt.setPosition(prompt_x, prompt_y);
      prompt.setFillColor(sf::Color(245, 245, 245));
      prompt.setOutlineColor(sf::Color::Black);
      prompt.setOutlineThickness(2);
      window.draw(prompt);
      sf::Text prompt_title;
      prompt_title.setFont(font);
      prompt_title.setString("Rename tab - Enter to save, Esc to cancel");
      prompt_title.setCharacterSize(15);
      prompt_title.setFillColor(sf::Color::Black);
      prompt_title.setPosition(prompt_x + 16, prompt_y + 14);
      window.draw(prompt_title);
      sf::Text prompt_value;
      prompt_value.setFont(font);
      prompt_value.setString(rename_title_text + "|");
      prompt_value.setCharacterSize(22);
      prompt_value.setFillColor(sf::Color::Black);
      prompt_value.setPosition(prompt_x + 16, prompt_y + 58);
      window.draw(prompt_value);
    } else if (text_annotation_tab != -1) {
      const float prompt_x = (window.getSize().x - 520.f) / 2;
      const float prompt_y = (window.getSize().y - 120.f) / 2;
      sf::RectangleShape prompt(sf::Vector2f(520, 120));
      prompt.setPosition(prompt_x, prompt_y);
      prompt.setFillColor(sf::Color(245, 245, 245));
      prompt.setOutlineColor(sf::Color::Black);
      prompt.setOutlineThickness(2);
      window.draw(prompt);
      sf::Text prompt_title;
      prompt_title.setFont(font);
      prompt_title.setString("Add text - Enter to place, Esc to cancel");
      prompt_title.setCharacterSize(15);
      prompt_title.setFillColor(sf::Color::Black);
      prompt_title.setPosition(prompt_x + 16, prompt_y + 14);
      window.draw(prompt_title);
      sf::Text prompt_value;
      prompt_value.setFont(font);
      prompt_value.setString(text_annotation_value + "|");
      prompt_value.setCharacterSize(20);
      prompt_value.setFillColor(sf::Color::Black);
      prompt_value.setPosition(prompt_x + 16, prompt_y + 58);
      window.draw(prompt_value);
    } else if (!triangle_center_vertices.empty()) {
      const float prompt_x = (window.getSize().x - 420.f) / 2;
      const float prompt_y = (window.getSize().y - 120.f) / 2;
      sf::RectangleShape prompt(sf::Vector2f(420, 120));
      prompt.setPosition(prompt_x, prompt_y);
      prompt.setFillColor(sf::Color(245, 245, 245));
      prompt.setOutlineColor(triangle_center_error ? sf::Color::Red : sf::Color::Black);
      prompt.setOutlineThickness(2);
      window.draw(prompt);
      sf::Text prompt_title;
      prompt_title.setFont(font);
      prompt_title.setString(triangle_center_error
        ? "Enter a valid center X(1) through X(100)"
        : "Triangle center number X(n) - Enter to create");
      prompt_title.setCharacterSize(15);
      prompt_title.setFillColor(sf::Color::Black);
      prompt_title.setPosition(prompt_x + 16, prompt_y + 14);
      window.draw(prompt_title);
      sf::Text prompt_value;
      prompt_value.setFont(font);
      prompt_value.setString("X(" + triangle_center_text + "|)");
      prompt_value.setCharacterSize(22);
      prompt_value.setFillColor(sf::Color::Black);
      prompt_value.setPosition(prompt_x + 16, prompt_y + 58);
      window.draw(prompt_value);
    } else if (rename_point != -1) {
      const float prompt_x = (window.getSize().x - 420.f) / 2;
      const float prompt_y = (window.getSize().y - 120.f) / 2;
      sf::RectangleShape prompt(sf::Vector2f(420, 120));
      prompt.setPosition(prompt_x, prompt_y);
      prompt.setFillColor(sf::Color(245, 245, 245));
      prompt.setOutlineColor(sf::Color::Black);
      prompt.setOutlineThickness(2);
      window.draw(prompt);
      sf::Text prompt_title;
      prompt_title.setFont(font);
      prompt_title.setString("Rename point - Enter to save, Esc to cancel");
      prompt_title.setCharacterSize(15);
      prompt_title.setFillColor(sf::Color::Black);
      prompt_title.setPosition(prompt_x + 16, prompt_y + 14);
      window.draw(prompt_title);
      sf::Text prompt_value;
      prompt_value.setFont(font);
      prompt_value.setString(rename_text + "|");
      prompt_value.setCharacterSize(22);
      prompt_value.setFillColor(sf::Color::Black);
      prompt_value.setPosition(prompt_x + 16, prompt_y + 58);
      window.draw(prompt_value);
    }

    window.display();
    window.clear(backgroundColor);
  }
  return 0;
}






