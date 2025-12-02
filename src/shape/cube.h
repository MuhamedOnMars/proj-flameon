#ifndef CUBE_H
#define CUBE_H

#include "shape.h"

class Cube : public Shape {
public:
    ~Cube() override = default;
    std::vector<float> generateShape() override;
    void updateParams(int param1, int param2) override;
    void setVertexData() override;

private:
    std::vector<float> m_vertexData;
    int m_param1;

    void makeTile(glm::vec3 topLeft, glm::vec3 topRight, glm::vec3 bottomLeft, glm::vec3 bottomRight);
    void makeFace(glm::vec3 topLeft, glm::vec3 topRight, glm::vec3 bottomLeft, glm::vec3 bottomRight);
    glm::vec3 calcNorm(glm::vec3& pt1, glm::vec3& pt2, glm::vec3& pt3);
    void insertVec3(std::vector<float> &data, glm::vec3 v);
};

#endif // CUBE_H
