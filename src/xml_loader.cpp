#include "xml_loader.hpp"
#include <iostream>
#include <memory>

std::map<std::string, std::unique_ptr<tinyxml2::XMLDocument>> LoadXMLFromMemory(const std::map<std::string, std::string>& xmlMap) {
    std::map<std::string, std::unique_ptr<tinyxml2::XMLDocument>> docs;

    for (const auto& [filename, content] : xmlMap) {
        auto doc = std::make_unique<tinyxml2::XMLDocument>();
        if (doc->Parse(content.c_str()) == tinyxml2::XML_SUCCESS) {
            docs[filename] = std::move(doc);
        } else {
            std::cerr << "Failed to parse " << filename << ": " << doc->ErrorStr() << "\n";
        }
    }

    return docs;
}

