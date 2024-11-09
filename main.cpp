#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_map>

#include <SFML/Graphics.hpp>

#include "src/view/toolbar.h"
#include "gui_tools/src/Gui/Gui.hpp"
#include "src/core/geometry.h"

using namespace std;

typedef long long ll;
typedef long double ld;
typedef vector<ll> vll;
typedef vector<vll> vvll;
typedef vector<vvll> vvvll;

const ll SCREEN_X = 1200;
const ll SCREEN_Y = 800;

signed main() {
  std::srand(static_cast<unsigned int>(std::time(nullptr)));

  sf::RenderWindow window(sf::VideoMode(SCREEN_X, SCREEN_Y), "Sketchpad");
  // Declare menu
  gui::Menu menu(window);
  menu.setPosition(10, 10);
  gui::Theme::loadFont("gui_assets/demo/tahoma.ttf");
  gui::Theme::loadTexture("gui_assets/demo/texture-default.png");

  gui::TextBox *command_prompt = new gui::TextBox();
  menu.add(command_prompt);

  sf::Color backgroundColor = sf::Color(252, 242, 172);
  GeometryVisual geomv;
  ToolView toolbar(&menu, &geomv);

  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      menu.onEvent(event);
      geomv.handleEvent(event, window, menu);

      if (event.type == sf::Event::KeyPressed &&
          event.key.code == sf::Keyboard::Enter) {
        sf::String s = command_prompt->getText();
        cout << "string = " << s.toAnsiString() << endl;
        command_prompt->clearSelectedText();
      }

      if (event.type == sf::Event::Closed) {
        window.close();
      }
    }

    toolbar.draw_tools(window);
    geomv.draw(window);
    // window.draw(menu);

    window.display();
    window.clear(backgroundColor);
  }
  return 0;
}






