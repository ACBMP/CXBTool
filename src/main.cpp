#include "cxb_parser.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <stdint.h>
#include <pugixml.hpp>
#include <getopt.h>

namespace fs = std::filesystem;

static struct option long_options[] = {
    {"chunk-size", required_argument, nullptr, 'c'},
    {"name-field-size", required_argument, nullptr, 'n'},
    {nullptr, 0, nullptr, 0}
};

void exportCXB(const std::string& input, const std::string& outputDir) {
    auto xmls = ExtractCXB(input);
    if (xmls.empty()) {
        std::cerr << "No XMLs extracted from " << input << "\n";
        return;
    }

    fs::create_directories(outputDir);
    for (const auto& [filename, content] : xmls) {
        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_string(content.c_str());
        if (!result) {
            std::cerr << "Failed to parse XML for " << filename
                      << ": " << result.description() << "\n";
            continue;
        }
        std::string path = (fs::path(outputDir) / filename).string();
        std::ofstream out(path);
        if (!out) {
            std::cerr << "Failed to write " << path << "\n";
            continue;
        }
        doc.save(out, "    ", pugi::format_default | pugi::format_indent);
        std::cout << "Wrote " << path << "\n";
    }
}

void convertToCXB(const std::string& inputDir, const std::string& outputFile, size_t chunkSize, uint8_t nameFieldSize) {
    std::map<std::string, std::string> xmlMap;

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

    std::vector<CXBFile> files;
    for (auto& [baseName, content] : xmlMap) {
        CXBFile f;
        f.name = baseName + ".xml";
        f.rawXml = content;
        files.push_back(std::move(f));
    }

    auto buffer = ConvertToCXB(files, nameFieldSize);

    if (chunkSize <= 0) {
        std::ofstream out(outputFile, std::ios::binary);
        if (!out) {
            std::cerr << "Failed to open output file " << outputFile << "\n";
            return;
        }
        out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
        std::cout << "Wrote CXB to " << outputFile << "\n";
    } else {
        size_t chunkBytes = static_cast<size_t>(chunkSize) * 1024 - 1;
        size_t totalSize = buffer.size();
        size_t numChunks = (totalSize + chunkBytes - 1) / chunkBytes;

        fs::path outPath(outputFile);
        fs::path parentDir = outPath.parent_path();
        std::string stem = outPath.stem().string();
        std::string ext = outPath.extension().string();

        for (size_t i = 0; i < numChunks; ++i) {
            size_t start = i * chunkBytes;
            size_t end = std::min(start + chunkBytes, totalSize);

            std::vector<uint8_t> chunk(buffer.begin() + start, buffer.begin() + end);
            chunk.push_back(CXB_CHUNK_MARKER[0]);
            chunk.push_back(CXB_CHUNK_MARKER[1]);

            std::ostringstream oss;
            oss << stem << "_" << static_cast<char>('a' + i) << ext;
            std::string chunkName = (parentDir / oss.str()).string();

            std::ofstream out(chunkName, std::ios::binary);
            if (!out) {
                std::cerr << "Failed to write chunk " << chunkName << "\n";
                continue;
            }
            out.write(reinterpret_cast<const char*>(chunk.data()), chunk.size());
            std::cout << "Wrote chunk " << chunkName << "\n";
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage:\n"
                  << "  " << argv[0] << " export <input.cxb> <output_folder>\n"
                  << "  " << argv[0] << " convert [-c|--chunk-size <KB>] [-n|--name-field-size <bytes>] <input_folder> <output.cxb>\n";
        return 1;
    }

    std::string cmd = argv[1];

    size_t chunkSizeKB = 0;
    uint8_t nameFieldSize = 32;

    optind = 2;

    int opt;
    while ((opt = getopt_long(argc, argv, "c:n:", long_options, nullptr)) != -1) {
        switch (opt) {
        case 'c':
            try {
                chunkSizeKB = std::stoul(optarg);
            } catch (...) {
                std::cerr << "Invalid chunk size: " << optarg << "\n";
                return 1;
            }
            break;
        case 'n':
            try {
                nameFieldSize = std::stoul(optarg);
            } catch (...) {
                std::cerr << "Invalid name field size: " << optarg << "\n";
                return 1;
            } break;
        default:
            return 1;
        }
    }

    int remaining = argc - optind;

    if (cmd == "export") {
        if (remaining != 2) {
            std::cerr << "Usage: " << argv[0]
                      << " export <input.cxb> <output_folder>\n";
            return 1;
        }

        exportCXB(argv[optind], argv[optind + 1]);
    }
    else if (cmd == "convert") {
        if (remaining != 2) {
            std::cerr << "Usage: " << argv[0]
                      << " convert [-c|--chunk-size <KB>] <input_folder> <output.cxb>\n";
            return 1;
        }

        convertToCXB(argv[optind], argv[optind + 1], chunkSizeKB, nameFieldSize);
    }
    else {
        std::cerr << "Unknown command: " << cmd << "\n";
        return 1;
    }

    return 0;
}

