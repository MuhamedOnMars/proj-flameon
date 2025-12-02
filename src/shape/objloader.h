#ifndef OBJLOADER_H
#define OBJLOADER_H

#include "shape.h"

#include <vector>
#include <glm/glm.hpp>
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>

class ObjLoader : public Shape
{
public:
    ~ObjLoader() override = default;
    std::vector<float> generateShape() override;
    void updateParams(int param1, int param2) override;
    void setVertexData() override;

    ObjLoader();
    ObjLoader(std::string mesh_file);
    void makeTile(glm::vec3 pt1, glm::vec3 pt2, glm::vec3 pt3);
    glm::vec3 calcNorm(glm::vec3& pt1, glm::vec3& pt2, glm::vec3& pt3);
    void insertVec3(std::vector<float> &data, glm::vec3 v);

private:
    std::vector<float> m_vertexData;
    std::vector<float> vertices;
    std::vector<int> faces;
};

#endif // OBJLOADER_H
