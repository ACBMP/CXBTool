#pragma once
#include <string>
#include <map>
#include "tinyxml2.h"
#include <vector>

struct CXBFile {
    std::string name;
    tinyxml2::XMLDocument* doc;
};


std::map<std::string, std::string> ExtractCXB(const std::string& path);
std::vector<uint8_t> ConvertToCXB(const std::vector<CXBFile>& files);
