#ifndef WHY_H
#define WHY_H

#include<cstdint>
#include<string>

#include<map>
#include<vector>

#include "tinyxml2.h"

using std::map;
using std::vector;
using std::string;

using tinyxml2::XMLElement;
using tinyxml2::XMLDocument;

typedef map<uint32_t, XMLElement *> TypeInfos;

void saveWhy3(XMLDocument &pog, std::ofstream &why, const std::map<int, std::vector<int>>& goals);
void saveWhy3(XMLDocument &pog, std::ofstream &why, bool xml);

#endif // WHY_H

