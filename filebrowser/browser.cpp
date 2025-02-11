#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <sstream>

#include "browser.h"

namespace fs = std::filesystem;

std::string FileExplorer::run() {
    while (window.isOpen()) {
        if (selected_filepath != "") { break; }
        handleEvents();
        window.clear(sf::Color::White);
        displayDirectory();
        window.display();
    }
    return selected_filepath;
}

void FileExplorer::handleEvents() {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            window.close();
        } 
        if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Up && selectedIndex > 0) {
                selectedIndex--;
            } 
            if (event.key.code == sf::Keyboard::Down && selectedIndex < fileNames.size() - 1) {
                selectedIndex++;
            }
            if (event.key.code == sf::Keyboard::Enter) {
                navigate(fileNames[selectedIndex]);
            }
            if (event.key.code == sf::Keyboard::Backspace) {
                navigate("..");
            }
        }
    }
}

void FileExplorer::loadDirectory(const fs::path& dir) {
    fileNames.clear();
    fileInfos.clear();
    
    // Add ".." (parent directory) entry if it's not the root directory
    if (dir != fs::current_path()) {
        fileNames.push_back("..");
        fileInfos.push_back(fs::directory_entry(fs::path("..")));
    }

    // Add files and directories in the current directory
    for (const auto& entry : fs::directory_iterator(dir)) {
        fileNames.push_back(entry.path().filename().string());
        fileInfos.push_back(entry);  // Add directory_entry object here
    }
}

void FileExplorer::navigate(const std::string& filename) {
    fs::path newDir = currentDir / filename;
    if (filename == "..") {
        if (currentDir.has_parent_path()) {
            currentDir = currentDir.parent_path();
        }
    } else if (fs::is_directory(newDir)) {
        currentDir = newDir;
    } else {
        std::cout << "YOU ARE TRYING TO SUBNAVIGATE A FILE..." << std::endl;
        selected_filepath = newDir.generic_string();
        std::cout << "[FILEPATH]: " << selected_filepath << std::endl;
    }
    loadDirectory(currentDir);
    selectedIndex = 0;  // Reset selection after navigating
}

void FileExplorer::displayDirectory() {
    float yOffset = 20.0f;
    for (size_t i = 0; i < fileNames.size(); ++i) {
        // Display Icons
        sf::Sprite icon;
        if (fs::is_directory(fileInfos[i])) {
            icon.setTexture(folderIcon);
        } else {
            icon.setTexture(fileIcon);
        }

        // Resize icon to a consistent size (e.g., 32x32 pixels)
        float iconSize = 32.0f;
        sf::Vector2u textureSize = icon.getTexture()->getSize();
        float scaleX = iconSize / textureSize.x;
        float scaleY = iconSize / textureSize.y;
        icon.setScale(scaleX, scaleY);

        icon.setPosition(10, yOffset + 5);
        window.draw(icon);

        // Display file/directory name and additional info
        sf::Text text(fileNames[i], font, 20);
        text.setPosition(50, yOffset);

        if (i == selectedIndex) {
            // Highlight selected item
            sf::RectangleShape background(sf::Vector2f(window.getSize().x - 45, 30));
            background.setPosition(45, yOffset - 5);
            background.setFillColor(sf::Color(60, 60, 60));  // Dark background for selected item
            window.draw(background);
            text.setFillColor(sf::Color::White);
        } else {
            text.setFillColor(sf::Color::Black);
        }

        // If it's a file, display size
        if (!fs::is_directory(fileInfos[i])) {
            std::stringstream sizeStream;
            sizeStream << formatFileSize(fs::file_size(fileInfos[i]));
            sf::Text sizeText(sizeStream.str(), font, 16);
            sizeText.setPosition(window.getSize().x - 150, yOffset + 3);
            if (i == selectedIndex) {
              sizeText.setFillColor(sf::Color::White);
            } else {
              sizeText.setFillColor(sf::Color::Black);
            }
            window.draw(sizeText);
        } else {
            // Display directory (indicating it's a directory)
            sf::Text dirText("(Directory)", font, 16);
            dirText.setPosition(window.getSize().x - 150, yOffset + 3);
            dirText.setFillColor(sf::Color(0, 100, 0));
            window.draw(dirText);
        }

        window.draw(text);
        yOffset += 40.0f;  // Adjust line spacing
    }
}

std::string FileExplorer::formatFileSize(uintmax_t size) {
    std::stringstream ss;
    if (size < 1024) {
        ss << size << " B";
    } else if (size < 1048576) {
        ss << size / 1024.0f << " KB";
    } else if (size < 1073741824) {
        ss << size / 1048576.0f << " MB";
    } else {
        ss << size / 1073741824.0f << " GB";
    }
    return ss.str();
}


/*
int main() {
    FileExplorer explorer;
    explorer.run();
    return 0;
}
*/

