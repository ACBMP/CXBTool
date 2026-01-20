#pragma once
#include <string>
#include <map>
#include <vector>
#include <stdint.h>
#include <pugixml.hpp>

struct CXBFile {
    std::string name;
    std::string rawXml;
};


std::map<std::string, std::string> ExtractCXB(const std::string& path);
std::vector<uint8_t> ConvertToCXB(const std::vector<CXBFile>& files);
