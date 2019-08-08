#ifndef TyC_H
#define TyC_H

#include <QtCore>
#include <QDomDocument>

namespace TypingContext {
    typedef QHash< uint, QDomElement > TypeInfos;

    QDomElement getType(QDomElement e);
    QDomElement getType(unsigned typref);
    QDomElement getUninterpreted(QDomElement e);
    void makeTypeInfos(QDomDocument doc, TypeInfos* result);
    void setTypeInfos(TypeInfos *typeInfos);

    bool isInt(QDomElement e);
    bool isReal(QDomElement e);
    bool isFloat(QDomElement e);
}

#endif // PO_H
