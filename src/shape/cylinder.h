#ifndef CYLINDER_H
#define CYLINDER_H

#include "shape.h"

class Cylinder : public Shape {
public:
    ~Cylinder() override = default;
    std::vector<float> generateShape() override;
    void updateParams(int param1, int param2) override;
    void setVertexData() override;

private:
    std::vector<float> m_vertexData;
    int m_param1;
    int m_param2;

    void makeWedge(float currentTheta, float nextTheta);
    void makeSlopeSlice(float currentTheta, float nextTheta);
    void makeSlopeTile(glm::vec3 topLeft, glm::vec3 topRight, glm::vec3 bottomLeft, glm::vec3 bottomRight);
    glm::vec3 calcNorm(glm::vec3& pt);
    void makeCapSlice(float currentTheta, float nextTheta, float y, glm::vec3 normal);
    void makeCapTile(glm::vec3 topLeft, glm::vec3 topRight, glm::vec3 bottomLeft, glm::vec3 bottomRight, glm::vec3 normal);
    void insertVec3(std::vector<float> &data, glm::vec3 v);
};

#endif // CYLINDER_H
