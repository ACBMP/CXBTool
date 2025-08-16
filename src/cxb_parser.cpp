#include "cxb_parser.hpp"
#include <fstream>
#include <sstream>
#include <regex>
#include <vector>
#include <cstring>
#include <iostream>
#include <zlib.h>
#include <lzo/lzo2a.h>
#include <cstdint>
#include "tinyxml2.h"


constexpr uint64_t MAGIC = 0x1004FA9957FBAA33;
constexpr uint16_t VERSION = 1;
constexpr uint8_t ALGO = 2;
constexpr uint16_t DECOMP_BUFFER_SIZE = 32768;
constexpr uint16_t COMP_BUFFER_SIZE = 0;

std::map<std::string, std::string> ExtractCXB(const std::string& path) {
    std::cout << "Loading XML";
    std::map<std::string, std::string> extractedXMLs;
    if (lzo_init() != LZO_E_OK) {
        std::cerr << "LZO initialization failed." << std::endl;
        return extractedXMLs;
    }

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open CXB file: " << path << std::endl;
        return extractedXMLs;
    }

    std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    const std::vector<uint8_t> magic_bytes(reinterpret_cast<const uint8_t*>(&MAGIC),
                                           reinterpret_cast<const uint8_t*>(&MAGIC) + sizeof(MAGIC));

    std::vector<size_t> indices;
    for (size_t i = 0; i + magic_bytes.size() <= data.size(); ++i) {
        if (std::memcmp(&data[i], magic_bytes.data(), magic_bytes.size()) == 0) {
            indices.push_back(i);
        }
    }

    if (indices.empty()) {
        std::cerr << "No magic number found in CXB file." << std::endl;
        return extractedXMLs;
    }


    // Extract segment names and sizes from pre-magic data
    std::string decoded;
    for (size_t i = 0; i < indices[0]; ++i) {
        char c = static_cast<char>(data[i]);
        decoded += (isprint(c) ? c : ' ');
    }

    std::regex token_regex("\\S+");
    std::sregex_iterator iter(decoded.begin(), decoded.end(), token_regex);
    std::vector<std::string> tokens;
    while (iter != std::sregex_iterator()) {
        tokens.push_back(iter->str());
        ++iter;
    }

    std::vector<std::pair<std::string, int>> segments;
    for (size_t i = 0; i + 1 < tokens.size() - 1; i += 2) {
        try {
            segments.emplace_back(tokens[i], std::stoi(tokens[i + 1]));
        } catch (const std::exception& e) {
            std::cerr << "Invalid segment size for token: " << tokens[i] << " — " << e.what() << std::endl;
        }

    }

    for (size_t i = 0; i < indices.size(); ++i) {
        size_t index = indices[i];
        size_t start = index - 4;
        size_t end = start + segments[i].second;
        std::vector<uint8_t> s(data.begin() + start, data.begin() + end);

        size_t offset = 0;
        auto read = [&](size_t n) -> std::vector<uint8_t> {
            if (offset + n > s.size()) {
                std::cerr << "Read out of bounds in segment " << segments[i].first << std::endl;
                return {};
            }
            std::vector<uint8_t> out(s.begin() + offset, s.begin() + offset + n);
            offset += n;
            return out;
        };

        auto readInt = [&](size_t n) -> uint64_t {
            auto bytes = read(n);
            if (bytes.size() != n) return 0;
            uint64_t val = 0;
            for (size_t j = 0; j < n; ++j) {
                val |= static_cast<uint64_t>(bytes[j]) << (8 * j);
            }
            return val;
        };

        // none of these need to be used
        uint32_t xmlSize = readInt(4);
        uint64_t magic = readInt(8);
        uint16_t version = readInt(2);
        uint8_t algo = readInt(1);
        uint16_t en_bufsize = readInt(2);
        uint16_t de_bufsize = readInt(2);
    
        if (xmlSize <= 0 || magic != MAGIC || version != VERSION || algo != ALGO || en_bufsize != DECOMP_BUFFER_SIZE || de_bufsize != COMP_BUFFER_SIZE)
            std::cerr << "Invalid header data" << std::endl;

        std::vector<uint8_t> xmlData;
        while (offset < s.size()) {
            uint8_t isCompressed = s[offset++];
            if (isCompressed) {
                uint32_t en_size = readInt(4);
                uint32_t de_size = readInt(4);
                uint32_t checksum = readInt(4);
                std::vector<uint8_t> comp_data = read(en_size);

                if (adler32(0, comp_data.data(), en_size) != checksum) {
                    std::cerr << "Checksum mismatch in segment " << segments[i].first << std::endl;
                }

                std::vector<uint8_t> decompressed(de_size);
                lzo_uint out_len = de_size;
                int result = lzo2a_decompress_safe(comp_data.data(), en_size,
                                                   decompressed.data(), &out_len, nullptr);
                if (result != LZO_E_OK) {
                    std::cerr << "LZO decompression failed for segment " << segments[i].first << std::endl;
                    continue;
                }

                xmlData.insert(xmlData.end(), decompressed.begin(), decompressed.begin() + out_len);
            } else {
                uint32_t data_size = readInt(4);
                std::vector<uint8_t> raw = read(data_size);
                xmlData.insert(xmlData.end(), raw.begin(), raw.end());
            }
        }

        std::string xml(xmlData.begin(), xmlData.end());
        if (!xml.empty() && xml.back() == '\0') xml.pop_back();

        extractedXMLs[segments[i].first + ".xml"] = xml;
    }

    return extractedXMLs;
}

