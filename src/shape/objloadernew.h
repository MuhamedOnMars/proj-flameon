#ifndef OBJLOADERNEW_H
#define OBJLOADERNEW_H
#include <utility>

class Objloadernew
{
public:
    Objloadernew();

    static std::vector<float> readOBJ(const std::string &path);

    static std::vector<float> readLeaf();
    static std::vector<float> readBranch();

    static std::vector<float> readTile(int tile);
};

#endif // OBJLOADERNEW_H
