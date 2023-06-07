#ifndef UTILS_H
#define UTILS_H

#define cstrEq(s1, s2) (strcmp(s1, s2) == 0)
#define sstrEq(s1, s2) ((s1).compare(s2) == 0)

#define child1(e) (e)->FirstChildElement()
#define childX(e) (e)->NextSiblingElement()

#endif // UTILS_H