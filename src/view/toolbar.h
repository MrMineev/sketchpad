#pragma once

#include <SFML/Graphics.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

// #include ".h"
#include "../../gui_tools/src/Gui/Gui.hpp"
#include "../../src/core/geometry.h"
#include "../../filebrowser/browser.h"

using namespace std;

typedef long long ll;
typedef long double ld;
typedef vector<ll> vll;
typedef vector<vll> vvll;
typedef vector<vvll> vvvll;

struct ToolView {
  gui::Menu* menu;
  GeometryVisual* geomv;
  sf::RectangleShape rectangle;
  int toolbar_width;
  int toolbar_height;
  float scroll_offset = 0;

  vector<string> tools_names = {
    "Mouse",
    "Point",
    "Segment",
    "Circle",
    "Midpoint",
    "Circle (three points)",
    "Line",
    "Intersect Lines",
    "Perpendicular Normal",
    "Invert Diagram",
    "Incenter",
    "Excenter",
    "Intersect Line & Circle",
    "SEARCHER TACTIC",
    "Parallel Line",
    "Conic (Through Five Points)",
    "Cubic (Through Nine Points)",
    "Angle Bisector",
    "Reflect Line Over Line",
    "Isogonal Conjugate",
    "Reflect Point Over Line",
    "Reflect Point Over Point",
    "Project Point Onto Line",
    "Circumcenter",
    "Triangle Center",
    "Isotomic Conjugate",
    "Rectangular Hyperbola",
    "Center",
    "Text",
    "Hide Object",
    "Hide Label",
    "Rename Point",
    "Intersect Objects",
    "Tangents: Point-Circle",
    "Common Circle Tangents",
    "Polar Line",
    "Radical Axis",
    "Radical Center",
    "Show All",
    "Save",
    "Open",
  };

  float max_scroll() const {
    return std::max(0.f, menu->getSize().y + 20 - toolbar_height);
  }

  void update_menu_position() {
    scroll_offset = std::clamp(scroll_offset, 0.f, max_scroll());
    menu->setPosition(10, 10 - scroll_offset);
  }

  void resize(int height) {
    toolbar_height = std::max(1, height);
    rectangle.setSize(sf::Vector2f(toolbar_width, toolbar_height));
    update_menu_position();
  }

  bool handleEvent(const sf::Event &event) {
    float delta = 0;
    int mouse_x = toolbar_width + 1;
    if (event.type == sf::Event::MouseWheelScrolled) {
      delta = event.mouseWheelScroll.delta;
      mouse_x = event.mouseWheelScroll.x;
    } else if (event.type == sf::Event::MouseWheelMoved) {
      delta = event.mouseWheel.delta;
      mouse_x = event.mouseWheel.x;
    } else {
      return false;
    }
    if (mouse_x < 0 || mouse_x > toolbar_width) return false;
    scroll_offset -= delta * 45;
    update_menu_position();
    return true;
  }

  void draw_tools(sf::RenderWindow &window) {
    const sf::View previous_view = window.getView();
    sf::View toolbar_view(sf::FloatRect(0, 0, toolbar_width, toolbar_height));
    toolbar_view.setViewport(sf::FloatRect(
      0, 0, static_cast<float>(toolbar_width) / window.getSize().x, 1
    ));
    window.setView(toolbar_view);
    window.draw(rectangle);
    window.draw(*menu);
    const float maximum = max_scroll();
    if (maximum > 0) {
      const float content_height = menu->getSize().y + 20;
      const float thumb_height = std::max(30.f, toolbar_height * toolbar_height / content_height);
      const float thumb_y = scroll_offset / maximum * (toolbar_height - thumb_height);
      sf::RectangleShape scrollbar(sf::Vector2f(5, thumb_height));
      scrollbar.setPosition(toolbar_width - 7, thumb_y);
      scrollbar.setFillColor(sf::Color(130, 130, 130));
      window.draw(scrollbar);
    }
    window.setView(previous_view);
  }

  void setup() {
    for (ll i = 0; i < (ll)tools_names.size(); i++) {
      gui::Button* button = new gui::Button(tools_names[i]);
      if (tools_names[i] == "Show All") {
        button->setCallback([this] {
          geomv->show_all();
        });
      } else if (i < (ll)tools_names.size() - 2) {
        button->setCallback([this, i] {
          geomv->current_tool = i;
        });
      } else if (i == (ll)tools_names.size() - 2) {
        // Save
        button->setCallback([this, i] {
          FileExplorer explorer;
          string conf_sel_pathway_end = explorer.run();
          geomv->save_configuration(conf_sel_pathway_end);
        });
      } else if (i == (ll)tools_names.size() - 1) {
        // Open
        button->setCallback([this, i] {
          FileExplorer explorer;
          string conf_sel_pathway_end = explorer.run();
          geomv->load_configuration(conf_sel_pathway_end);
          std::cout << "CONFIGURATION LOADED!" << std::endl;
        });
      }
      menu->add(button);
    }
  }

  ToolView(gui::Menu* _menu, GeometryVisual* _geomv, int width, int height, int menu_bar_x) {
    menu = _menu;
    geomv = _geomv;
    toolbar_width = menu_bar_x;
    toolbar_height = height;
    setup();
    update_menu_position();

    rectangle = sf::RectangleShape(sf::Vector2f((float)toolbar_width, (float)toolbar_height));
    rectangle.setPosition(0, 0);
    rectangle.setFillColor(sf::Color::White);
  }
};




