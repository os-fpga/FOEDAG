#pragma once

#include <string>
#include <vector>
#include <memory>

enum Role {
    PATH,
    SEGMENT,
    OTHER
};

struct Line {
    std::string line;
    Role role;
};

struct PathInfo {
    int index = -1;
    std::string start;
    std::string end;
    std::string slack;
    std::string id() const { return start + ":" + end;}
    bool isValid() const { return (index != -1); }
};
struct Block {
    std::vector<Line> lines;
    bool isPath() { return pathInfo.isValid(); }

    PathInfo pathInfo;
};
using BlockPtr = std::shared_ptr<Block>;

#ifdef ENABLE_OPEN_FILE_FEATURE
std::vector<std::string> readTextFile(const std::string& filename);
#endif

std::vector<BlockPtr> parsePathData(const std::vector<std::string>& lines);

