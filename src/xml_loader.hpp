#pragma once
#include <string>
#include <map>
#include <tinyxml2.h>
#include <memory>

std::map<std::string, std::unique_ptr<tinyxml2::XMLDocument>> LoadXMLFromMemory(const std::map<std::string, std::string>& xmlMap);
