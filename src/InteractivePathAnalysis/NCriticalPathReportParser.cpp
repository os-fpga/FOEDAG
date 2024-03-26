#include "NCriticalPathReportParser.h"

#include <iostream>
#include <regex>


std::vector<GroupPtr> NCriticalPathReportParser::process(const std::vector<std::string>& lines)
{
    static std::regex pathStartPattern(R"(^\#Path (\d+)$)");
    static std::regex startPointPattern(R"(^Startpoint: (\w+(?:\[\d+\])?(\.\w+(?:\[\d+\])?)?))");
    static std::regex endPointPattern(R"(^Endpoint\s+: (\w+(?:\[\d+\])?(\.\w+(?:\[\d+\])?)?))");
    static std::regex segmentPattern(R"((^(\w+:)?(\w*[\$_]\w*)?\w+(?:\[\d+\])?(\.\w+(?:\[\d+\])?)?))");
    static std::regex slackPattern(R"(^slack\s+\(VIOLATED\)\s+(-?\d+\.\d+)$)");

    std::vector<GroupPtr> groups;
    GroupPtr currentGroup = std::make_shared<Group>();

    bool pathStarted = false;
    bool pathElementStarted = false;

    Role prevRole = Role::OTHER;
    for (const std::string& line: lines) {
        bool isMultiColumn = true;

        bool skipServiceLine = false;
        bool itemBreaker = false;

        Role currentRole = Role::OTHER;
        if (pathElementStarted) {
            currentRole = Role::SEGMENT;
        }
        bool hasMatch = false;

        if (line == "") {
            currentRole = prevRole;
            isMultiColumn = false;
            hasMatch = true;
        }
        if (line == "#End of timing report") {
            currentRole = Role::OTHER;
            groups.push_back(currentGroup);
            currentGroup = std::make_shared<Group>();
            isMultiColumn = false;
            hasMatch = true;
        }

        // handle path started
        if (!hasMatch) {
            if (std::smatch m; std::regex_search(line, m, pathStartPattern)) {
                if (m.size() > 1) {
                    pathStarted = true;
                    if (currentGroup) {
                        groups.push_back(currentGroup);
                    }
                    currentGroup = std::make_shared<Group>();
                    currentGroup->pathInfo.index = std::atoi(m[1].str().c_str());
                    currentRole = Role::PATH;
                    isMultiColumn = false;
                    hasMatch = true;
                }
            }
        }

        // handle path element
        if (pathStarted) {
            if (!hasMatch) {
                if (std::smatch m; std::regex_search(line, m, startPointPattern)) {
                    if (m.size() > 1) {
                        if (currentGroup && currentGroup->isPath()) {
                            currentGroup->pathInfo.start = m[1].str();
                            currentRole = Role::PATH;
                            hasMatch = true;
                        } else {
                            std::cerr << "bad group";
                        }
                    }
                }
            }

            if (!hasMatch) {            
                if (std::smatch m; std::regex_search(line, m, endPointPattern)) {
                    if (m.size() > 1) {
                        if (currentGroup && currentGroup->isPath()) {
                            currentGroup->pathInfo.end = m[1].str();
                            currentRole = Role::PATH;
                            hasMatch = true;
                        } else {
                            std::cerr << "bad group";
                        }
                    }
                }
            }

            if (!hasMatch) {
                if (line == "el{") {
                    currentRole = Role::SEGMENT;
                    pathElementStarted = true;
                    itemBreaker = true;

                    skipServiceLine = true;
                    hasMatch = true;
                }
                else if (line == "el}") {
                    currentRole = Role::OTHER;
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
                        currentGroup->pathInfo.slack = val;
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

        (void)(hasMatch); // suprass unused warning


        if ((currentRole != currentGroup->currentElement->currentRole()) || itemBreaker) {
            currentGroup->getNextCurrentElement();
        }
        if (!skipServiceLine) {
            currentGroup->currentElement->lines.emplace_back(Line{line, currentRole, isMultiColumn});
        }

        prevRole = currentRole;
    }

    // handle last items
    if (currentGroup) {
        groups.push_back(currentGroup);
    }

    return groups;
}


std::vector<GroupPtr> NCriticalPathReportParser::process2(const std::vector<std::string>& lines)
{
    static std::regex pathPattern(R"(^\#Path (\d+)$)");
    static std::regex startPointPattern(R"(^Startpoint: (\w+(?:\[\d+\])?(\.\w+(?:\[\d+\])?)?))");
    static std::regex endPointPattern(R"(^Endpoint\s+: (\w+(?:\[\d+\])?(\.\w+(?:\[\d+\])?)?))");
    static std::regex segmentPattern(R"((^(\w+:)?(\w*[\$_]\w*)?\w+(?:\[\d+\])?(\.\w+(?:\[\d+\])?)?))");
    static std::regex slackPattern(R"(^slack\s+\(VIOLATED\)\s+(-?\d+\.\d+)$)");
    //static std::regex pathTypePattern(R"(^Path Type : \w+$)");

    std::vector<GroupPtr> groups;
    GroupPtr currentGroup = std::make_shared<Group>();

    Role prevRole = Role::OTHER;
    for (const std::string& line: lines) {
        bool isMultiColumn = true;
        bool itemBreaker = false;

        // skip service lines
        if (line == "el{") {
            continue;
        } else if (line == "el}") {
            continue;
        }
        //

        Role currentRole = Role::OTHER;
        bool hasMatch = false;
        //std::cout << "line=[" << line << "]" << std::endl;
        if (line == "") {
            currentRole = prevRole;
            isMultiColumn = false;
            hasMatch = true;
        }

        if (!hasMatch) {
            if (line == "#End of timing report") {
                currentRole = Role::OTHER;
                groups.push_back(currentGroup);
                currentGroup = std::make_shared<Group>();
                isMultiColumn = false;
                hasMatch = true;
            }
        }

        if (!hasMatch) {
            if (std::smatch m; std::regex_search(line, m, pathPattern)) {
                if (m.size() > 1) {
                    groups.push_back(currentGroup);
                    currentGroup = std::make_shared<Group>();
                    currentGroup->pathInfo.index = std::atoi(m[1].str().c_str());
                    //std::cout << "\n\npath#" << m[1] << std::endl;
                    currentRole = Role::PATH;
                    isMultiColumn = false;
                    hasMatch = true;
                }
            }
        }

        if (!hasMatch) {
            if (std::smatch m; std::regex_search(line, m, slackPattern)) {
                if (m.size() > 1) {
                    std::string val = m[1].str();
                    currentGroup->pathInfo.slack = val;
                    //std::cout << "slack=" << m[1] << std::endl;
                    hasMatch = true;
                }
            }
        }
        
        if (!hasMatch) {
            if (std::smatch m; std::regex_search(line, m, startPointPattern)) {
                if (m.size() > 1) {
                    currentGroup->pathInfo.start = m[1].str();
                    //std::cout << "startpoint=" << m[1] << std::endl;
                    currentRole = Role::PATH;
                    hasMatch = true;
                }
            }
        }

        if (!hasMatch) {            
            if (std::smatch m; std::regex_search(line, m, endPointPattern)) {
                if (m.size() > 1) {
                    currentGroup->pathInfo.end = m[1].str();
                    //std::cout << "endpoint=" << m[1] << std::endl;
                    currentRole = Role::PATH;
                    hasMatch = true;
                }
            }
        }

//        if (!hasMatch) {
//            if (std::smatch m; std::regex_search(line, m, pathTypePattern)) {
//                currentRole = Role::PATH;
//                hasMatch = true;
//            }
//        }

        if (!hasMatch) {
            if (std::smatch m; std::regex_search(line, m, segmentPattern)) {
                if (m.size() > 1) {
                    std::string val = m[1].str();
                    if ((val.find("[") != std::string::npos) && (val.find("]") != std::string::npos)) {
                        currentRole = Role::SEGMENT;
                        std::cout << "segment=" << m[1] << std::endl;
                        itemBreaker = true;
                        hasMatch = true;
                    }
                }
            }
        }

        (void)(hasMatch); // suprass unused warning

        if ((currentRole != currentGroup->currentElement->currentRole()) || itemBreaker) {
            currentGroup->getNextCurrentElement();
        }

        if (currentGroup) {
            currentGroup->currentElement->lines.emplace_back(Line{line, currentRole, isMultiColumn});
        }

        prevRole = currentRole;
    }

    if (currentGroup) {
        groups.push_back(currentGroup);
    }

    return groups;
}
