#include "NCriticalPathReportParser.h"

#include <iostream>
#include <regex>

std::vector<GroupPtr> NCriticalPathReportParser::parseReport(const std::vector<std::string>& lines)
{
    static std::regex pathPattern(R"(^\#Path (\d+)$)");
    static std::regex startPointPattern(R"(^Startpoint: (\w+(?:\[\d+\])?(\.\w+(?:\[\d+\])?)?))");
    static std::regex endPointPattern(R"(^Endpoint\s+: (\w+(?:\[\d+\])?(\.\w+(?:\[\d+\])?)?))");
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
            if ((line.find('[') != std::string::npos) && (line.find(']') != std::string::npos)) { 
                //std::cout << "segment=" << m[1] << std::endl;

                currentRole = Role::SEGMENT;

                isEndPathElement = true;
                isInsideSegment = false;
                hasMatch = true;
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