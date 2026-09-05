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
  ToolView toolbar(&menu, tabs[0].geometry.get(), SCREEN_X, SCREEN_Y, MENU_BAR_X);

  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      bool tab_event = false;
      if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left &&
          event.mouseButton.y >= 0 && event.mouseButton.y <= TAB_HEIGHT &&
          event.mouseButton.x >= MENU_BAR_X) {
        const int selected_tab = (event.mouseButton.x - MENU_BAR_X) / TAB_WIDTH;
        if (selected_tab >= 0 && selected_tab < tabs.size()) {
          active_tab = selected_tab;
          toolbar.geomv = tabs[active_tab].geometry.get();
        }
        tab_event = true;
      }

      if (!tab_event && !tabs[active_tab].is_linked()) {
        menu.onEvent(event);
        tabs[active_tab].geometry->handleEvent(event, window, menu);
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

      if (!tabs[active_tab].is_linked() && event.type == sf::Event::KeyPressed &&
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

    tabs[active_tab].geometry->draw(window);
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

    window.display();
    window.clear(backgroundColor);
  }
  return 0;
}






