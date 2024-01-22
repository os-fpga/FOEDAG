#include "NCriticalPathReportParser.h"

#include <iostream>
#include <fstream>
#include <regex>


std::vector<BlockPtr> NCriticalPathReportParser::process(const std::vector<std::string>& lines)
{
    static std::regex pathStartPattern(R"(^\#Path (\d+)$)");
    static std::regex startPointPattern(R"(^Startpoint: (\w+(?:\[\d+\])?(\.\w+(?:\[\d+\])?)?))");
    static std::regex endPointPattern(R"(^Endpoint\s+: (\w+(?:\[\d+\])?(\.\w+(?:\[\d+\])?)?))");
    static std::regex segmentPattern(R"((^(\w+:)?(\w*[\$_]\w*)?\w+(?:\[\d+\])?(\.\w+(?:\[\d+\])?)?))");
    static std::regex slackPattern(R"(^slack\s+\(VIOLATED\)\s+(-?\d+\.\d+)$)");

    std::vector<BlockPtr> blocks;
    BlockPtr block = std::make_shared<Block>();

    int currentPathNum = 0;
    int currentPathElementNum = 0;
    int currentOtherElementNum = 0;

    bool pathStarted = false;
    bool pathElementStarted = false;

    Role prevRole = Role::OTHER;
    for (const std::string& line: lines) {
        bool skipServiceLine = false;

        Role role = Role::OTHER;
        bool hasMatch = false;
        //std::cout << "line=[" << line << "]" << std::endl;

        // handle path started
        if (!hasMatch) {
            if (std::smatch m; std::regex_search(line, m, pathStartPattern)) {
                if (m.size() > 1) {
                    blocks.push_back(block);
                    block = std::make_shared<Block>();
                    block->pathInfo.index = std::atoi(m[1].str().c_str());
                    //std::cout << "\n\npath#" << m[1] << std::endl;
                    role = Role::PATH;
                    currentPathNum++;
                    hasMatch = true;
                }
            }
        }

        // handle path element
        if (pathStarted) {
            if (!hasMatch) {
                if (std::smatch m; std::regex_search(line, m, startPointPattern)) {
                    if (m.size() > 1) {
                        block->pathInfo.start = m[1].str();
                        //std::cout << "startpoint=" << m[1] << std::endl;
                        role = Role::PATH;
                        hasMatch = true;
                    }
                }
            }

            if (!hasMatch) {            
                if (std::smatch m; std::regex_search(line, m, endPointPattern)) {
                    if (m.size() > 1) {
                        block->pathInfo.end = m[1].str();
                        //std::cout << "endpoint=" << m[1] << std::endl;
                        role = Role::PATH;
                        hasMatch = true;
                    }
                }
            }

            if (!hasMatch) {
                if (line == "el{") {
                    role = Role::SEGMENT;
                    pathElementStarted = true;
                    skipServiceLine = true;
                    currentPathElementNum++;
                    hasMatch = true;
                }
                else if (line == "el}") {
                    role = Role::OTHER;
                    pathElementStarted = false;
                    skipServiceLine = true;
                    hasMatch = true;
                }
            }

            // end of path
            if (!hasMatch) {
                if (std::smatch m; std::regex_search(line, m, slackPattern)) {
                    if (m.size() > 1) {
                        std::string val = m[1].str();
                        block->pathInfo.slack = val;
                        //std::cout << "slack=" << m[1] << std::endl;
                        hasMatch = true;
                    }
                }
            }

            if (!hasMatch) {
                if (pathElementStarted) {
                    hasMatch = true;
                }
            }
        }

        // if (!hasMatch) {
        //     if (std::smatch m; std::regex_search(line, m, segmentPattern)) {
        //         if (m.size() > 1) {
        //             std::string val = m[1].str();
        //             if ((val.find("[") != std::string::npos) && (val.find("]") != std::string::npos)) {
        //                 role = Role::SEGMENT;
        //                 //std::cout << "segment=" << m[1] << std::endl;
        //                 hasMatch = true;
        //             }
        //         }
        //     }
        // }

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
        
        (void)(hasMatch); // suprass unused warning

        int itemIndex = 0;
        if (role == PATH) {
            itemIndex = currentPathNum;
        } else if (role == SEGMENT) {
            itemIndex = currentPathElementNum;
        } else {
            itemIndex = currentOtherElementNum;
        }

        if (!skipServiceLine) {
            block->elements[itemIndex].emplace_back(Line{line, role});
        }
        prevRole = role;
    }

    if (block) {
        blocks.push_back(block);
    }

    return blocks;
}
