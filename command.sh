#!/bin/bash

cd src
g++ main.cpp src/view/button.cpp -o main -I /opt/homebrew/Cellar/sfml/2.6.1/include/ -L /opt/homebrew/Cellar/sfml/2.6.1/lib/ -lsfml-graphics -lsfml-window -lsfml-system -std=c++17

