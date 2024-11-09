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

  vector<string> tools_names = {
    "Mouse", "Point", "Line", "Circle"
  };

  void draw_tools(sf::RenderWindow &window) {
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

  ToolView(gui::Menu* _menu, GeometryVisual* _geomv) {
    menu = _menu;
    geomv = _geomv;
    setup();
  }
};




