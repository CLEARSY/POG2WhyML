/** typingContext.h

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
#ifndef TyC_H
#define TyC_H

#include <QDomDocument>
#include <QtCore>

namespace TypingContext {
typedef QHash<uint, QDomElement> TypeInfos;

QDomElement getType(QDomElement e);
QDomElement getType(unsigned typref);
void makeTypeInfos(QDomDocument doc, TypeInfos* result);
void setTypeInfos(TypeInfos* typeInfos);

bool isInt(QDomElement e);
bool isReal(QDomElement e);
bool isFloat(QDomElement e);
}  // namespace TypingContext

#endif  // TyC_H
