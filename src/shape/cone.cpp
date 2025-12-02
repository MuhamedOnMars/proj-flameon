#include "cone.h"
#include <iostream>

std::vector<float> Cone::generateShape() {
    return m_vertexData;
}


void Cone::updateParams(int param1, int param2) {
    m_vertexData = std::vector<float>();
    m_param1 = std::max(param1, 1);
    m_param2 = std::max(param2, 3);
    setVertexData();
}


void Cone::setVertexData() {
    float increment = glm::radians(360.f / m_param2);

    for (int i = 0; i < m_param2; i++) {
        float theta1 = i * increment;
        float theta2 = (i + 1) * increment;
        makeWedge(theta1, theta2);
    }
}


void Cone::makeWedge(float currentTheta, float nextTheta) {
    makeCapSlice(currentTheta, nextTheta);
    makeSlopeSlice(currentTheta, nextTheta);
}


void Cone::makeSlopeSlice(float currentTheta, float nextTheta){
    float incrementY = 1.0f / float(m_param1);
    float incrementRad = 0.5f / float(m_param1);

    // Average base normals for tip normal
    glm::vec3 tip = glm::vec3(0.0f, 0.5f, 0.0f);
    glm::vec3 base1 = glm::vec3(0.5f * cos(currentTheta), 0.0f, 0.5f * sin(currentTheta));
    glm::vec3 normal1 = calcNorm(base1);
    glm::vec3 base2 = glm::vec3(0.5f * cos(nextTheta), 0.0f, 0.5f * sin(nextTheta));
    glm::vec3 normal2 = calcNorm(base2);
    glm::vec3 tipNormal = glm::normalize(normal1 + normal2);
    tipNormal = glm::normalize(glm::vec3(tipNormal.x, 0.5f, tipNormal.z));

    // Create slices top top to bottom
    for (int i = 0; i < m_param1; i++) {
        float topY = 0.5f - i * incrementY;
        float bottomY = 0.5f - (i + 1) * incrementY;
        float topRadius = i * incrementRad;
        float bottomRadius = (i + 1) * incrementRad;

        glm::vec3 topLeft = glm::vec3(topRadius * cos(currentTheta), topY, topRadius * sin(currentTheta));
        glm::vec3 topRight = glm::vec3(topRadius * cos(nextTheta), topY, topRadius * sin(nextTheta));
        glm::vec3 bottomLeft = glm::vec3(bottomRadius * cos(currentTheta), bottomY, bottomRadius * sin(currentTheta));
        glm::vec3 bottomRight = glm::vec3(bottomRadius * cos(nextTheta), bottomY, bottomRadius * sin(nextTheta));

        if (topLeft.y == tip.y || topRight.y == tip.y) {
            // Deal with base to tip slope slice
            insertVec3(m_vertexData, bottomLeft);
            insertVec3(m_vertexData, calcNorm(bottomLeft));
            insertVec3(m_vertexData, tip);
            insertVec3(m_vertexData, tipNormal);
            insertVec3(m_vertexData, bottomRight);
            insertVec3(m_vertexData, calcNorm(bottomRight));
        } else {
            makeSlopeTile(topLeft, topRight, bottomLeft, bottomRight);
        }
    }
}


void Cone::makeSlopeTile(glm::vec3 topLeft, glm::vec3 topRight, glm::vec3 bottomLeft, glm::vec3 bottomRight) {
    glm::vec3 normTL = calcNorm(topLeft);
    glm::vec3 normTR = calcNorm(topRight);
    glm::vec3 normBL = calcNorm(bottomLeft);
    glm::vec3 normBR = calcNorm(bottomRight);

    insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, normTL);
    insertVec3(m_vertexData, topRight);
    insertVec3(m_vertexData, normTR);
    insertVec3(m_vertexData, bottomLeft);
    insertVec3(m_vertexData, normBL);

    insertVec3(m_vertexData, topRight);
    insertVec3(m_vertexData, normTR);
    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, normBR);
    insertVec3(m_vertexData, bottomLeft);
    insertVec3(m_vertexData, normBL);
}


glm::vec3 Cone::calcNorm(glm::vec3& pt) {
    float xNorm = (2.0f * pt.x);
    float yNorm = -(1.0f/4.0f) * (2.0f * pt.y - 1.0f);
    float zNorm = (2.0f * pt.z);

    return glm::normalize(glm::vec3{ xNorm, yNorm, zNorm });
}


void Cone::makeCapSlice(float currentTheta, float nextTheta) {
    float increment = 0.5f / float(m_param1);

    for (int i = 0; i < m_param1; i++) {
        float val1 = i * increment;
        float val2 = (i + 1) * increment;

        glm::vec3 topLeft = glm::vec3(val1 * cos(currentTheta), -0.5f, val1 * sin(currentTheta));
        glm::vec3 topRight = glm::vec3(val1 * cos(nextTheta), -0.5f, val1 * sin(nextTheta));
        glm::vec3 bottomLeft = glm::vec3(val2 * cos(currentTheta), -0.5f, val2 * sin(currentTheta));
        glm::vec3 bottomRight = glm::vec3(val2 * cos(nextTheta), -0.5f, val2 * sin(nextTheta));

        makeCapTile(topLeft, topRight, bottomLeft, bottomRight);
    }
}


void Cone::makeCapTile(glm::vec3 topLeft, glm::vec3 topRight, glm::vec3 bottomLeft, glm::vec3 bottomRight) {
    glm::vec3 normal = glm::vec3(0.0f, -1.0f, 0.0f);

    insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, normal);
    insertVec3(m_vertexData, bottomLeft);
    insertVec3(m_vertexData, normal);
    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, normal);

    insertVec3(m_vertexData, topLeft);
    insertVec3(m_vertexData, normal);
    insertVec3(m_vertexData, bottomRight);
    insertVec3(m_vertexData, normal);
    insertVec3(m_vertexData, topRight);
    insertVec3(m_vertexData, normal);
}


void Cone::insertVec3(std::vector<float> &data, glm::vec3 v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
}

