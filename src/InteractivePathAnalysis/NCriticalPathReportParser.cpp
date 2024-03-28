#include "NCriticalPathReportParser.h"

#include <iostream>
#include <regex>


template<typename T>
void expect_eq(const T& left, const T& right, const std::string& label = "") {
    if (left != right) {
        std::cout << "mismatch!!! [[[" << label << "]]]" << std::endl;
        std::cout << "left= " << left << std::endl;
        std::cout << "right=" << right << std::endl;
        exit(1);
    }
}

void compareGroups(const std::vector<GroupPtr>& proofGroups, const std::vector<GroupPtr>& groups)
{
    expect_eq(proofGroups.size(), groups.size(), "group size");

    for (std::size_t gi=0; gi<proofGroups.size(); ++gi) {
        const GroupPtr& proofGroup = proofGroups.at(gi);
        const GroupPtr& group = groups.at(gi);

        expect_eq(proofGroup->isPath(), group->isPath(), "is path");

        // compare path info
        expect_eq(proofGroup->pathInfo.index, group->pathInfo.index, "path info index");
        expect_eq(proofGroup->pathInfo.start, group->pathInfo.start, "path info start");
        expect_eq(proofGroup->pathInfo.end, group->pathInfo.end, "path info end");
        expect_eq(proofGroup->pathInfo.slack, group->pathInfo.slack, "path info stack");
        expect_eq(proofGroup->pathInfo.id(), group->pathInfo.id(), "path info id");
        expect_eq(proofGroup->pathInfo.isValid(), group->pathInfo.isValid(), "path info is valid");

        // compare elements
        expect_eq(proofGroup->elements.size(), group->elements.size(), "elements size");
        for (std::size_t ei=0; ei<proofGroup->elements.size(); ++ei) {
            const ElementPtr& proofElement = proofGroup->elements.at(ei);
            const ElementPtr& element = group->elements.at(ei);

            expect_eq(proofElement->currentRole(), element->currentRole(), "current role");
            expect_eq(proofElement->lines.size(), element->lines.size(), "lines size");
            for (std::size_t li=0; li<proofElement->lines.size(); ++li) {
                const Line& proofLine = proofElement->lines.at(li);
                const Line& line = element->lines.at(li);

                std::cout << "gi(" << gi << "), ei(" << ei << "), proof line(" << li << ")=" << proofLine.line << std::endl;
                std::cout << "gi(" << gi << "), ei(" << ei << "), line      (" << li << ")=" << line.line << std::endl;

                expect_eq(proofLine.line, line.line, "line.line");
                expect_eq(proofLine.role, line.role, "line.role");
                expect_eq(proofLine.isMultiColumn, line.isMultiColumn, "line.isMultiColumn");
            }
        }
    }
}

std::vector<GroupPtr> NCriticalPathReportParser::parseReport_OLD(const std::vector<std::string>& lines)
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
    bool isEndReportReached = false;
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
            isEndReportReached = true;
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

        if (isEndReportReached) {
            break;
        }

        prevRole = currentRole;
    }

    // handle last items
    if (currentGroup) {
        groups.push_back(currentGroup);
    }

    return groups;
}

std::vector<GroupPtr> NCriticalPathReportParser::parseReport(const std::vector<std::string>& lines)
{
    static std::regex pathPattern(R"(^\#Path (\d+)$)");
    static std::regex startPointPattern(R"(^Startpoint: (\w+(?:\[\d+\])?(\.\w+(?:\[\d+\])?)?))");
    static std::regex endPointPattern(R"(^Endpoint\s+: (\w+(?:\[\d+\])?(\.\w+(?:\[\d+\])?)?))");
    // TODO: check below pattern
    static std::regex endPathElementPattern(R"((^(\w+:)?(\w*[\$_]\w*)?\w+(?:\[\d+\])?(\.\w+(?:\[\d+\])?)?))");
    static std::regex slackPattern(R"(^slack\s+\(VIOLATED\)\s+(-?\d+\.\d+)$)");
    // static std::regex pathTypePattern(R"(^Path Type : \w+$)");

    std::vector<GroupPtr> groups;
    GroupPtr currentGroup = std::make_shared<Group>();

    bool isInsideSegment = false;

    Role prevRole = Role::OTHER;
    bool isEndReportReached = false;
    for (const std::string& line: lines) {
        bool isMultiColumn = true;
        bool isEndPathElement = false;

        // TODO: remove this
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

    //    if (!hasMatch) {
    //        if (std::smatch m; std::regex_search(line, m, pathTypePattern)) {
    //            currentRole = Role::PATH;
    //            hasMatch = true;
    //        }
    //    }

        if (!hasMatch) {
            if (line.at(0) == '|') {
                if (!isInsideSegment) {
                    isInsideSegment = true;
                }
                currentRole = Role::SEGMENT;
                hasMatch = true;
            }
        }

        if (!hasMatch) {
            if (std::smatch m; std::regex_search(line, m, endPathElementPattern)) {
                currentRole = Role::OTHER;
                if (m.size() > 1) {
                    std::string val = m[1].str();
                    if ((val.find("[") != std::string::npos) && (val.find("]") != std::string::npos)) {                        
                        //std::cout << "segment=" << m[1] << std::endl;

                        currentRole = Role::SEGMENT;

                        isEndPathElement = true;
                        isInsideSegment = false;
                        hasMatch = true;
                    }
                }
            }
        }

        if (!hasMatch) {
            if (line == "#End of timing report") {
                currentRole = Role::OTHER;
                groups.push_back(currentGroup);
                currentGroup = std::make_shared<Group>();
                isMultiColumn = false;
                isEndReportReached = true;
                hasMatch = true;
            }
        }

        (void)(hasMatch); // suprass unused warning

        if (currentRole != currentGroup->currentElement->currentRole()) {
            currentGroup->getNextCurrentElement();
        }

        if (currentGroup) {
            currentGroup->currentElement->lines.emplace_back(Line{line, currentRole, isMultiColumn});
        }

        if (isEndPathElement) {
            currentGroup->getNextCurrentElement();
            isEndPathElement = false;
        }
 
        if (isEndReportReached) {
            break;
        }

        prevRole = currentRole;
    }

    if (currentGroup) {
        groups.push_back(currentGroup);
    }

    // DEBUG
    // std::vector<GroupPtr> proofGroups = process_OLD(lines);
    // compareGroups(proofGroups, groups);
    // DEBUG

    return groups;
}

void NCriticalPathReportParser::parseMetaData(const std::vector<std::string>& lines, std::map<int, std::pair<int, int>>& metadata)
{
    int pathIndex = -1;
    int offsetIndex = -1;
    int numElements = -1;
    char delim = '/';
    for (auto it = lines.rbegin(); it != lines.rend(); ++it) {
        std::istringstream iss(*it);
        if (iss >> pathIndex >> delim >> offsetIndex >> delim >> numElements && delim == '/') {
            metadata[pathIndex] = std::make_pair(offsetIndex, numElements);
        } else {
            if (*it == "#RPT METADATA:") {
                break;
            }
        }
    } 
}