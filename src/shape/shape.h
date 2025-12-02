#ifndef SHAPE_H
#define SHAPE_H

#pragma once

#include <glm/glm.hpp>
#include <vector>

class Shape {

public:
    virtual ~Shape() = default;
    virtual std::vector<float> generateShape() = 0;
    virtual void updateParams(int param1, int param2) = 0;
    virtual void setVertexData() = 0;
};

#endif // SHAPE_H