std::vector<uint8_t> CompressLZO(const std::vector<uint8_t>& input) {
    std::vector<uint8_t> output(input.size() + input.size() / 16 + 64 + 3);
    std::vector<uint8_t> wrkmem(LZO2A_999_MEM_COMPRESS);

    lzo_uint out_len = output.size();
    int r = lzo2a_999_compress(input.data(), input.size(), output.data(), &out_len, wrkmem.data());

    if (r != LZO_E_OK) throw std::runtime_error("LZO compression failed");

    output.resize(out_len);
    return output;
}

std::vector<uint8_t> ConvertToCXB(const std::vector<CXBFile>& files) {
    if (lzo_init() != LZO_E_OK) throw std::runtime_error("LZO init failed");

    std::vector<uint8_t> fileInfos;
    std::vector<uint8_t> fileData;

    for (const auto& file : files) {
        // Serialize XML
        tinyxml2::XMLPrinter printer;
        file.doc->Print(&printer);
        std::string xmlStr = printer.CStr();
        xmlStr.push_back('\0');

        std::vector<uint8_t> xmlBytes(xmlStr.begin(), xmlStr.end());
        uint32_t uncompressedSize = xmlBytes.size();

        // Chunk and compress
        std::vector<uint8_t> blocks;
        for (size_t i = 0; i < xmlBytes.size(); i += DECOMP_BUFFER_SIZE) {
            std::vector<uint8_t> chunk(xmlBytes.begin() + i, xmlBytes.begin() + std::min(i + DECOMP_BUFFER_SIZE, xmlBytes.size()));
            std::vector<uint8_t> comp = CompressLZO(chunk);

            uint32_t de_size = chunk.size();
            uint32_t en_size = comp.size();
            uint32_t checksum = lzo_adler32(0, comp.data(), comp.size());

            blocks.push_back(1);  // is_compressed
            for (uint32_t val : {en_size, de_size, checksum}) {
                for (int j = 0; j < 4; ++j) blocks.push_back((val >> (8 * j)) & 0xFF);
            }
            blocks.insert(blocks.end(), comp.begin(), comp.end());
        }

        // Build file header
        std::vector<uint8_t> header;
        for (int i = 0; i < 4; ++i) header.push_back((uncompressedSize >> (8 * i)) & 0xFF);
        for (int i = 0; i < 8; ++i) header.push_back((MAGIC >> (8 * i)) & 0xFF);
        header.push_back(VERSION & 0xFF);
        header.push_back((ALGO >> 8) & 0xFF);
        header.push_back(ALGO & 0xFF);
        header.push_back(DECOMP_BUFFER_SIZE & 0xFF);
        header.push_back((DECOMP_BUFFER_SIZE >> 8) & 0xFF);
        header.push_back(COMP_BUFFER_SIZE & 0xFF);
        header.push_back((COMP_BUFFER_SIZE >> 8) & 0xFF);

        header.insert(header.end(), blocks.begin(), blocks.end());
        fileData.insert(fileData.end(), header.begin(), header.end());

        // Build file info
        std::string baseName = file.name.substr(0, file.name.find('.'));
        std::vector<uint8_t> nameBuf(baseName.begin(), baseName.end());
        nameBuf.resize(32, 0);

        std::ostringstream sizeStr;
        sizeStr << header.size();
        std::string sizeBuf = sizeStr.str();
        std::vector<uint8_t> sizeBytes(sizeBuf.begin(), sizeBuf.end());
        sizeBytes.resize(8, 0);

        fileInfos.insert(fileInfos.end(), nameBuf.begin(), nameBuf.end());
        fileInfos.insert(fileInfos.end(), sizeBytes.begin(), sizeBytes.end());
    }

    // Final padding
    fileInfos.push_back(0x30);
    fileInfos.resize(fileInfos.size() + 39, 0);

    // Combine all
    std::vector<uint8_t> finalBuffer;
    finalBuffer.insert(finalBuffer.end(), fileInfos.begin(), fileInfos.end());
    finalBuffer.insert(finalBuffer.end(), fileData.begin(), fileData.end());

    return finalBuffer;
}

