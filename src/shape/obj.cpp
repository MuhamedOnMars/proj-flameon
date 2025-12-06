#include "obj.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <glm/glm.hpp>

std::vector<float> Obj::generateShape() {
    return m_vertexData;
}

void Obj::updateParams(int param1, int param2) {
    // m_vertexData = std::vector<float>();
    // m_param1 = std::max(param1, 2);
    // m_param2 = std::max(param2, 3);
    // setVertexData();
}

void Obj::setVertexData() {
    //makeSphere();
}

void Obj::readOBJ(const std::string &path){
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Cannot open OBJ " + path);
    }

    std::vector<float> objData;

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        std::string type;
        ss >> type;

        if (type == "v") {
            glm::vec3 p;
            ss >> p.x >> p.y >> p.z;
            positions.push_back(p);
        }
        else if (type == "vn") {
            glm::vec3 n;
            ss >> n.x >> n.y >> n.z;
            normals.push_back(glm::normalize(n));
        }
        else if (type == "f") {
            for (int i = 0; i < 3; ++i) {
                std::string vtn;
                ss >> vtn;

                int vIdx = 0, tIdx = 0, nIdx = 0;
                sscanf(vtn.c_str(), "%d/%d/%d", &vIdx, &tIdx, &nIdx);

                glm::vec3 p = positions[vIdx - 1];
                glm::vec3 n = normals[nIdx - 1];

                objData.push_back(p.x);
                objData.push_back(p.y);
                objData.push_back(p.z);
                objData.push_back(n.x);
                objData.push_back(n.y);
                objData.push_back(n.z);
            }
        }
    }

    m_vertexData = objData;
}
