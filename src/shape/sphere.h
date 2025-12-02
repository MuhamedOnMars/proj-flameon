#ifndef SPHERE_H
#define SPHERE_H

#include "shape.h"

class Sphere : public Shape {
public:
    ~Sphere() override = default;
    std::vector<float> generateShape() override;
    void updateParams(int param1, int param2) override;
    void setVertexData() override;

private:
    std::vector<float> m_vertexData;
    int m_param1;
    int m_param2;

    void makeSphere();
    void makeWedge(float currTheta, float nextTheta);
    void makeTile(glm::vec3 topLeft, glm::vec3 topRight, glm::vec3 bottomLeft, glm::vec3 bottomRight);
    void insertVec3(std::vector<float> &data, glm::vec3 v);
};

#endif // SPHERE_H
