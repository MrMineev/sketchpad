#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <memory>

#include <SFML/Graphics.hpp>

#include "src/view/toolbar.h"
#include "gui_tools/src/Gui/Gui.hpp"
#include "src/core/geometry.h"
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

struct SketchTab {
  std::unique_ptr<GeometryVisual> geometry;
  std::string title;
  int source_tab = -1;
  json inversion_circle;
  std::string source_signature;

  bool is_linked() const {
    return source_tab != -1;
  }
};

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
  if (!tab.is_linked() || tab.source_tab < 0 || tab.source_tab >= tabs.size()) return;
  GeometryVisual &source = *tabs[tab.source_tab].geometry;
  const std::string signature = source.protocol.get_string_format();
  if (signature == tab.source_signature) return;
  tab.geometry->build_inversion(source, find_inversion_circle(source.protocol, tab.inversion_circle));
  tab.source_signature = signature;
}

const Protocol &visible_protocol(const std::vector<SketchTab> &tabs, int active_tab) {
  const SketchTab &tab = tabs[active_tab];
  return tab.is_linked() ? tabs[tab.source_tab].geometry->protocol : tab.geometry->protocol;
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
  tabs.push_back(std::move(original));
  int active_tab = 0;
  bool protocol_preview = false;
  int protocol_scroll = 0;
  int rename_tab = -1;
  int rename_point = -1;
  std::string rename_text;
  ToolView toolbar(&menu, tabs[0].geometry.get(), SCREEN_X, SCREEN_Y, MENU_BAR_X);

  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      bool tab_event = false;
      if (rename_point != -1) {
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
      if (!tab_event && event.type == sf::Event::MouseButtonPressed &&
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
          active_tab = selected_tab;
          protocol_scroll = 0;
          toolbar.geomv = tabs[active_tab].geometry.get();
        }
        tab_event = true;
      }
      if (!tab_event && protocol_preview && event.type == sf::Event::MouseWheelScrolled) {
        protocol_scroll = std::max(0, protocol_scroll - static_cast<int>(event.mouseWheelScroll.delta * 3));
        const int line_count = protocol_preview_lines(visible_protocol(tabs, active_tab)).size();
        protocol_scroll = std::min(protocol_scroll, std::max(0, line_count - 1));
        tab_event = true;
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

      if (!tab_event && !tabs[active_tab].is_linked()) {
        menu.onEvent(event);
        tabs[active_tab].geometry->handleEvent(event, window, menu);
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
        const int request = tabs[active_tab].geometry->take_inversion_request();
        if (request >= 0 && request < tabs[active_tab].geometry->circles.size()) {
          GeometryVisual &source = *tabs[active_tab].geometry;
          SketchTab inversion;
          inversion.geometry = std::make_unique<GeometryVisual>(MENU_BAR_X);
          inversion.title = "Inversion " + std::to_string(tabs.size());
          inversion.source_tab = active_tab;
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
      title.setString(tabs[i].title);
      title.setCharacterSize(14);
      title.setFillColor(sf::Color::Black);
      title.setPosition(MENU_BAR_X + i * TAB_WIDTH + 10, 9);
      window.draw(title);
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

      const int protocol_tab = tabs[active_tab].is_linked() ? tabs[active_tab].source_tab : active_tab;
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

    if (rename_point != -1) {
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






