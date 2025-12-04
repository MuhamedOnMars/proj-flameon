#include "lsystem.h"

#include "settings.h"
#include <sstream>
#include "glm/gtc/constants.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"

Lsystem::Lsystem() {
    std::random_device rd;
    m_rng = std::mt19937(rd());
}

//note: use stochastic grammar
//note: add parameter to randomize angle and length6

std::string Lsystem::generate(std::string word){
    std::string newWord = "";

    for(int i = 0; i < word.length(); i++){
        char c = word[i];
        if(ruleMap.contains(c)){
            newWord += ruleMap[c];
        }else{
            newWord += c;
        }
    }

    return newWord;
}

//remove whitespace in rule
static inline void trimInPlace(std::string &s) {
    const char *ws = " \t\n\r";
    auto start = s.find_first_not_of(ws);
    if (start == std::string::npos) {
        s.clear();
        return;
    }
    auto end = s.find_last_not_of(ws);
    s = s.substr(start, end - start + 1);
}

std::vector<std::pair<char, std::string>>
parseLSystemRules(const std::string &text)
{
    std::vector<std::pair<char, std::string>> result;

    std::stringstream ss(text);
    std::string ruleStr;

    // Split by ';' into individual rules
    while (std::getline(ss, ruleStr, ';')) {
        trimInPlace(ruleStr);
        if (ruleStr.empty()) continue;

        std::size_t arrowPos = ruleStr.find("->");
        if (arrowPos == std::string::npos) {
            continue;
        }

        std::string lhs = ruleStr.substr(0, arrowPos);
        std::string rhs = ruleStr.substr(arrowPos + 2); // skip "->"

        trimInPlace(lhs);
        trimInPlace(rhs);

        if (lhs.empty() || rhs.empty()) {
            continue;
        }

        // Take first character of LHS as the symbol
        char symbol = lhs[0];

        result.emplace_back(symbol, rhs);
    }

    return result;
}

void Rule::insertRule(std::string rule){
    ruleList.push_back(rule);
}

std::string Rule::getRule(std::mt19937 &rng){
    if(ruleList.empty()){
        return "";
    }

    std::uniform_int_distribution<std::size_t> dist(0, ruleList.size() - 1);
    return ruleList[dist(rng)];

}

void Lsystem::insertRule(char c, std::string rule){
    ruleMap[c] = rule;
    ruleObjMap[c].insertRule(rule);
}



