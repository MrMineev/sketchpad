#pragma once

#include <SFML/Graphics.hpp>

#include <iostream>
#include <vector>
#include <string>

// #include ".h"
#include "../../gui_tools/src/Gui/Gui.hpp"
#include "../../src/core/geometry.h"

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
    "Inversion (Partial)",
    "Incenter",
    "Excenter"
  };

  void draw_tools(sf::RenderWindow &window) {
    window.draw(rectangle);
    window.draw(*menu);
  }

  void setup() {
    for (ll i = 0; i < (ll)tools_names.size(); i++) {
      gui::Button* button = new gui::Button(tools_names[i]);
      button->setCallback([this, i] {
        geomv->current_tool = i;
      });
      menu->add(button);
    }
  }

  ToolView(gui::Menu* _menu, GeometryVisual* _geomv, int width, int height, int menu_bar_x) {
    menu = _menu;
    geomv = _geomv;
    setup();

    const int w = menu_bar_x;
    const int h = height;
    rectangle = sf::RectangleShape(sf::Vector2f((float)w, (float)h));
    rectangle.setPosition(0, 0);
    rectangle.setFillColor(sf::Color::White);
  }
};




