#include "cube.h"
#include <iostream>

std::vector<float> Cube::generateShape() {
    return m_vertexData;
}


void Cube::updateParams(int param1, int param2) {
    m_vertexData = std::vector<float>();
    m_param1 = param1;
    setVertexData();
}


void Cube::setVertexData() {
    // Assuming unit cube in local space
    // Front
    makeFace(glm::vec3(-0.5f,  0.5f, 0.5f),
             glm::vec3( 0.5f,  0.5f, 0.5f),
             glm::vec3(-0.5f, -0.5f, 0.5f),
             glm::vec3( 0.5f, -0.5f, 0.5f));
    // Back
    makeFace(glm::vec3( 0.5f,  0.5f, -0.5f),
             glm::vec3(-0.5f,  0.5f, -0.5f),
             glm::vec3( 0.5f, -0.5f, -0.5f),
             glm::vec3(-0.5f, -0.5f, -0.5f));
    // Left
    makeFace(glm::vec3(-0.5f,  0.5f, -0.5f),
             glm::vec3(-0.5f,  0.5f,  0.5f),
             glm::vec3(-0.5f, -0.5f, -0.5f),
             glm::vec3(-0.5f, -0.5f,  0.5f));
    //Right
    makeFace(glm::vec3(0.5f,  0.5f,  0.5f),
             glm::vec3(0.5f,  0.5f, -0.5f),
             glm::vec3(0.5f, -0.5f,  0.5f),
             glm::vec3(0.5f, -0.5f, -0.5f));
    //Top
    makeFace(glm::vec3(-0.5f,  0.5f, -0.5f),
             glm::vec3( 0.5f,  0.5f, -0.5f),
             glm::vec3(-0.5f,  0.5f,  0.5f),
             glm::vec3( 0.5f,  0.5f,  0.5f));
    // Bottom
    makeFace(glm::vec3(-0.5f, -0.5f,  0.5f),
             glm::vec3( 0.5f, -0.5f,  0.5f),
             glm::vec3(-0.5f, -0.5f, -0.5f),
             glm::vec3( 0.5f, -0.5f, -0.5f));
}


void Cube::makeFace(glm::vec3 topLeft, glm::vec3 topRight, glm::vec3 bottomLeft, glm::vec3 bottomRight) {
    glm::vec3 colIncrement = (topRight - topLeft) / float(m_param1);
    glm::vec3 rowIncrement  = (bottomLeft - topLeft) / float(m_param1);

    for (int row = 0; row < m_param1; row++) {
        for (int col = 0; col < m_param1; col++) {
            glm::vec3 newTopLeft = topLeft + (colIncrement * float(col) + rowIncrement * float(row));
            glm::vec3 newTopRight = topLeft + (colIncrement * float(col + 1) + rowIncrement * float(row));
            glm::vec3 newBottomLeft = topLeft + (colIncrement * float(col) + rowIncrement * float(row + 1));
            glm::vec3 newBottomRight = topLeft + (colIncrement * float(col + 1) + rowIncrement * float(row + 1));

            makeTile(newTopLeft, newTopRight, newBottomLeft, newBottomRight);
        }
    }
}


void Cube::makeTile(glm::vec3 topLeft, glm::vec3 topRight, glm::vec3 bottomLeft, glm::vec3 bottomRight) {
    insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, calcNorm(topLeft, bottomLeft, bottomRight));
    insertVec3(m_vertexData, bottomLeft);
    insertVec3(m_vertexData, calcNorm(bottomLeft, bottomRight, topLeft));
    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, calcNorm(bottomRight, topLeft, bottomLeft));

    insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, calcNorm(topLeft, bottomRight, topRight));
    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, calcNorm(bottomRight, topRight, topLeft));
    insertVec3(m_vertexData, topRight);
    insertVec3(m_vertexData, calcNorm(topRight, topLeft, bottomRight));
}


glm::vec3 Cube::calcNorm(glm::vec3& pt1, glm::vec3& pt2, glm::vec3& pt3) {
    return glm::normalize(glm::cross(pt1 - pt2, pt1 - pt3));
}


void Cube::insertVec3(std::vector<float> &data, glm::vec3 v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}

