#include "parse.h"

#include "../core/primitives/point.h"
#include "../core/primitives/line.h"
#include "../core/primitives/circle.h"

std::vector<std::string> tokenize(std::string &s) {
  std::string curr = "";
  std::vector<std::string> order;
  for (char c : s) {
    if (c == '=' || c == '(' || c == ')' || c == ';'){ 
      curr.clear();
      curr += c;
      order.push_back(curr);
      curr.clear();
    } else {
      curr += c;
    }
  }
  return std::vector<std::string>();
}

std::vector<std::any> parse(std::string &s) {
  return std::vector<std::any>();
}

