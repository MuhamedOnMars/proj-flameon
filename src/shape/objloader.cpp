#include "objloader.h"

ObjLoader::ObjLoader() {}

ObjLoader::ObjLoader(std::string mesh_file) {
    std::ifstream input_file(mesh_file);
    if (!input_file.is_open()) {
        std::cerr << "Error opening file!" << std::endl;
        return;
    }

    std::string line;
    while (std::getline(input_file, line)) {
        // Need first word of line to see if its a vertex or face val
        std::stringstream ss(line);
        std::string word;
        ss >> word;
        if (word == "v") {
            while (ss >> word) {
                float vert_val = std::stof(word);
                vertices.push_back(vert_val);
            }
        } else if (word == "f") {
            while (ss >> word) {
                int face_val = std::stoi(word);
                faces.push_back(face_val);
            }
        }
    }
}


std::vector<float> ObjLoader::generateShape() {
    return m_vertexData;
}


void ObjLoader::updateParams(int param1, int param2) {
    m_vertexData = std::vector<float>();
    setVertexData();
}


void ObjLoader::setVertexData() {
    // Three points make up 1 vertex --> need the 3 vertices for normal calc
    for (int i = 0; i < faces.size(); i = i + 3) {
        // Faces indices start at 1
        int val1 = faces[i] - 1;
        glm::vec3 pt1 = glm::vec3(vertices[3*val1], vertices[3*val1 + 1], vertices[3*val1 + 2]);
        int val2 = faces[i+1] - 1;
        glm::vec3 pt2 = glm::vec3(vertices[3*val2], vertices[3*val2 + 1], vertices[3*val2 + 2]);
        int val3 = faces[i+2] - 1;
        glm::vec3 pt3 = glm::vec3(vertices[3*val3], vertices[3*val3 + 1], vertices[3*val3 + 2]);

        makeTile(pt1, pt2, pt3);
    }
}


void ObjLoader::makeTile(glm::vec3 pt1, glm::vec3 pt2, glm::vec3 pt3) {
    insertVec3(m_vertexData, pt1);
    insertVec3(m_vertexData, calcNorm(pt1, pt2, pt3));
    insertVec3(m_vertexData, pt2);
    insertVec3(m_vertexData, calcNorm(pt2, pt3, pt1));
    insertVec3(m_vertexData, pt3);
    insertVec3(m_vertexData, calcNorm(pt3, pt1, pt2));
}


glm::vec3 ObjLoader::calcNorm(glm::vec3& pt1, glm::vec3& pt2, glm::vec3& pt3) {
    return glm::normalize(glm::cross(pt1 - pt2, pt1 - pt3));
}


void ObjLoader::insertVec3(std::vector<float> &data, glm::vec3 v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}

