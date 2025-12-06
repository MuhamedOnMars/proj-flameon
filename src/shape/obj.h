#ifndef OBJ_H
#define OBJ_H

#include "shape.h"

class Obj : public Shape
{
public:
    ~Obj() override = default;
    std::vector<float> generateShape() override;
    void updateParams(int param1, int param2) override;
    void setVertexData() override;
    void readOBJ(const std::string &path);

private:
    std::vector<float> m_vertexData;
};

#endif // OBJ_H
