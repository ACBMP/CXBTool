#include "cxb_parser.hpp"
#include "xml_loader.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>

namespace fs = std::filesystem;

// required XMLs
const std::set<std::string> REQUIRED_FILES = {
    "abilitymanagermulti", "communityinfo", "custommanagermulti",
    "gamemodemanagermulti", "gamemodeparams_advteamwanted", "gamemodeparams_advwanted",
    "gamemodeparams_assassinate", "gamemodeparams_catsmice", "gamemodeparams_free",
    "gamemodeparams_pacman", "gamemodeparams_teamvip", "gamemodeparams_teamwanted",
    "gamemodeparams_vip", "gamemodeparams_wanted_2", "gamemode_advteamwanted",
    "gamemode_advwanted", "gamemode_assassinate", "gamemode_catsmice", "gamemode_free",
    "gamemode_pacman", "gamemode_teamvip", "gamemode_teamwanted", "gamemode_vip",
    "gamemode_wanted_2", "globalparams", "levelxpmanager", "mapmanagermulti",
    "scorebonusmanager"
};

void exportCXB(const std::string& input, const std::string& outputDir) {
    auto xmls = ExtractCXB(input);
    if (xmls.empty()) {
        std::cerr << "No XMLs extracted from " << input << "\n";
        return;
    }

    fs::create_directories(outputDir);
    for (const auto& [filename, content] : xmls) {
        std::string path = (fs::path(outputDir) / filename).string();
        std::ofstream out(path);
        if (!out) {
            std::cerr << "Failed to write " << path << "\n";
            continue;
        }
        out << content;
        std::cout << "Wrote " << path << "\n";
    }
}

void convertToCXB(const std::string& inputDir, const std::string& outputFile) {
    std::map<std::string, std::string> xmlMap;

    // collect all XMLs in folder
    for (const auto& entry : fs::directory_iterator(inputDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".xml") {
            std::string baseName = entry.path().stem().string();
            std::ifstream in(entry.path());
            if (!in) {
                std::cerr << "Failed to read " << entry.path() << "\n";
                return;
            }
            std::string content((std::istreambuf_iterator<char>(in)), {});
            xmlMap[baseName] = content;
        }
    }

    // validation
    std::set<std::string> found;
    for (const auto& [name, _] : xmlMap) found.insert(name);

    if (found != REQUIRED_FILES) {
        std::cerr << "Invalid XML set in folder: " << inputDir << "\n";
        std::cerr << "Expected exactly these files:\n";
        for (auto& f : REQUIRED_FILES) std::cerr << "  " << f << ".xml\n";

        std::cerr << "Found instead:\n";
        for (auto& f : found) std::cerr << "  " << f << ".xml\n";
        return;
    }

    // parse into tinyxml2 docs
    auto docs = LoadXMLFromMemory(xmlMap);

    // build CXBFile vector
    std::vector<CXBFile> files;
    for (auto& [name, doc] : docs) {
        CXBFile f{ name + ".xml", doc.get() };
        files.push_back(f);
    }

    auto buffer = ConvertToCXB(files);

    // write output
    std::ofstream out(outputFile, std::ios::binary);
    if (!out) {
        std::cerr << "Failed to open output file " << outputFile << "\n";
        return;
    }
    out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    std::cout << "Wrote CXB to " << outputFile << "\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage:\n"
                  << "  " << argv[0] << " export <input.cxb> <output_folder>\n"
                  << "  " << argv[0] << " convert <input_folder> <output.cxb>\n";
        return 1;
    }

    std::string cmd = argv[1];
    if (cmd == "export") {
        if (argc != 4) {
            std::cerr << "Usage: " << argv[0] << " export <input.cxb> <output_folder>\n";
            return 1;
        }
        exportCXB(argv[2], argv[3]);
    } else if (cmd == "convert") {
        if (argc != 4) {
            std::cerr << "Usage: " << argv[0] << " convert <input_folder> <output.cxb>\n";
            return 1;
        }
        convertToCXB(argv[2], argv[3]);
    } else {
        std::cerr << "Unknown command: " << cmd << "\n";
        return 1;
    }

    return 0;
}

