#include "TypingContext.h"

#include <cassert>

static TypingContext::TypeInfos* typeInfos;

void TypingContext::makeTypeInfos(QDomDocument doc, TypeInfos *result)
{
    QDomElement typeElem = doc.documentElement().firstChildElement("TypeInfos").firstChildElement("Type");
    while(!typeElem.isNull()) {
        uint id = typeElem.attribute("id").toUInt();
        if(!result->contains(id))
            result->insert(id, typeElem);
        typeElem = typeElem.nextSiblingElement("Type");
    }
}

void TypingContext::setTypeInfos(TypeInfos *ti) {
    typeInfos = ti;
}

QDomElement TypingContext::getType(QDomElement e) {
    assert(!e.isNull());
    if(e.hasAttribute("typref")){
        QString trs = e.attribute("typref");
        uint tr = trs.toUInt();
        QDomElement res = (*typeInfos)[tr].firstChildElement();
        assert(!res.isNull());
        return res;
    }
    else if(!e.firstChildElement("Attr").firstChildElement("Type").isNull()){
        return e.firstChildElement("Attr").firstChildElement("Type").firstChildElement();
    }
    assert(false);
}

QDomElement TypingContext::getType(unsigned typref) {
    return (*typeInfos)[typref].firstChildElement();
}

bool TypingContext::isInt(QDomElement e) {
    if (e.tagName() == "Integer_Literal")
        return true;
    QDomElement ti = getType(e);
    return (ti.tagName() == "Id" && ti.attribute("value") == "INTEGER");
}

bool TypingContext::isReal(QDomElement e) {
    if (e.tagName() == "Real_Literal")
        return true;
    QDomElement ti = getType(e);
    return (ti.tagName() == "Id" && ti.attribute("value") == "REAL");
}

bool TypingContext::isFloat(QDomElement e) {
    QDomElement ti = getType(e);
    return (ti.tagName() == "Id" && ti.attribute("value") == "FLOAT");
}
