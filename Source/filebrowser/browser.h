#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

class FileExplorer {
 private:
  sf::RenderWindow window;
  sf::Font font;
  sf::Texture folderIcon, fileIcon;
  fs::path currentDir;
  std::vector<std::string> fileNames;
  std::vector<fs::directory_entry> fileInfos;
  size_t selectedIndex = 0;
  std::string selected_filepath = "";

  void handleEvents();
  void loadDirectory(const fs::path& dir);
  void navigate(const std::string& filename);
  void displayDirectory();
  std::string formatFileSize(uintmax_t size);

 public:
  FileExplorer() : window(sf::VideoMode(800, 600), "Styled File Explorer") {
    font.loadFromFile("filebrowser/assets/arial.ttf");
    folderIcon.loadFromFile("filebrowser/assets/folder.png");
    fileIcon.loadFromFile("filebrowser/assets/file.png");
    currentDir = fs::current_path();
    // std::cout << "CURRENT PATHWAY OF OPERATION: " << currentDir.generic_string() << std::endl;
    loadDirectory(currentDir);
  }

  std::string run();
};



