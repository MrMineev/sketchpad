#include "parse.h"

#include <iostream>
#include <string>
#include <vector>

std::vector<std::string> tokenize(std::string &s) {
  std::string curr = "";
  std::vector<std::string> order;
  for (char c : s) {
    if (c == '[' || c == ']' || c == ',' || c == ' ' || c == '=' || c == '(' || c == ')' || c == ';' || c == '\n') { 
      if (curr.size() != 0) order.push_back(curr);
      curr.clear();
    } else {
      curr += c;
    }
  }
  if (curr.size() != 0) order.push_back(curr);
  return order;
}

