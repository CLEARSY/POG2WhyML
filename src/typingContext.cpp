/** typingContext.cpp

   \copyright Copyright © CLEARSY 2019-2026
   \license This file is part of POGLIB.

   POG2WhyML is free software: you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

    POG2WhyML is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with POG2WhyML. If not, see <https://www.gnu.org/licenses/>.
*/
#include "typingContext.h"

#include <cassert>

static TypingContext::TypeInfos *typeInfos;

void TypingContext::makeTypeInfos(QDomDocument doc, TypeInfos *result) {
  QDomElement typeElem = doc.documentElement()
                             .firstChildElement("TypeInfos")
                             .firstChildElement("Type");
  while (!typeElem.isNull()) {
    uint id = typeElem.attribute("id").toUInt();
    if (!result->contains(id)) result->insert(id, typeElem);
    typeElem = typeElem.nextSiblingElement("Type");
  }
}

void TypingContext::setTypeInfos(TypeInfos *ti) { typeInfos = ti; }

QDomElement TypingContext::getType(QDomElement e) {
  assert(!e.isNull());
  if (e.hasAttribute("typref")) {
    QString trs = e.attribute("typref");
    uint tr = trs.toUInt();
    QDomElement res = (*typeInfos)[tr].firstChildElement();
    assert(!res.isNull());
    return res;
  } else if (!e.firstChildElement("Attr").firstChildElement("Type").isNull()) {
    return e.firstChildElement("Attr")
        .firstChildElement("Type")
        .firstChildElement();
  }
  assert(false);
}

QDomElement TypingContext::getType(unsigned typref) {
  return (*typeInfos)[typref].firstChildElement();
}

bool TypingContext::isInt(QDomElement e) {
  if (e.tagName() == "Integer_Literal") return true;
  QDomElement ti = getType(e);
  return (ti.tagName() == "Id" && ti.attribute("value") == "INTEGER");
}

bool TypingContext::isReal(QDomElement e) {
  if (e.tagName() == "Real_Literal") return true;
  QDomElement ti = getType(e);
  return (ti.tagName() == "Id" && ti.attribute("value") == "REAL");
}

bool TypingContext::isFloat(QDomElement e) {
  QDomElement ti = getType(e);
  return (ti.tagName() == "Id" && ti.attribute("value") == "FLOAT");
}
