#include "pathrpthelper.h"

#include <iostream>
#include <fstream>
#include <regex>

std::vector<std::string> readTextFile(const std::string& filename) 
{
    std::vector<std::string> lines;
    std::ifstream file(filename);

    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            lines.push_back(line);
        }
        file.close();
    } else {
        std::cerr << "Unable to open file: " << filename << std::endl;
    }

    return lines;
}

std::vector<BlockPtr> parsePathData(const std::vector<std::string>& lines)
{
    static std::regex pathPattern(R"(^\#Path (\d+)$)");
    static std::regex startPointPattern(R"(^Startpoint: (\w+(?:\[\d+\])?(\.\w+(?:\[\d+\])?)?))");
    static std::regex endPointPattern(R"(^Endpoint\s+: (\w+(?:\[\d+\])?(\.\w+(?:\[\d+\])?)?))");
    static std::regex segmentPattern(R"((^(\w+:)?(\w*[\$_]\w*)?\w+(?:\[\d+\])?(\.\w+(?:\[\d+\])?)?))");
    static std::regex slackPattern(R"(^slack\s+\(VIOLATED\)\s+(-?\d+\.\d+)$)");
    //static std::regex pathTypePattern(R"(^Path Type : \w+$)");

    std::vector<BlockPtr> blocks;
    BlockPtr block = std::make_shared<Block>();

    Role prevRole = Role::OTHER;
    for (const std::string& line: lines) {
        Role role = Role::OTHER;
        bool hasMatch = false;
        //std::cout << "line=[" << line << "]" << std::endl;
        if (line == "") {
            role = prevRole;
            hasMatch = true;
        }
        if (line == "#End of timing report") {
            role = Role::OTHER;
            blocks.push_back(block);
            block = std::make_shared<Block>();
            hasMatch = true;
        }

        if (!hasMatch) {
            std::smatch m;
            if (std::regex_search(line, m, pathPattern)) {
                if (m.size() > 1) {
                    blocks.push_back(block);
                    block = std::make_shared<Block>();
                    block->pathInfo.index = std::atoi(m[1].str().c_str());
                    //std::cout << "\n\npath#" << m[1] << std::endl;
                    role = Role::PATH;
                    hasMatch = true;
                }
            }
        }

        if (!hasMatch) {
            std::smatch m;
            if (std::regex_search(line, m, slackPattern)) {
                if (m.size() > 1) {
                    std::string val = m[1].str();
                    block->pathInfo.slack = val;
                    //std::cout << "slack=" << m[1] << std::endl;
                    hasMatch = true;
                }
            }
        }
        
        if (!hasMatch) {
            std::smatch m;
            if (std::regex_search(line, m, startPointPattern)) {
                if (m.size() > 1) {
                    block->pathInfo.start = m[1].str();
                    //std::cout << "startpoint=" << m[1] << std::endl;
                    role = Role::PATH;
                    hasMatch = true;
                }
            }
        }

        if (!hasMatch) {
            std::smatch m;
            if (std::regex_search(line, m, endPointPattern)) {
                if (m.size() > 1) {
                    block->pathInfo.end = m[1].str();
                    //std::cout << "endpoint=" << m[1] << std::endl;
                    role = Role::PATH;
                    hasMatch = true;
                }
            }
        }

//        if (!hasMatch) {
//            std::smatch m;
//            if (std::regex_search(line, m, pathTypePattern)) {
//                role = Role::PATH;
//                hasMatch = true;
//            }
//        }

        if (!hasMatch) {
            std::smatch m;
            if (std::regex_search(line, m, segmentPattern)) {
                if (m.size() > 1) {
                    std::string val = m[1].str();
                    if ((val.find("[") != std::string::npos) && (val.find("]") != std::string::npos)) {
                        role = Role::SEGMENT;
                        //std::cout << "segment=" << m[1] << std::endl;
                        hasMatch = true;
                    }
                }
            }
        }

        prevRole = role;
        block->lines.push_back(Line{line, role});
    }

    if (block) {
        blocks.push_back(block);
    }

    return blocks;
}
