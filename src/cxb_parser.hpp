#pragma once
#include <string>
#include <map>
#include <vector>
#include <stdint.h>
#include <pugixml.hpp>
#include <array>

struct CXBFile {
    std::string name;
    std::string rawXml;
};
constexpr std::array<uint8_t, 2> CXB_CHUNK_MARKER = {0x01, 0x0A};

std::map<std::string, std::string> ExtractCXB(const std::string& path);
std::vector<uint8_t> ConvertToCXB(const std::vector<CXBFile>& files, uint8_t nameFieldSize);
