#pragma once

#ifndef LSYSTEM_H
#define LSYSTEM_H

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <stdio.h>
#include <string>
#include <unordered_map>
#include <random>

class Rule{
public:
    std::string getRule(std::mt19937 &rng);
    void insertRule(std::string rule);
private:
    std::vector<std::string> ruleList;
};

struct Turtle{
    glm::vec4 pos;
    float angle;
    glm::mat4 ctm;
};


class Lsystem
{
public:
    Lsystem();

    std::string generate(std::string word);
    void interpretStr(std::string word);
    void insertRule(char c, std::string rule);

private:
    std::mt19937 m_rng;
    std::unordered_map<char, std::string> ruleMap;
    std::unordered_map<char, Rule> ruleObjMap;
};

#endif // LSYSTEM_H
