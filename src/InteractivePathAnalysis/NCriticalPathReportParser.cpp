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
        bool skipServiceLine = false;
        bool itemBreaker = false;

        Role currentRole = Role::OTHER;
        if (pathElementStarted) {
            currentRole = Role::SEGMENT;
        }
        bool hasMatch = false;

        if (line == "") {
            currentRole = prevRole;
            hasMatch = true;
        }
        if (line == "#End of timing report") {
            currentRole = Role::OTHER;
            groups.push_back(currentGroup);
            currentGroup = std::make_shared<Group>();
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
            currentGroup->currentElement->lines.emplace_back(Line{line, currentRole});
        }

        prevRole = currentRole;
    }

    // handle last items
    if (currentGroup) {
        groups.push_back(currentGroup);
    }

    return groups;
}
