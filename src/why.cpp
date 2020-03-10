#include<algorithm>
#include<iostream>
#include<map>
#include<vector>

using std::map;
using std::pair;
using std::vector;

#include "why.h"
#include "../POG/po.h"
#include "typingContext.h"

#define child1(e) e.firstChildElement()
#define childX(e) e.nextSiblingElement()

static int nbLambda = 0;
static int nbSet = 0;
static int nbUnion = 0;
static int nbInter = 0;
static int nbStruct = 0;
static int nbSigma = 0;
static int nbPi = 0;
static QVector<QString> obfsucV; /** @brief already obfuscated names, order is meaningful */
QMap<QString,QString> structTypesG; /** useless??? */
static bool obf = true;

static bool eqType
(const QDomElement& e1,
 const QDomElement& e2);

static QString whyApply
(const QString& fun,
 const QVector<QString>& args);

/*
  Encoding of struct (register types).

  In B, two different register types may have the same field name.
  In Why this is not permitted.

  So it is necessary to mangle record field names.

  For each register type in the source B, a unique identifier is
  associated. This identifier is a counter, starting from 0. It
  is combined with the source field name to create the target
  field name. The mangled name is constructed as follows:
  "l_" + source_label + "_" + type_id.

  All register types are stored in a vector registryStruct.
  The position of the register type is the same as its identifier.

  When a Struct element is translated, one needs its identifier.
  The process is as follows:
  the vector registryStruct is searched for an identical element.
  If found, the result is the position of the element found.
  Otherwise, the element is added at the end of registryStruct,
  and the result is its position (size minus one).
*/
static vector<QDomElement> registryStruct;

static unsigned lookupStruct(const QDomElement& e)
{
    unsigned i;
    for(i = 0; i < registryStruct.size(); ++i) {
        if(eqType(e, registryStruct.at(i)))
            return i;
    }
    registryStruct.push_back(e);
    return i;
}

/*
  Also, for each B register type, a Why register type is
  created. It has a name and an expression.

  For B register type "struct(xx : INTEGER, yy : INTEGER)",
  if its position in registryStruct is 0, then:
  whyStructName[0] == "type_struct_0"
  whyStructName[0] == "{ l_xx_0 : int ; l_yy_0 : int }
*/

static vector<QString> whyStructName;
static vector<QString> whyStructExpr;

/*
  For each B record expression, there is a typref attribute
  that corresponds to a Struct type. It is necessary to
  translate this type to Why, hence to store the source
  B element into registryStruct and useful to get know what
  is the index of this type in registryStruct.

  Once this translation has been realized, the association
  between the typref and the index in StructRegistry is
  memorized. This is the role of the variable
  whyStructTyprefToIndex.
*/
static map<unsigned int, unsigned int> whyStructTyprefToIndex;

static unsigned getStructIdOfTypref(unsigned typref) {
    unsigned id;
    map<unsigned, unsigned>::iterator it =
        whyStructTyprefToIndex.find(typref);
    if(it == whyStructTyprefToIndex.end()) {
        id = lookupStruct(TypingContext::getType(typref));
        whyStructTyprefToIndex.insert(it,
                                      pair<unsigned, unsigned>(typref, id));
        return id;
    }
    else {
        return it->second;
    }
}

static bool isEnumeratedSetElement(const QDomElement& e);
static bool isDeferredSetElement(const QDomElement& e);
static QString deferredSetWhyName(const QDomElement& e);
static QString whyName(const QDomElement& ident, const QMap<QString,QString>& enums, bool& var);

static QString obfusc(QString name)
{
    if (!obf)
        return name;
    int i = obfsucV.indexOf(name);
    QString obfs;
    if(name.length() > 0 && name.at(0).isUpper())
        obfs = "Obf_";
    else
        obfs = "obf_";
    if (i == -1) {
        obfsucV << name;
        return obfs + QString::number(obfsucV.size());
    }
    else
        return obfs + QString::number(i+1);
}

/**
 * @brief appendPrelude prints Why3 prelude to import all definitions for the B theory.
 * @param out is the stream receiving the produced text
 */
void appendPrelude(QTextStream &out)
{
    out << "theory B_translation\n";
    out << "  use bool.Bool\n";
    out << "  use int.Int\n";
    out << "  use int.Power\n";
    out << "  use int.ComputerDivision\n";
    out << "    (* See B-Book, section 3.5.7-Arithmetic, Division, p. 164\n";
    out << "       and section 3.6-Integers, table p. 173 *)\n";
    out << "\n";
    out << "  use bpo2why_prelude.B_BOOL\n";
    out << "  use bpo2why_prelude.Interval\n";
    out << "  use bpo2why_prelude.PowerSet\n";
    out << "  use bpo2why_prelude.Relation\n";
    out << "  use bpo2why_prelude.Image\n";
    out << "  use bpo2why_prelude.Identity\n";
    out << "  use bpo2why_prelude.InverseDomRan\n";
    out << "  use bpo2why_prelude.Function\n";
    out << "  use bpo2why_prelude.Sequence\n";
    out << "  use bpo2why_prelude.IsFinite\n";
    out << "  use bpo2why_prelude.PowerRelation\n";
    out << "  use bpo2why_prelude.Restriction\n";
    out << "  use bpo2why_prelude.Overriding\n";
    out << "  use bpo2why_prelude.Composition\n";
    out << "  use bpo2why_prelude.BList\n";
    out << "  use bpo2why_prelude.MinMax\n";
    out << "  use bpo2why_prelude.Projection\n";
    out << "  use bpo2why_prelude.Iteration\n";
    out << "  use bpo2why_prelude.Generalized\n";
    out << "\n";
    //out << "  type uninterpreted_type\n";
    out << "\n";
}

/**
 * @brief saveEnumeration prints type definitions corresponding to enumerated sets and
 * builds translation dictionary from B names to why3 names.
 * @param out is the text stream receiving the text produced
 * @param enums is map where the B to why3 translation dictionary is stored
 * @param set is a DOM element containing enumerations to be translated
 * @details For the B enumeration
 *   s1 = { e1, e2, e3 }
 * The following why3 output is produced
*   type enum_s1 = E_e1 | E_e2 | E_e3
 *   function set_enum_s1 : set enum_s1
 *   axiom set_enum_s1_axiom :
 *     forall x:enum_s1. mem x set_enum_s1
 *
 */
void saveEnumeration(QTextStream &out, QMap< QString, QString > &enums, QDomElement &set)
{
    Q_ASSERT(isEnumeratedSetElement(set));
    QString setName = set.firstChildElement("Id").attribute("value");

    if (!enums.contains(setName)) {
        QDomElement enumE = set.firstChildElement("Enumerated_Values");
        QString obfSetName = obfusc(setName);
        QString enumName = "enum_" + obfSetName;
        QString axiomName = "set_enum_" + obfSetName + "_axiom";
        QStringList elemStringList;
        enums[setName] = "set_enum_" + obfSetName;
        for (QDomElement elem = enumE.firstChildElement("Id"); !elem.isNull(); elem = elem.nextSiblingElement("Id")) {
            QString stringB = elem.attribute("value");
            QString stringW = obfusc("E_" + stringB);
            elemStringList << stringW;
            enums[stringB] = stringW;
        }
        out << "type " << enumName << " = " << elemStringList.join(" | ") << "\n";
        out << "function " << enums[setName] << " : set " << enumName << "\n";
        out << "axiom " << axiomName << ":\n";
        out << "forall x:" << enumName << ". mem x " << enums[setName] << "\n\n";
    }
}

void saveWhy3(QDomDocument pog, QFile& why, bool xml, bool obfs) {
    QDomElement root = pog.documentElement();
    obf = obfs;
    nbLambda = 0;
    nbSet = 0;
    if (!why.open(QIODevice::WriteOnly))
        return;
    QTextStream out(&why);

    appendPrelude(out);

    TypeInfos* typeInfos {new TypeInfos};
    TypingContext::makeTypeInfos(pog, typeInfos);
    TypingContext::setTypeInfos(typeInfos);

    QDomElement elem;

    QMap<QString,QString> enums; // maps B names to why3 names.
    QMap<QString,whyDefine*> defines;

    // Enumerated sets declaration
    for (elem = root.firstChildElement("Define"); !elem.isNull(); elem = elem.nextSiblingElement("Define")) {
        for (QDomElement set = elem.firstChildElement("Set"); !set.isNull(); set = set.nextSiblingElement("Set")) {
            if (isEnumeratedSetElement(set))
                saveEnumeration(out, enums, set);
        }
    }
    out << "\n";

    // Defines declaration
    int count;
    for (count = 1, elem = root.firstChildElement("Define");
         !elem.isNull();
         ++count, elem = elem.nextSiblingElement("Define")) {
        whyDefine* wd = new whyDefine(elem, count);
        wd->collectVarAndTypes(enums);
        wd->translate(enums);
        wd->declare(out);
        defines[elem.attribute("name")] = wd;
    }
    out << "\n";

    // Local hypothesis declaration
    QMap<int, whyLocalHyp*> localHyps;
    int goal = 0;
    for (elem = root.firstChildElement("Proof_Obligation"); !elem.isNull(); elem = elem.nextSiblingElement("Proof_Obligation")) {
        if (elem.firstChildElement("Simple_Goal").isNull()) continue;
        for (QDomElement lh = elem.firstChildElement("Local_Hyp"); !lh.isNull(); lh = lh.nextSiblingElement("Local_Hyp")) {
            whyLocalHyp* wlh = new whyLocalHyp(lh, goal, lh.attribute("num"));
            wlh->collectVarAndTypes(enums);
            wlh->translate(enums);
            wlh->declareWithoutDuplicate(localHyps.values(), out);
            localHyps[goal] = wlh;
        }
        ++goal;
    }

    // Goals declaration
    int simple_goal = 0;
    for (goal = 0, elem = root.firstChildElement("Proof_Obligation");
         !elem.isNull();
         ++goal, elem = elem.nextSiblingElement("Proof_Obligation")) {
        if (xml) {
            QDomElement tag = elem.firstChildElement("Tag");
            std::cout << "<translate>"
                      << tag.text().toStdString()
                      << "</translate>"
                      << std::endl;
        }
        if (!elem.firstChildElement("Simple_Goal").isNull()) {
            QVector<whyDefine*> sgDefinitions;
            for (QDomElement definition = elem.firstChildElement("Definition");
                 !definition.isNull();
                 definition = definition.nextSiblingElement("Definition")) {
                if (!defines.contains(definition.attribute("name"))) {
                    throw WhyTranslateException("Unknown definition " + definition.attribute("name"));
                }
                sgDefinitions << defines[definition.attribute("name")];
            }
            whyLocalHyp* wh;
            if (!elem.firstChildElement("Hypothesis").isNull()) {
                wh = new whyLocalHyp(elem.firstChildElement("Hypothesis"), goal, "0");
                wh->collectVarAndTypes(enums);
                wh->translate(enums);
            } else {
                wh = new whyLocalHyp(elem, goal, "0");
            }
            wh->declare(out);

            for(QDomElement sg = elem.firstChildElement("Simple_Goal");
                !sg.isNull();
                sg = sg.nextSiblingElement("Simple_Goal")) {
                whySimpleGoal* wsg = new whySimpleGoal(sg, simple_goal, localHyps.values(goal), sgDefinitions, wh);
                wsg->collectVarAndTypes(enums);
                wsg->translate(enums);
                wsg->declare(out);
                simple_goal++;
            }
        }
    }
    out << "\n";
    out << "end\n";
    why.close();
}

void saveWhy3(QDomDocument pog, QFile& why, const std::map<int, std::vector<int>>& goals) {
    bool saveObfuscationFlag {obf};
    QDomElement root = pog.documentElement();
    nbLambda = 0;
    nbSet = 0;
    if (!why.open(QIODevice::WriteOnly))
        return;
    QTextStream out(&why);

    obf = false;
    appendPrelude(out);

    TypeInfos* typeInfos {new TypeInfos};
    TypingContext::makeTypeInfos(pog, typeInfos);
    TypingContext::setTypeInfos(typeInfos);

    QDomElement elem;
    QMap<QString,QString> enums; // maps B names to why3 names.
    QMap<QString,whyDefine*> defines;

    // Enumerated sets declaration
    for (elem = root.firstChildElement("Define");
         !elem.isNull();
         elem = elem.nextSiblingElement("Define")) {
        for (QDomElement set = elem.firstChildElement("Set");
             !set.isNull();
             set = set.nextSiblingElement("Set")) {
            if (isEnumeratedSetElement(set))
                saveEnumeration(out, enums, set);
        }
    }
    out << "\n";

    // Defines declaration
    int count;
    for (count = 1, elem = root.firstChildElement("Define");
         !elem.isNull();
         ++count, elem = elem.nextSiblingElement("Define")) {
        whyDefine* wd = new whyDefine(elem, count);
        wd->collectVarAndTypes(enums);
        wd->translate(enums);
        wd->declare(out);
        defines[elem.attribute("name")] = wd;
    }
    out << "\n";

    // Local hypothesis declaration
    QMultiMap<int, whyLocalHyp*> localHyps;
    int group = 0;
    for (elem = root.firstChildElement("Proof_Obligation");
         !elem.isNull();
         ++group, elem = elem.nextSiblingElement("Proof_Obligation")) {
        if (goals.find(group) == goals.end())
            continue;
        QDomElement lh;
        for (lh = elem.firstChildElement("Local_Hyp");
             !lh.isNull();
             lh = lh.nextSiblingElement("Local_Hyp")) {
            whyLocalHyp* wlh {new whyLocalHyp(lh, group, lh.attribute("num"))};
            wlh->collectVarAndTypes(enums);
            wlh->translate(enums);
            wlh->declareWithoutDuplicate(localHyps.values(), out);
            localHyps.insert(group, wlh);
        }
    }

    int why3goal = 0;
    for (group = 0, elem = root.firstChildElement("Proof_Obligation");
         !elem.isNull();
         ++group, elem = elem.nextSiblingElement("Proof_Obligation")) {
        if(goals.find(group) == goals.end())
            continue;
        if (elem.firstChildElement("Simple_Goal").isNull())
            continue;
        QVector<whyDefine*> sgDefinitions;
        for (QDomElement definition = elem.firstChildElement("Definition");
             !definition.isNull();
             definition = definition.nextSiblingElement("Definition")) {
            if (!defines.contains(definition.attribute("name")))
                throw WhyTranslateException("Unknown definition " + definition.attribute("name"));
            sgDefinitions << defines[definition.attribute("name")];
        }
        whyLocalHyp* wh;
        if (!elem.firstChildElement("Hypothesis").isNull()) {
            wh = new whyLocalHyp(elem.firstChildElement("Hypothesis"), group, "0");
            wh->collectVarAndTypes(enums);
            wh->translate(enums);
        } else {
            wh = new whyLocalHyp(elem, group, "0");
        }
        wh->declare(out);

        const std::vector<int>& simple_goals {goals.at(group)};
        const int nb_simple_goals {static_cast<int>(simple_goals.size())};
        int j; /// @brief counts Simple_Goal child elements
        int k; /// @brief index in sgoals.at(i)
        QDomElement sg;

        for(j = 0, k = 0, sg = elem.firstChildElement("Simple_Goal");
            k < nb_simple_goals && !sg.isNull();
            ++j, sg = sg.nextSiblingElement("Simple_Goal")) {
            if (j < simple_goals[k])
                continue;
            whySimpleGoal* wsg = new whySimpleGoal(sg, why3goal, localHyps.values(group), sgDefinitions, wh);
            wsg->collectVarAndTypes(enums);
            wsg->translate(enums);
            wsg->declare(out);
            ++k;
            ++why3goal;
        }
    }
    delete typeInfos;

    out << "\n";
    out << "end\n";
    why.close();
    obf = saveObfuscationFlag;
}

void saveWhy3(TypeInfos* typeInfos, QList<QDomElement> hypotheses, QDomElement goal, QFile& file)
{
    if (!file.open(QIODevice::WriteOnly))
        return;
    QTextStream out(&file);

    obf = false;
    appendPrelude(out);

    TypingContext::setTypeInfos(typeInfos);

    QMap< QString, QString > enums; // maps B names to why3 names
    QMap< QString, whyDefine* > defines;

    // Enumerated sets declaration
    for (int i =0; i < hypotheses.size(); i++) {
	QDomElement elem = hypotheses.at(i);
        if (elem.nodeName() == "Set") {
            if (isEnumeratedSetElement(elem))
                saveEnumeration(out, enums, elem);
        }
    }
    out << "\n";

    // Defines declaration
    int count = 1;
    QVector< whyLemmaDefine* > wDefines;
    for (int i =0; i < hypotheses.size(); i++) {
	QDomElement elem = hypotheses.at(i);
        if (elem.nodeName() != "Set") {
            whyLemmaDefine* wd = new whyLemmaDefine(elem, count);
            wd->collectVarAndTypes(enums);
            wd->translate(enums);
            wd->declare(out);
            wDefines.push_back(wd);
            defines[elem.attribute("name")] = wd;
            ++count;
        }
    }

    whyLemmaGoal* wgoal = new whyLemmaGoal(goal, wDefines);
    wgoal->collectVarAndTypes(enums);
    wgoal->translate(enums);
    wgoal->declare(out);

    out << "\n" << "end\n";
    out.flush();

    // cleanup
    delete wgoal;
    for (int i =0; i < wDefines.size(); i++) {
	whyLemmaDefine* ptr = wDefines.at(i);
        delete ptr;
    }
}

whyDefine::whyDefine(QDomElement d, int count): whyPredicate(d)
{
    predName = "define" + QString::number(count);
    name = d.attribute("name");
}

whyPredicate::whyPredicate(QDomElement _e) {
    e = _e;
}

whyPredicate::whyPredicate(QDomElement _e, QList<whyLocalHyp*> lh) {
    e = _e;
    localHyps = lh;
}

void whyPredicate::collectVarAndTypes(QMap<QString,QString> enums) {
    for (QDomElement e1 = e.firstChildElement(); !e1.isNull(); e1 = e1.nextSiblingElement()) {
        if (isDeferredSetElement(e1)) {
            QString name = deferredSetWhyName(e1);
            if (!variables.contains(name)) {
                variables << name;
                types << "set int";
            }
        }
        else
            collectVarAndTypes(&variables, &types, e1, enums, localHyps);
    }
}


static QString whyName(const QDomElement& ident, const QMap<QString,QString>& enums, bool& var) {
    var = false;
    QString val = ident.attribute("value");
    if (val == "BOOL")
        return "b_bool";
    if (val == "INTEGER")
        return "integer";
    if (val == "NATURAL")
        return "natural";
    if (val == "NATURAL1")
        return "natural1";
    if (val == "NAT")
        return "nat";
    if (val == "NAT1")
        return "nat1";
    if (val == "INT")
        return "bounded_int";
    if (val == "STRING")
        return "string";
    if (val == "MAXINT") {
        QString val = po::getMaxint();
        if (val.startsWith('-'))
            return "(" + val + ")";
        else
            return val;
    }
    if (val == "MININT") {
        QString val = po::getMinint();
        if (val.startsWith('-'))
            return "(" + val + ")";
        else
            return val;
    }
    if (enums.contains(val))
        return enums.value(val);

    var = true;
    QString name;
    if (val.contains('.'))
        name = "renamed_v_" + val.replace('.','_') + "_" + ident.attribute("suffix");
    else
        name = "v_" + val + "_" + ident.attribute("suffix");
    return obfusc(name);
}

void whyPredicate::collectVarAndTypes(QVector<QString>* variables,
                                      QVector<QString>* types,
                                      QDomElement formula,
                                      QMap<QString,QString> enums,
                                      QList<whyLocalHyp*> localHyps) {
    if (formula.isNull())
        return;
    QString tag = formula.tagName();
    if (tag == "Attr")
        return;
    if (tag == "Id") {
        bool var;
        QString name = whyName(formula, enums, var);
        if (var && !variables->contains(name)) {
            variables->append(name);
            types->append(translateTypeInfo(TypingContext::getType(formula), enums));
        }
        return;
    }
    if (tag == "Ref_Hyp") {
        QString num = formula.attribute("num");

        for (int i =0; i < localHyps.size(); i++) {
            whyLocalHyp* hyp = localHyps.at(i);
            if (hyp->is(num)) {
                QVector<QString> v = hyp->getVariables();
                for (int j=0;j<v.size();j++) {
                    if (!variables->contains(v.at(j))) {
                        variables->append(v.at(j));
                        types->append(hyp->getTypes().at(j));
                    }
                }
            }
        }
        return;
    }
    if (tag == "Quantified_Pred" || tag == "Quantified_Exp" || tag == "Quantified_Set") {
        QVector<QString> qVariables;
        QVector<QString> qTypes;
        QDomElement child = formula.firstChildElement();
        while(!child.isNull()) {
            QDomElement next = child.nextSiblingElement();
            collectVarAndTypes(&qVariables, &qTypes, child, enums, localHyps);
            child = next;
        }
        QDomElement vars = formula.firstChildElement("Variables").firstChildElement("Id");
        while (!vars.isNull()) {
            bool isVar;
            int i = qVariables.indexOf(whyName(vars, enums, isVar));
            qVariables.remove(i);
            qTypes.remove(i);
            vars = vars.nextSiblingElement("Id");
        }
        for (int i = 0; i < qVariables.size(); i++) {
            if (!variables->contains(qVariables.at(i))) {
                variables->append(qVariables.at(i));
                types->append(qTypes.at(i));
            }
        }
       return;
    }
    if (tag == "Binary_Exp" || tag == "Ternary_Exp") {
        QString op = formula.attribute("op");
        if (op == "'") {
            collectVarAndTypes(variables, types, formula.firstChildElement(), enums, localHyps);
            return;
        }
        if (op == "<'") {
            collectVarAndTypes(variables, types, formula.firstChildElement(), enums, localHyps);
            collectVarAndTypes(variables, types, formula.firstChildElement().nextSiblingElement().nextSiblingElement(), enums, localHyps);
            return;
        }
    }

    QDomElement child = formula.firstChildElement();
    while(!child.isNull()) {
        QDomElement next = child.nextSiblingElement();
        collectVarAndTypes(variables, types, child, enums, localHyps);
        child = next;
    }
}

QString whyPredicate::translateTypeStruct
(const QDomElement& e,
 const QMap<QString,QString>& enums)
{

    unsigned id = lookupStruct(e);
    if(id == whyStructName.size()) {
        QString suffix = QString::number(id);
        QString l = "{";
        QDomElement ri;
        bool first;
        for(ri = e.firstChildElement("Record_Item"), first = true;
            !ri.isNull();
            ri = ri.nextSiblingElement("Record_Item")) {
            if(first)
                first = false;
            else
                l+= ";";
            l += "l_" + ri.attribute("label") + "_" + suffix
                + " : " + translateTypeInfo(child1(ri), enums);
        }
        l += "}";
        QString name = "type_struct_" + suffix;
        whyStructName.push_back(name);
        whyStructExpr.push_back(l);
        structTypes[l] = name;
    }

    return whyStructName.at(id);
}

QString whyPredicate::translateTypeInfo
(QDomElement ti, const QMap<QString,QString>& enums)
{
    QString tag = ti.tagName();
    QStringList localLabels;
    if (tag == "Struct") {
        return translateTypeStruct(ti, enums);
    }
    if (tag == "Unary_Exp" && ti.attribute("op") == "POW") {
        return "(set " + translateTypeInfo(child1(ti),enums) + ")";
    }
    if (tag == "Binary_Exp" && ti.attribute("op") == "*") {
        return "(" + translateTypeInfo(child1(ti),enums) + ","
            + translateTypeInfo(childX(child1(ti)),enums) + ")";
    }
    if (tag == "Id" && ti.attribute("value") == "INTEGER")
        return "int";
    if (tag == "Id" && ti.attribute("value") == "BOOL")
        return "bool";
    if (tag == "Id" && ti.attribute("value") == "STRING")
        return "set (int, int)";
    if (tag == "Id" && enums.contains(ti.attribute("value"))) {
        QString s = enums.value(ti.attribute("value"));
        s.remove(0,4); // Pour supprimer le préfixe set_
        return s;
    }
    if (tag == "Id") {
       return "int";// (*" + ti.attribute("value") + "*)";
    }

    throw WhyTranslateException("Unknown type element " + tag);
}

void whyPredicate::declare(QTextStream& out) {
    /*
       Record types may depend from one another (non recursively).
       If type t1 depends on type t2, then the declaration of t2
       must precede that of t1.
       This is the sorting algorithm to handle these dependencies.
    */
    QStringList structs;
    QMapIterator<QString, QString> i(structTypes);
    while (i.hasNext()) {
        i.next();
        int j=0;
        while (j<structs.size()) {
            if (structs.at(j).contains(i.value()))
                break;
            else
                j++;
        }
        structs.insert(j,"type " + i.value() + " = " + i.key());
    }
    out << structs.join("\n");
    out << "\n";
    for (int i=0;i<functions.size();i++) {
        out << functions.at(i);
        out << "\n";
    }
    out << "\npredicate " << predName << " ";
    for (int i=0;i < variables.size(); i++) {
        out << "(" + variables.at(i) + ": " + types.at(i) + ")";
    }
    out << " =\n   " << predicate << "\n\n";
}

void whyPredicate::translate(const QMap<QString,QString>& enums) {
    QStringList res;
    res.append("true");
    QDomElement e1 = e.firstChildElement();
    while (!e1.isNull()) {
        QString tag = e1.tagName();
        if (tag != "Set" && tag != "Tag" && tag != "Proof_State" && tag != "iapa") {
            res.append(translate(e1, enums));
        }
        e1 = e1.nextSiblingElement();
    }
    predicate = res.join(" /\\\n");
}

QDomElement whyPredicate::subset(QDomElement e) {
    QString tag1 = e.tagName();
    assert(tag1 == "Unary_Exp");
    return e.firstChildElement();
}

QDomElement whyPredicate::left(const QDomElement e) {
    QString tag1 = e.tagName();
    assert(tag1 == "Binary_Exp");
    return e.firstChildElement();
}

QDomElement whyPredicate::right(const QDomElement e) {
    QString tag1 = e.tagName();
    assert(tag1 == "Binary_Exp");
    return e.firstChildElement().nextSiblingElement();
}

QDomElement getLabel(QString label, QDomElement rec) {
    QDomElement ri = rec.firstChildElement("Record_Item");
    while (!ri.isNull()) {
        if (obfusc(ri.attribute("label")) == label) {
            return ri.firstChildElement().cloneNode().toElement();
        }
        ri = ri.nextSiblingElement("Record_Item");
    }
    throw WhyTranslateException("Cannot find label " + label);
}

QString getLabelN(QDomElement rec, int n) {
    int i = n;
    QDomElement ri = rec.firstChildElement("Record_Item");
    while (!ri.isNull()) {
        i--;
        if (i == 0) {
            return ri.attribute("label");
        }
        ri = ri.nextSiblingElement("Record_Item");
    }
    throw WhyTranslateException("Cannot find label " + QString::number(n));
}

QString whyPredicate::translatePred(const QDomElement formula, const QMap<QString,QString>& enums) {
    QStringList res = translate(formula, enums);
    if (res.isEmpty()) {
        return "true";
    } else {
        return res.join(" /\\ ");
    }
}

QString whyPredicate::translateId
(const QDomElement& e,
 const QMap<QString,QString>& enums)
{
    bool var;
    return whyName(e, enums, var);
}

/*
  The translation to Why of a B struct expression follows
  this rule:

  struct(xx: AA, yy: BB, zz : BB)

  where xx, yy, zz are field names and AA, BB are field types.
  There is no more field types than field names, and there
  might be fewer.

  For Why, a function named "anon_struct_<I_AS>"
  where I_AS is a unique identifier.

  function anon_struct_<I_AS>
         (v_AA_: (set SA))
         (v_BB_: (set SB)) : (set type_struct_<I_TS>)

  The function parameters are the field types that are
  not predefined types (INTEGER/int, BOOLEAN/bool).

  <I_TS> is the index of the register type in
  structRegistry.

  And the following axiomatizes this operator:

  axiom anon_struct_0_def :
      forall
        v_AA_: (set SA),
        v_BB_: (set SB),
        p_xx : SA,
        p_yy : SB,
        p_zz : SB .
        mem
                {
                        l_xx_<I_TS> = p_xx;
                        l_yy_<I_TS> = p_yy;
                        l_zz_<I_TS> = p_zz
                }
                (anon_struct_<I_AS> v_AA_ v_BB_ )
                <->
                (mem p_xx v_AA_ /\
                 mem p_yy v_BB_ /\
                 mem p_zz v_BB_)
*/

QString
whyPredicate::translateStruct
(const QDomElement& formula,
 const QMap<QString,QString>& enums)
{
    int i_as = nbStruct++;
    QString l;
    QString function_name = "anon_struct_" + QString::number(i_as);
    QString axiom_name = function_name + "_def";

    unsigned i_ts;
    {
        bool ok = true;
        unsigned typref = formula.attribute("typref").toUInt(&ok, 10);
        if(!ok)
            throw WhyTranslateException("Untyped struct expression found.");
        QDomElement e = TypingContext::getType(typref);
        if(e.tagName() != "Unary_Exp")
            throw WhyTranslateException("Typing error");
        i_ts = lookupStruct(child1(e));
    }
    QString struct_name = "type_struct_" + QString::number(i_ts);

    QVector<QString> qVariables;
    QVector<QString> qTypes;
    QList<whyLocalHyp*> qLocalHyps;
    collectVarAndTypes(&qVariables,
                       &qTypes,
                       formula,
                       enums,
                       qLocalHyps);
    l += "function " + function_name + " ";
    for (int i=0;i<qVariables.size();i++)
        l += "(" + qVariables.at(i) + ": " + qTypes.at(i) + ")";
    l += ": (set " + struct_name + ")\n";

    l += "axiom " + axiom_name + " : forall ";
    bool first = true;
    for (int i=0;i<qVariables.size();i++) {
        if (first)
            first = false;
        else
            l += ", ";
        l += qVariables.at(i) + ": " + qTypes.at(i);
    }
    QDomElement ri = formula.firstChildElement("Record_Item");
    while (!ri.isNull()) {
        if (first)
            first = false;
        else
            l+= ", ";
        l += "p_" + ri.attribute("label") + " : ";
        l += translateTypeInfo(TypingContext::getType(child1(ri)).firstChildElement(),enums);
        ri = ri.nextSiblingElement("Record_Item");
    }
    l += ". mem {";
    first = true;
    ri = formula.firstChildElement("Record_Item");
    while (!ri.isNull()) {
        if (first)
            first = false;
        else
            l+= ";";
        l += "l_" + ri.attribute("label") + "_" + QString::number(i_ts) +
            " = p_" + ri.attribute("label");
        ri = ri.nextSiblingElement("Record_Item");
    }
    l += "} (" + function_name + " ";
    for (int i=0;i<qVariables.size();i++)
        l += qVariables.at(i) + " ";
    l += ") <-> (";
    first = true;
    ri = formula.firstChildElement("Record_Item");
    while (!ri.isNull()) {
        if (first)
            first = false;
        else
            l+= " /\\ ";
        l += "mem p_" + ri.attribute("label") + " ";
        l += translate(child1(ri), enums).at(0);
        ri = ri.nextSiblingElement("Record_Item");
    }
    l += ")\n";
    functions << l;
    return whyApply(function_name + " ", qVariables);
}

QString
whyPredicate::translateRecord
(const QDomElement& formula,
 const QMap<QString, QString>& enums)
{
    bool ok;
    unsigned typref = formula.attribute("typref", "").toUInt(&ok, 10);
    if(!ok)
        throw WhyTranslateException("Untyped record found.");

    QString suffix = QString::number(getStructIdOfTypref(typref));
    QString res = "{";
    QDomElement ri;
    bool first;
    for(ri = formula.firstChildElement("Record_Item"), first = true;
        !ri.isNull();
        ri = ri.nextSiblingElement("Record_Item")) {
        if (first)
            first = false;
        else
            res+= "; ";
        res += "l_" + ri.attribute("label") + "_" + suffix + " = ";
        res += translate(child1(ri), enums).at(0);
    }
    res += "}";
    return res;
}

QString
whyPredicate::translateRecordFieldAccess
(const QDomElement& e,
 const QMap<QString,QString>& enums)
{
    QString res;
    QDomElement accessed = child1(e);
    bool ok = true;
    unsigned typref = accessed.attribute("typref", "").toUInt(&ok, 10);
    if(!ok)
        throw WhyTranslateException("Record field access to untyped record.");

    QString suffix = QString::number(getStructIdOfTypref(typref));

    res = "(" + translate(accessed, enums).at(0)
        + ".l_"
        + e.attribute("label")
        + "_"
        + suffix
        + ")";

    return res;
}

QStringList whyPredicate::translate(const QDomElement formula, const QMap<QString,QString>& enums) {
    QStringList res;

    QDomElement fce = formula.firstChildElement();
    QDomElement sce;
    if (!fce.isNull())
        sce = fce.nextSiblingElement();

    QDomElement tce;
    if (!sce.isNull())
        tce = sce.nextSiblingElement();


    QString tag = formula.tagName();
    QString op = formula.attribute("op");

    if (tag == "Ref_Hyp") {
        QString num = formula.attribute("num");
        for (int i=0;i<localHyps.size();i++) {
            if (localHyps.at(i)->is(num)) {
                res << localHyps.at(i)->declareCallDuplicate();
            }
        }
        return res;
    }

    if (tag == "Binary_Pred" && op == "&") {
        res = translate(fce, enums);
        res.append(translate(sce, enums));
        return res;
    }

    if  (tag == "Binary_Pred"
            || tag == "Exp_Comparison"
            || tag == "Binary_Exp"
         || tag == "Ternary_Exp") {
        if (op == "or")
            res << "(" + translatePred(fce, enums)
                   + " \\/ "
                   + translatePred(sce, enums) + ")";
        else if (op == "=>")
            res << "(" + translatePred(fce, enums)
                   + " -> "
                   + translatePred(sce, enums) + ")";
        else if (op == "<=>")
            res << "(" + translatePred(fce, enums)
                   + " <-> "
                   + translatePred(sce, enums) + ")";
        else if (op == ":")
            res << "(mem " + translate(fce, enums).at(0)
                   + " "
                   + translate(sce, enums).at(0) + ")";
        else if (op == "/:")
            res << "not(mem " + translate(fce, enums).at(0)
                   + " "
                   + translate(sce, enums).at(0) + ")";
        else if (op == "<:")
            res << "(mem " + translate(fce, enums).at(0)
                   + " (power "
                   + translate(sce, enums).at(0) + "))";
        else if (op == "/<:")
            res << "not(mem " + translate(fce, enums).at(0)
                   + " (power "
                   + translate(sce, enums).at(0) + "))";
        else if (op == "<<:")
            res << "((mem " + translate(fce, enums).at(0)
                   + " (power "
                   + translate(sce, enums).at(0) + ")) /\\ not("
                   + translate(fce, enums).at(0) + " == "
                   + translate(sce, enums).at(0) + "))";
        else if (op == "/<<:")
            res << "not((mem " + translate(fce, enums).at(0)
                   + " (power "
                   + translate(sce, enums).at(0) + ")) /\\ not("
                   + translate(fce, enums).at(0) + " == "
                   + translate(sce, enums).at(0) + "))";
        else if (op == "=")
            if (isSet(fce))
                res << "(" + translate(fce, enums).at(0)
                       + " == "
                       + translate(sce, enums).at(0) + ")";
            else
                res << "(" + translate(fce, enums).at(0)
                       + " = "
                       + translate(sce, enums).at(0) + ")";
        else if (op == "/=")
            if (isSet(fce))
                res << "not(" + translate(fce, enums).at(0)
                       + " == "
                       + translate(sce, enums).at(0) + ")";
            else
                res << "not(" + translate(fce, enums).at(0)
                       + " = "
                       + translate(sce, enums).at(0) + ")";
        else if (op == ">=i")
            res << "(" + translate(fce, enums).at(0)
                   + " >= "
                   + translate(sce, enums).at(0) + ")";
        else if (op == ">i")
            res << "(" + translate(fce, enums).at(0)
                   + " > "
                   + translate(sce, enums).at(0) + ")";
        else if (op == "<i")
            res << "(" + translate(fce, enums).at(0)
                   + " < "
                   + translate(sce, enums).at(0) + ")";
        else if (op == "<=i")
            res << "(" + translate(fce, enums).at(0)
                   + " <= "
                   + translate(sce, enums).at(0) + ")";

        else if (op == ",")
            res << "(" + translate(fce, enums).at(0)
                   + " , "
                   + translate(sce, enums).at(0) + ")";
        else if (op == "*s")
            res << "(times " + translate(fce, enums).at(0)
                + " "
                + translate(sce, enums).at(0) + ")";
        else if (op == "*i")
            res << "(" + translate(fce, enums).at(0)
                + " * "
                + translate(sce, enums).at(0) + ")";
        else if (op == "**i")
            res << "(Power.power " + translate(fce, enums).at(0)
                   + " "
                   + translate(sce, enums).at(0) + ")";
        else if (op == "+i")
            res << "(" + translate(fce, enums).at(0)
                   + " + "
                   + translate(sce, enums).at(0) + ")";
        else if (op == "+->")
            res << "(" + translate(fce, enums).at(0)
                   + " +-> "
                   + translate(sce, enums).at(0) + ")";
        else if (op == "+->>")
            res << "(" + translate(fce, enums).at(0)
                   + " +->> "
                   + translate(sce, enums).at(0) + ")";
        else if (op == "-s")
            res << "(diff " + translate(fce, enums).at(0)
                + " "
                + translate(sce, enums).at(0) + ")";
        else if (op == "-i")
            res << "(" + translate(fce, enums).at(0)
                + " - "
                + translate(sce, enums).at(0) + ")";
        else if (op == "-->")
            res << "(" + translate(fce, enums).at(0)
                   + " --> "
                   + translate(sce, enums).at(0) + ")";
        else if (op == "-->>")
            res << "(" + translate(fce, enums).at(0)
                   + " -->> "
                   + translate(sce, enums).at(0) + ")";
        else if (op == "->")
            res << "(insert_in_front " + translate(fce, enums).at(0)
                   + " "
                   + translate(sce, enums).at(0) + ")";
        else if (op == "..")
            res << "(Interval.mk " + translate(fce, enums).at(0)
                   + " "
                   + translate(sce, enums).at(0) + ")";
        else if (op == "/i")
            res << "(div " + translate(fce, enums).at(0)
                   + " "
                   + translate(sce, enums).at(0) + ")";
        else if (op == "/\\")
            res << "(inter " + translate(fce, enums).at(0)
                   + " "
                   + translate(sce, enums).at(0) + ")";
        else if (op == "/|\\")
            res << "(restriction_head " + translate(fce, enums).at(0)
                   + " "
                   + translate(sce, enums).at(0) + ")";
        else if (op == ";")
            res << "(semicolon "
                + translate(fce, enums).at(0)
                + " "
                + translate(sce, enums).at(0)
                + ")";
        else if (op == "iterate")
            res << "(iterate "
                + translate(fce, enums).at(0)
                + " "
                + translate(sce, enums).at(0)
                + ")";
        else if (op == "<+")
            res << "(" + translate(fce, enums).at(0)
                   + " <+ "
                   + translate(sce, enums).at(0) + ")";
        else if (op == "<->")
            res << "(relation " + translate(fce, enums).at(0)
                   + " "
                   + translate(sce, enums).at(0) + ")";
        else if (op == "<-")
            res << "(insert_at_tail " + translate(fce, enums).at(0)
                   + " "
                   + translate(sce, enums).at(0) + ")";
        else if (op == "<<|")
            res << "(domain_substraction " + translate(fce, enums).at(0)
                   + " "
                   + translate(sce, enums).at(0) + ")";
        else if (op == "<|")
            res << "(domain_restriction " + translate(fce, enums).at(0)
                   + " "
                   + translate(sce, enums).at(0) + ")";
        else if (op == ">+>")
            res << "(" + translate(fce, enums).at(0)
                   + " >+> "
                   + translate(sce, enums).at(0) + ")";
        else if (op == ">->")
            res << "(" + translate(fce, enums).at(0)
                   + " >-> "
                   + translate(sce, enums).at(0) + ")";
        else if (op == ">+>>")
            res << "(" + translate(fce, enums).at(0)
                   + " >+>> "
                   + translate(sce, enums).at(0) + ")";
        else if (op == ">->>")
            res << "(" + translate(fce, enums).at(0)
                   + " >->> "
                   + translate(sce, enums).at(0) + ")";
        // Dans l'expression prj1 (E, F), E et F doivent etre de type P(T) et P(U).
        // Alors prj1 (E, F) est une relation de type P((T*U)*T).
        else if (op == "prj1")
            res << "(prj1 "
                + translate(fce, enums).at(0)
                + " "
                + translate(sce, enums).at(0)
                + ")";
        // Dans l'expression prj2(E, F), E et F doivent etre de type P(T) et P(U).
        // Alors prj2 (E, F) est une relation de type P((T*U)*U).
        else if (op == "prj2")
            res << "(prj2 "
                + translate(fce, enums).at(0)
                + " "
                + translate(sce, enums).at(0)
                + ")";
        else if (op == "><")
            res << "(direct_product " + translate(fce, enums).at(0)
                   + " "
                   + translate(sce, enums).at(0) + ")";
        else if (op == "||")
            res << "(parallel_product " + translate(fce, enums).at(0)
                   + " "
                   + translate(sce, enums).at(0) + ")";
        else if (op == "\\/")
            res << "(union " + translate(fce, enums).at(0)
                   + " "
                   + translate(sce, enums).at(0) + ")";
        else if (op == "\\|/")
            res << "(restriction_tail " + translate(fce, enums).at(0)
                   + " "
                   + translate(sce, enums).at(0) + ")";
        else if (op == "^")
            res << "(concatenation " + translate(fce, enums).at(0)
                   + " "
                   + translate(sce, enums).at(0) + ")";
        else if (op == "mod")
            res << "(mod " + translate(fce, enums).at(0)
                   + " "
                   + translate(sce, enums).at(0) + ")";
        else if (op == "|->")
            res << "(" + translate(fce, enums).at(0)
                   + " , "
                   + translate(sce, enums).at(0) + ")";
        else if (op == "|>")
            res << "(range_restriction " + translate(fce, enums).at(0)
                   + " "
                   + translate(sce, enums).at(0) + ")";
        else if (op == "|>>")
            res << "(range_substraction " + translate(fce, enums).at(0)
                   + " "
                   + translate(sce, enums).at(0) + ")";
        else if (op == "[")
            res << "(image " + translate(fce, enums).at(0)
                   + " "
                   + translate(sce, enums).at(0) + ")";
        else if (op == "(")
            res << "(apply " + translate(fce, enums).at(0)
                   + " " + translate(sce, enums).at(0) + ")";
        else if (op == "<'")
            res << "{" + translate(fce, enums).at(0)
                   + " with l_"
                   + obfusc(sce.attribute("value"))
                   + " = "
                   + translate(tce, enums).at(0) + "}";
        else
            throw WhyTranslateException(tag + " " + op);
        return res;
    }
    else if (tag == "Unary_Pred"
            || (tag == "Unary_Exp")) {
        if (op == "not") {
            res << "not("
                   + translatePred(fce, enums) + ")";
        }
        else if (op == "max" || op == "imax") {
            res << "(max "
                   + translate(fce, enums).at(0) + ")";
        }
        else if (op == "min" || op == "imin") {
            res << "(min "
                   + translate(fce, enums).at(0) + ")";
        }
        else if (op == "card") {
            res << "(card "
                   + translate(fce, enums).at(0) + ")";
        }
        else if (op == "dom") {
            res << "(dom "
                   + translate(fce, enums).at(0) + ")";
        }
        else if (op == "ran") {
            res << "(ran "
                   + translate(fce, enums).at(0) + ")";
        }
        else if (op == "POW") {
            res << "(power "
                   + translate(fce, enums).at(0) + ")";
        }
        else if (op == "POW1") {
            res << "(non_empty_power "
                   + translate(fce, enums).at(0) + ")";
        }
        else if (op == "FIN") {
            res << "(finite_subsets "
                   + translate(fce, enums).at(0) + ")";
        }
        else if (op == "FIN1") {
            res << "(non_empty_finite_subsets "
                   + translate(fce, enums).at(0) + ")";
        }
        else if (op == "union") {
            res << "(generalized_union "
                   + translate(fce, enums).at(0) + ")";
        }
        else if (op == "inter") {
            res << "(generalized_inter "
                   + translate(fce, enums).at(0) + ")";
        }
        else if (op == "seq") {
            res << "(seq "
                   + translate(fce, enums).at(0) + ")";
        }
        else if (op == "seq1") {
            res << "(seq1 "
                   + translate(fce, enums).at(0) + ")";
        }
        else if (op == "iseq") {
            res << "(iseq "
                   + translate(fce, enums).at(0) + ")";
        }
        else if (op == "iseq1") {
            res << "(iseq1 "
                   + translate(fce, enums).at(0) + ")";
        }
        else if (op == "-" || op == "-i") {
            res << "(- "
                   + translate(fce, enums).at(0) + ")";
        }
        else if (op == "~") {
            res << "(inverse "
                   + translate(fce, enums).at(0) + ")";
        }
        else if (op == "size") {
            res << "(size "
                   + translate(fce, enums).at(0) + ")";
        }
        else if (op == "perm") {
            res << "(perm "
                   + translate(fce, enums).at(0) + ")";
        }
        else if (op == "first") {
            res << "(first "
                   + translate(fce, enums).at(0) + ")";
        }
        else if (op == "last") {
            res << "(last "
                   + translate(fce, enums).at(0) + ")";
        }
        else if (op == "id") {
            res << "(id "
                   + translate(fce, enums).at(0) + ")";
        }
        else if (op == "closure") {
            res << "(closure "
                   + translate(fce, enums).at(0) + ")";
        }
        else if (op == "closure1") {
            res << "(closure1 "
                   + translate(fce, enums).at(0) + ")";
        }
        else if (op == "tail") {
            res << "(tail "
                   + translate(fce, enums).at(0) + ")";
        }
        else if (op == "front") {
            res << "(front "
                   + translate(fce, enums).at(0) + ")";
        }
        else if (op == "rev") {
            res << "(rev "
                   + translate(fce, enums).at(0) + ")";
        }
        else if (op == "struct") {
            throw WhyTranslateException(tag + " " + op);
        }
        else if (op == "record") {
            throw WhyTranslateException(tag + " " + op);
        }
        else if (op == "conc") {
            res << "(conc "
                   + translate(fce, enums).at(0) + ")";
        }
        else if (op == "succ") {
            res << "( "
                   + translate(fce, enums).at(0) + " + 1)";
        }
        else if (op == "pred") {
            res << "( "
                   + translate(fce, enums).at(0) + " - 1)";
        }
        else if (op == "rel") {
            res << "(to_relation "
                   + translate(fce, enums).at(0) + ")";
        }
        else if (op == "fnc") {
            res << "(to_function "
                   + translate(fce, enums).at(0) + ")";
        }
        else
            throw WhyTranslateException(tag + " " + op);
        return res;
    }
    else if (tag == "Nary_Pred" || tag == "Nary_Exp") {
        if(op == "&") {
            QDomElement subFormula = fce;
            while (!subFormula.isNull()) {
                res.append(translate(subFormula,enums));
                subFormula = subFormula.nextSiblingElement();
            }
        }
        else if(op == "or") {
            QString res2;
            QDomElement subFormula = fce;
            while (!subFormula.isNull()) {
                QString res1 = translatePred(subFormula, enums);
                if (!res1.isNull() && !(res1 == "true")) {
                    if (res2.isNull())
                        res2 = res1;
                    else
                        res2 = res2 + " \\/ " + res1;
                }
                else
                    return res;
                subFormula = subFormula.nextSiblingElement();
            }
            if (!res2.isNull())
                res << res2;
        }
        else if (op == "[") {
            res << translateExtensionSeq(fce, enums, 1);
        }
        else if (op == "{") {
            res << translateExtensionSet(fce, enums);
        }
        else
            throw WhyTranslateException(tag + " " + op);
        return res;

    }

    if (tag == "Quantified_Pred") {
        if (formula.attribute("type") == "#") {
            res << "(exists "
                   + translateVariables(formula.firstChildElement("Variables"), enums) + "."
                   + translatePred(formula.firstChildElement("Body"),enums) + ")";
        } else {
            res << "(forall "
                   + translateVariables(formula.firstChildElement("Variables"), enums) + "."
                   + translatePred(formula.firstChildElement("Body"),enums) + ")";
        }
        return res;
    }

    if (tag == "Left"
            || tag == "Right"
            || tag == "Body"
            || tag == "Pred"
            || tag == "Goal")
        return translate(fce, enums);

    if (tag == "Boolean_Literal") {
        QString val = formula.attribute("value");
        if (val == "TRUE")
            res << "True";
        if (val == "FALSE")
            res << "False";
        return res;
    }

    if (tag == "Integer_Literal" || tag == "Real_Literal") {
        QString val = formula.attribute("value");
        if (val.startsWith('-'))
            res << "(" + val + ")";
        else res << val;
        return res;
    }

    if (tag == "STRING_Literal") {
        QString val = formula.attribute("value");
        if (val.isEmpty()) {
            res << "(empty : set (int,int))";
        } else {
        QString l = "(";
        for (int i=0;i<val.length();i++) {
            if (i<val.length()-1) l+= "(union";
            l += "(singleton(" + QString::number(i) + "," + QString::number(val.at(i).toLatin1()) + "))";
        }
        for (int i=0;i<val.length()-1;i++) {
            l+=")";
        }
        l+=")";
        res << l;
        }
        return res;
    }

    if(tag == "Record_Field_Access") {
        res << translateRecordFieldAccess(formula, enums);
        return res;
    }

    if (tag == "Id") {
        res << translateId(formula, enums);
        return res;
    }

    if (tag == "Record") {
        res << translateRecord(formula, enums);
        return res;
    }

    if (tag == "Struct") {
        res << translateStruct(formula, enums);
        return res;
    }

    if (tag == "Quantified_Exp") {
        QString op = formula.attribute("type");
        QVector<QString> qVariables;
        QVector<QString> qTypes;
        QList<whyLocalHyp*> qLocalHyps;
        collectVarAndTypes(&qVariables,
                           &qTypes,
                           formula.firstChildElement("Pred"),
                           enums,
                           qLocalHyps);
        collectVarAndTypes(&qVariables,
                           &qTypes,
                           formula.firstChildElement("Body"),
                           enums,
                           qLocalHyps);

        QVector<QString> qVariablesQ;
        QVector<QString> qTypesQ;
        QVector<QString> qVariablesG;
        QVector<QString> qTypesG;
        VariablesDecl(qVariables,
                      qTypes,
                      formula.firstChildElement("Variables"),
                      &qVariablesQ, &qTypesQ,
                      &qVariablesG, &qTypesG, enums);

        QString preT = translatePred(formula.firstChildElement("Pred"), enums);

        if (op == "%") {
            QString bodyT = translate(formula.firstChildElement("Body"), enums).at(0);
            nbLambda++;
            QString l = "function anon_lambda_" + QString::number(nbLambda) + " ";
            for (int i=0;i<qVariablesG.size();i++)
                l += "(" + qVariablesG.at(i) + ": " + qTypesG.at(i) + ")";
            l += ": set((";
            for (int i=0;i<qVariablesQ.size()-1;i++)
                l += "(";
            for (int i=0;i<qVariablesQ.size();i++) {
                if (i>0)
                    l += "), ";
                l += qTypesQ.at(i);
            }
            l += "),";
            l += translateTypeInfo(TypingContext::getType(formula.firstChildElement("Body").firstChildElement()), enums);
            l += ")\n\naxiom anon_lambda_" + QString::number(nbLambda) + "_def :\n";
            l += "forall ";
            bool first = true;
            for (int i=0;i<qVariablesG.size();i++) {
                if (first)
                    first = false;
                else
                    l += ", ";
                l += qVariablesG.at(i) + ": " + qTypesG.at(i);
            }
            for (int i=0;i<qVariablesQ.size();i++) {
                if (first)
                    first = false;
                else
                    l += ", ";
                l += qVariablesQ.at(i) + ": " + qTypesQ.at(i);
            }
            l += " .\n";
            l += preT + "\n-> (apply (anon_lambda_" + QString::number(nbLambda) + " ";
            for (int i=0;i<qVariablesG.size();i++)
                l += qVariablesG.at(i) + " ";
            l+= ") (";
            for (int i=0;i<qVariablesQ.size()-1;i++)
                l+= "(";
            for (int i=0;i<qVariablesQ.size();i++) {
                if (i>0) l+= "),";
                l += qVariablesQ.at(i);
            }
            l+= ")) = " + bodyT + "\n";
            functions << l;
            l = "(anon_lambda_" + QString::number(nbLambda) + " ";
            for (int i=0;i<qVariablesG.size();i++)
                l += qVariablesG.at(i) + " ";
            l += ")";
            res << l;
            return res;
        } else if (op == "iSIGMA") {
            nbSigma++;
            return translateSigmaPi(formula, enums,
                             qVariablesG, qTypesG, qVariablesQ, qTypesQ,
                                    nbSigma, "sigma", "0", "+");
        } else if (op == "iPI") {
            nbPi++;
            return translateSigmaPi(formula, enums,
                             qVariablesG, qTypesG, qVariablesQ, qTypesQ,
                                    nbPi, "pi", "1", "*");
        } else if (op == "INTER") {
            QString bodyT = translate(formula.firstChildElement("Body"), enums).at(0);
            nbInter++;
            QString l = "function anon_inter_" + QString::number(nbInter) + " ";
            for (int i=0;i<qVariablesG.size();i++)
                l += "(" + qVariablesG.at(i) + ": " + qTypesG.at(i) + ")";
            l += ": ";
            l +=  translateTypeInfo(TypingContext::getType(formula),enums);
            l += "\n\naxiom anon_inter_" + QString::number(nbInter) + "_def :\n";
            l += "forall ";
            bool first = true;
            for (int i=0;i<qVariablesG.size();i++) {
                if (first)
                    first = false;
                else
                    l += ", ";
                l += qVariablesG.at(i) + ": " + qTypesG.at(i);
            }
            if (first)
                first = false;
            else
                l += ", ";
            l += "xx : " + translateTypeInfo(TypingContext::getType(formula).firstChildElement(),enums);
            l += " .\nmem xx (anon_inter_" + QString::number(nbInter);
            for (int i=0;i<qVariablesG.size();i++)
                l += " " + qVariablesG.at(i);
            l += ")\n<-> (forall ";
            first = true;
            for (int i=0;i<qVariablesQ.size();i++) {
                if (first)
                    first = false;
                else
                    l += ", ";
                l += qVariablesQ.at(i) + ": " + qTypesQ.at(i);
            }
            l += ".\n(" + preT + ")\n-> (mem xx (" + bodyT + ")))\n";
            functions << l;
            l = "(anon_inter_" + QString::number(nbInter);
            for (int i=0;i<qVariablesG.size();i++)
                l += " " + qVariablesG.at(i);
            l += ")";
            res << l;
            return res;
        } else if (op == "UNION") {
            QString bodyT = translate(formula.firstChildElement("Body"), enums).at(0);
            nbUnion++;
            QString l = "function anon_union_" + QString::number(nbUnion) + " ";
            for (int i=0;i<qVariablesG.size();i++)
                l += "(" + qVariablesG.at(i) + ": " + qTypesG.at(i) + ")";
            l += ": ";
            l +=  translateTypeInfo(TypingContext::getType(formula),enums);
            l += "\n\naxiom anon_union_" + QString::number(nbUnion) + "_def :\n";
            l += "forall ";
            bool first = true;
            for (int i=0;i<qVariablesG.size();i++) {
                if (first)
                    first = false;
                else
                    l += ", ";
                l += qVariablesG.at(i) + ": " + qTypesG.at(i);
            }
            if (first)
                first = false;
            else
                l += ", ";
            l += "xx : " + translateTypeInfo(TypingContext::getType(formula).firstChildElement(),enums);
            l += " .\nmem xx (anon_union_" + QString::number(nbUnion);
            for (int i=0;i<qVariablesG.size();i++)
                l += " " + qVariablesG.at(i);
            l += ")\n<-> (exists ";
            first = true;
            for (int i=0;i<qVariablesQ.size();i++) {
                if (first)
                    first = false;
                else
                    l += ", ";
                l += qVariablesQ.at(i) + ": " + qTypesQ.at(i);
            }
            l += ".\n(" + preT + ")\n/\\ (mem xx (" + bodyT + ")))\n";
            functions << l;
            l = "(anon_union_" + QString::number(nbUnion);
            for (int i=0;i<qVariablesG.size();i++)
                l += " " + qVariablesG.at(i);
            l += ")";
            res << l;
            return res;
        }
    }

    if (tag == "Quantified_Set") {
        QVector<QString> qVariables;
        QVector<QString> qTypes;
        QList<whyLocalHyp*> qLocalHyps;
        collectVarAndTypes(&qVariables,
                           &qTypes,
                           formula.firstChildElement("Body"),
                           enums,
                           qLocalHyps);

        QVector<QString> qVariablesQ;
        QVector<QString> qTypesQ;
        QVector<QString> qVariablesG;
        QVector<QString> qTypesG;
        VariablesDecl(qVariables,
                      qTypes,
                      formula.firstChildElement("Variables"),
                      &qVariablesQ, &qTypesQ,
                      &qVariablesG, &qTypesG, enums);

        QString bodyT = translatePred(formula.firstChildElement("Body"), enums);

        nbSet++;
        QString l = "function anon_set_" + QString::number(nbSet) + " ";
        for (int i=0;i<qVariablesG.size();i++)
            l += "(" + qVariablesG.at(i) + ": " + qTypesG.at(i) + ")";
        l += ": ";
        l +=  translateTypeInfo(TypingContext::getType(formula),enums);
        l += "\n\naxiom anon_set_" + QString::number(nbSet) + "_def :\n";
        l += "forall ";
        bool first = true;
        for (int i=0;i<qVariablesG.size();i++) {
            if (first)
                first = false;
            else
                l += ", ";
            l += qVariablesG.at(i) + ": " + qTypesG.at(i);
        }
        for (int i=0;i<qVariablesQ.size();i++) {
            if (first)
                first = false;
            else
                l += ", ";
            l += qVariablesQ.at(i) + ": " + qTypesQ.at(i);
        }
        l += " .\n mem (";

        for (int i=0;i<qVariablesQ.size()-1;i++)
            l+= "(";
        for (int i=0;i<qVariablesQ.size();i++) {
            if (i>0) l+= "),";
            l += qVariablesQ.at(i);
        }
        l += ")(anon_set_" + QString::number(nbSet) + " ";
        for (int i=0;i<qVariablesG.size();i++)
            l += qVariablesG.at(i) + " ";
        l+= ") <-> " + bodyT + "\n";
        functions << l;
        l = "(anon_set_" + QString::number(nbSet) + " ";
        for (int i=0;i<qVariablesG.size();i++)
            l += qVariablesG.at(i) + " ";
        l += ")";
        res << l;
        return res;
    }


    if (tag == "Boolean_Exp") {
        res << "(if "
               + translatePred(fce, enums) + "then True else False)";
        return res;
    }

    if (tag == "EmptySet" || tag == "EmptySeq") {
        res << "(empty : " + translateTypeInfo(TypingContext::getType(formula), enums) + ")";
        return res;
    }

    // Pour les valuations
    if (tag == "Set") {
        QDomElement enumE = formula.firstChildElement("Enumerated_Values");
        if (enumE.isNull()) {
            QString setName = translate(fce, enums).at(0);
            res << "(mem " + setName + " (finite_subsets integer)) /\\ not(" + setName + "== (empty : set int))";
        } else
            res << "true";
        return res;
    }
    
    throw WhyTranslateException(tag + " " + op);
}

QStringList whyPredicate::translateSigmaPi(QDomElement formula, const QMap<QString,QString>& enums,
                                           QVector<QString> qVariablesG,
                                           QVector<QString> qTypesG,
                                           QVector<QString> qVariablesQ,
                                           QVector<QString> qTypesQ,
                                           int nbOp,
                                           QString op, QString a1, QString a2) {
    QStringList res;
    QDomElement qs = formula.ownerDocument().createElement("Quantified_Set");
    QDomElement  att = formula.ownerDocument().createElement("Attr");
    qs.appendChild(att);
    QDomElement  ti = formula.ownerDocument().createElement("Type");
    att.appendChild(ti);
    QDomElement ue = formula.ownerDocument().createElement("Unary_Exp");
    ti.appendChild(ue);
    ue.setAttribute("op", "POW");
    QDomElement vars = formula.firstChildElement("Variables").firstChildElement("Id");
    if (!vars.isNull()) {
        ue.appendChild(TypingContext::getType(vars).cloneNode().toElement());
        vars = vars.nextSiblingElement("Id");
    }
    while (!vars.isNull()) {
        QDomElement be = formula.ownerDocument().createElement("Binary_Exp");
        be.setAttribute("op", "*");
        ue.appendChild(be);
        be.appendChild(ue.firstChild());
        be.appendChild(TypingContext::getType(vars).cloneNode().toElement());
        vars = vars.nextSiblingElement("Id");
    }
    qs.appendChild(formula.firstChildElement("Variables").cloneNode().toElement());
    QDomElement b = formula.firstChildElement("Pred").cloneNode().toElement();
    b.setTagName("Body");
    qs.appendChild(b);
    QString setName = translate(qs, enums).at(0);
    QString bodyT = translate(formula.firstChildElement("Body"), enums).at(0);
    QString l = "function anon_" + op + "_" + QString::number(nbOp) + " ";
    for (int i=0;i<qVariablesG.size();i++)
        l += "(" + qVariablesG.at(i) + ": " + qTypesG.at(i) + ")";
    l += " (p_" + op + "_" + QString::number(nbOp) + " : set(";
    for (int i=0;i<qVariablesQ.size()-1;i++)
        l+= "(";
    for (int i=0;i<qVariablesQ.size();i++) {
        if (i>0) l+= "),";
        l += qTypesQ.at(i);
    }
    l += ")) : int\n\n";

    QString l1 = "axiom anon_" + op + "_" + QString::number(nbOp) + "_def0 : ";
    bool first = true;
    for (int i=0;i<qVariablesG.size();i++) {
        if (first) {
            l1 += "forall ";
            first = false;
        } else
            l1 += ", ";
        l1 += qVariablesG.at(i) + ": " + qTypesG.at(i);
    }
    if (!first)
        l1 += ".";
    l1 += " anon_" + op + "_" + QString::number(nbOp) + " ";
    for (int i=0;i<qVariablesG.size();i++)
        l1 += qVariablesG.at(i) + " ";
    l1 += "(empty : set(";

    for (int i=0;i<qVariablesQ.size()-1;i++)
        l1+= "(";
    for (int i=0;i<qVariablesQ.size();i++) {
        if (i>0) l1+= "),";
        l1 += qTypesQ.at(i);
    }
    l1 += ")) = " + a1 + " \n\n";
    QString l2 = "axiom anon_" + op + "_" + QString::number(nbOp) + "_def_ind : forall ";
    first = true;
    for (int i=0;i<qVariablesG.size();i++) {
        if (first)
            first = false;
        else
            l2 += ", ";
        l2 += qVariablesG.at(i) + ": " + qTypesG.at(i);
    }
    if (first)
        first = false;
    else
        l2 += ", ";
    l2 += "p_" + op + "_" + QString::number(nbOp) + " : set(";
    for (int i=0;i<qVariablesQ.size()-1;i++)
        l2+= "(";
    for (int i=0;i<qVariablesQ.size();i++) {
        if (i>0) l2+= "),";
        l2 += qTypesQ.at(i);
    }
    l2 += ")";
    for (int i=0;i<qVariablesQ.size();i++) {
        l2 += ", " + qVariablesQ.at(i) + ": " + qTypesQ.at(i);
    }
    l2 += ". mem (";
    for (int i=0;i<qVariablesQ.size()-1;i++)
        l2+= "(";
    for (int i=0;i<qVariablesQ.size();i++) {
        if (i>0) l2+= "),";
        l2 += qVariablesQ.at(i);
    }
    l2 += ") ";
    l2 += "p_" + op + "_" + QString::number(nbOp);
    l2 += " -> anon_" + op + "_" + QString::number(nbOp) + " ";
    for (int i=0;i<qVariablesG.size();i++)
        l2 += qVariablesG.at(i) + " ";
    l2 += "p_" + op + "_" + QString::number(nbOp);
    l2 += " = anon_" + op + "_" + QString::number(nbOp) + " ";
    for (int i=0;i<qVariablesG.size();i++)
        l2 += qVariablesG.at(i) + " ";
    l2 += "(diff ";
    l2 += "p_" + op + "_" + QString::number(nbOp);
    l2 += " (singleton (";
    for (int i=0;i<qVariablesQ.size()-1;i++)
        l2+= "(";
    for (int i=0;i<qVariablesQ.size();i++) {
        if (i>0) l2+= "),";
        l2 += qVariablesQ.at(i);
    }
    l2 += "))) " + a2 + " " + bodyT;
    functions << l + l1 + l2;
    l = "(anon_" + op + "_" + QString::number(nbOp) + " ";
    for (int i=0;i<qVariablesG.size();i++)
        l += qVariablesG.at(i) + " ";
    l += setName + ")";
    res << l;
    return res;
}


QString whyPredicate::translateExtensionSet(QDomElement formula, const QMap<QString,QString>& enums) {

    QDomElement sibling = formula.nextSiblingElement();
    if(sibling.isNull())
        return "(singleton " + translate(formula, enums).at(0) + ")";
    else
        return "(union (singleton " + translate(formula, enums).at(0) + ") " + translateExtensionSet(sibling, enums) + ")";
}

QString whyPredicate::translateExtensionSeq(QDomElement formula, const QMap<QString,QString>& enums, int index) {

    QDomElement sibling = formula.nextSiblingElement();
    if(sibling.isNull())
        return "(singleton (" + QString::number(index) + ", " + translate(formula, enums).at(0) + "))";
    else
        return "(union (singleton (" + QString::number(index) + ", " + translate(formula, enums).at(0) + ")) "
                + translateExtensionSeq(sibling, enums, index+1) + ")";
}

QString whyPredicate::VariablesDecl(QVector<QString> variables,
                                    QVector<QString> types,
                                    QDomElement formula,
                                    QVector<QString>* variablesQ,
                                    QVector<QString>* typesQ,
                                    QVector<QString>* variablesG,
                                    QVector<QString>* typesG, QMap<QString,QString> enums) {
    QString res;
    QDomElement fce = formula.firstChildElement("Id");
    while (!fce.isNull()) {
        bool var;
        QString name = whyName(fce, enums, var);
        if (var) {
            variablesQ->append(name);
            typesQ->append(translateTypeInfo(TypingContext::getType(fce), enums));
        }
        fce = fce.nextSiblingElement("Id");
    }
    for (int i=0;i < variables.size();i++) {
        if (!variablesQ->contains(variables.at(i))) {
            variablesG->append(variables.at(i));
            typesG->append(types.at(i));
        }
    }
    return res;
}

QString whyPredicate::translateVariables(QDomElement formula, const QMap<QString,QString>& enums) {
    QString res;
    bool first = true;
    QDomElement fce = formula.firstChildElement("Id");
    while (!fce.isNull()) {
        if (first) {
            first = false;
        } else {
            res += ", ";
        }
        res += translateId(fce, enums);
        res += ": ";
        res += translateTypeInfo(TypingContext::getType(fce),enums);

        fce = fce.nextSiblingElement("Id");
    }
    return res;
}



bool whyPredicate::isSet(QDomElement e) {
    QDomElement ti = TypingContext::getType(e);
    return (ti.tagName() == "Unary_Exp" && ti.attribute("op") == "POW");
}

whyLocalHyp::whyLocalHyp(QDomElement d, int _goal, QString _num) : whyPredicate(d) {
    predName = "lh_" + QString::number(_goal) + "_" + _num;
    goal = QString::number(_goal);
    num = _num;
    duplicate = 0;
    predicate = "true";
}


void whyLocalHyp::declareWithoutDuplicate(QList<whyLocalHyp*> lhs, QTextStream& out) {
    for (int i=0;i<lhs.size();i++) {
        if (lhs.at(i)->predicate == predicate
                &&  lhs.at(i)->variables.size() == variables.size()) {
            bool dup = true;
            for (int j=0;j < variables.size(); j++) {
                dup = (dup
                        && lhs.at(i)->variables.at(j) == variables.at(j)
                        && lhs.at(i)->types.at(j) == types.at(j));
            }
            if (dup) {
                duplicate = lhs.at(i);
                return;
            }
        }
    }
    if(!obf)
        out << "\n(* " << num << " *)";
    declare(out);
}

QString whyLocalHyp::declareCallDuplicate() {
    if (duplicate == 0) {
        return declareCall();
    } else
        return duplicate->declareCallDuplicate();
}


QString whyPredicate::declareCall() {
    QStringList args;
    foreach (QString variable, variables)
        args << variable;
    return "(" + predName + " " + args.join(" ") + ")";
}

whySimpleGoal::whySimpleGoal(QDomElement d, int goal, QList<whyLocalHyp*> wlh, QVector<whyDefine*> wds, whyLocalHyp *hyp)
    : whyPredicate(d, wlh)
{
    predName = "g_" + QString::number(goal);
    defines = wds;
    hypothesis = hyp;
    for (int i=0;i<wds.size();i++) {
        QVector<QString> v = wds.at(i)->getVariables();
        for (int j=0;j<v.size();j++) {
            if (!variables.contains(v.at(j))) {
                variables << v.at(j);
                types << wds.at(i)->getTypes().at(j);
            }
        }
    }
    QVector<QString> v = hyp->getVariables();
    for (int j=0;j<v.size();j++) {
        if (!variables.contains(v.at(j))) {
            variables << v.at(j);
            types << hyp->getTypes().at(j);
        }
    }
}

void whySimpleGoal::declare(QTextStream& out) {
    QStringList structs;
    QMapIterator<QString, QString> i(structTypes);
    while (i.hasNext()) {
        i.next();
        int j=0;
        while (j<structs.size()) {
            if (structs.at(j).contains(i.value()))
                break;
            else
                j++;
        }
        structs.insert(j,"type " + i.value() + " = " + i.key());
    }
    out << structs.join("\n");
    out << "\n";
    QStringList tmp;
    for (int i =0; i < functions.size(); i++) {
        QString function = functions.at(i);
        tmp << function;
    }
    out << tmp.join("\n");
    out << "\ngoal " << predName << " :\n";
    if (!variables.isEmpty()) {
        out << "   forall ";
        for (int i=0;i < variables.size(); i++) {
            if (i>0)
                out << ", ";
            out << variables.at(i) + ": " + types.at(i);
        }
        out << ".\n   ";
    }
    for (int i=0;i<defines.size();i++) {
        if (i>0)
            out << " /\\ ";
        out << defines.at(i)->declareCall();
    }
    out << " /\\ ";
    out << hypothesis->declareCall();
    out << "\n   -> \n   " << predicate << "\n\n";
}

whyLemmaGoal::whyLemmaGoal(QDomElement d, QVector<whyLemmaDefine*> wds)
    : whyPredicate(d)
{
    predName = "g";
    defines = wds;
    for (int i=0;i<wds.size();i++) {
        QVector<QString> v = wds.at(i)->getVariables();
        for (int j=0;j<v.size();j++) {
            if (!variables.contains(v.at(j))) {
                variables << v.at(j);
                types << wds.at(i)->getTypes().at(j);
            }
        }
    }
}

void whyLemmaGoal::translate(QMap<QString,QString> enums) {
    predicate = whyPredicate::translate(e, enums).join("");
}

void whyLemmaGoal::declare(QTextStream& out) {
    QStringList structs;
    QMapIterator<QString, QString> i(structTypes);
    while (i.hasNext()) {
        i.next();
        int j=0;
        while (j<structs.size()) {
            if (structs.at(j).contains(i.value()))
                break;
            else
                j++;
        }
        structs.insert(j,"type " + i.value() + " = " + i.key());
    }
    out << structs.join("\n");
    structs.clear();
    out << "\n";
    for (int i=0;i<functions.size();i++)
        out << functions.at(i) << "\n";
    // pas compatible Qt 5.2: out << QStringList::fromVector(functions).join("\n");
    out << "\ngoal " << predName << " :\n";
    QStringList tmp;
    if (!variables.isEmpty()) {
        out << "   forall ";
        for (int i=0;i < variables.size(); i++)
            tmp << variables.at(i) + ": " + types.at(i);
        out << tmp.join(", ");
        out << ".\n   ";
    }
    tmp.clear();
    if (!defines.isEmpty()) {
        for (int i=0;i<defines.size();i++)
            tmp << defines.at(i)->declareCall();
        out << tmp.join(" /\\ ");
        out << "\n   -> \n   ";
    }
    tmp.clear();
    out << predicate << "\n\n";
}

whyLemmaDefine::whyLemmaDefine(QDomElement d, int count)
    : whyDefine(d, count) {}

void whyLemmaDefine::translate(QMap<QString, QString> enums) {
    predicate = whyPredicate::translate(e, enums).join(" /\\ ");
}

WhyTranslateException::WhyTranslateException(const char *desc)
    : description(desc)
{

}

WhyTranslateException::WhyTranslateException(const std::string& desc)
    : description(desc)
{

}

WhyTranslateException::WhyTranslateException(const QString desc)
    : description(desc.toLocal8Bit().data())
{

}

WhyTranslateException::~WhyTranslateException() throw ()
{

}

const char *WhyTranslateException::what() const throw()
{
    return description.c_str();
}

static bool isEnumeratedSetElement(const QDomElement &e)
{
    return !e.isNull() && e.tagName() == "Set" && !e.firstChildElement("Enumerated_Values").isNull();
}

static bool isDeferredSetElement(const QDomElement& e)
{
    return !e.isNull() && e.tagName() == "Set" && e.firstChildElement("Enumerated_Values").isNull();
}

static QString deferredSetWhyName(const QDomElement& e1)
{
    QDomElement id = e1.firstChildElement("Id");
    return obfusc("v_" + id.attribute("value") + "_" + id.attribute("suffix"));
}

static bool eqType
(const QDomElement& e1,
 const QDomElement& e2)
{
    if(e1.isNull())
        return e2.isNull();
    if(e2.isNull())
        return false;
    if(e1.tagName() != e2.tagName())
        return false;
    if(e1.tagName() == "Binary_Exp") {
        QDomElement c1 = child1(e1);
        QDomElement c2 = child1(e2);
        if(!eqType(c1, c2))
            return false;
        c1 = childX(c1);
        c2 = childX(c2);
        return eqType(c1, c2);
    }
    if(e1.tagName() == "Id") {
        return e1.attribute("value") == e2.attribute("value");
    }
    if(e1.tagName() == "Unary_Exp") {
        QDomElement c1 = child1(e1);
        QDomElement c2 = child1(e2);
        return eqType(c1, c2);
    }
    if(e1.tagName() == "Struct") {
        QDomElement c1 = e1.firstChildElement("Record_Item");
        QDomElement c2 = e2.firstChildElement("Record_Item");
        while(!c1.isNull() && !c2.isNull()) {
            if(c1.attribute("label") != c2.attribute("label"))
                return false;
            if(!eqType(child1(c1), child1(c2)))
                return false;
            c1 = c1.nextSiblingElement("Record_Item");
            c2 = c2.nextSiblingElement("Record_Item");
        }
        return c1.isNull() && c2.isNull();
    }
    if(e1.tagName() == "Generic_Type")
        return true;

    throw WhyTranslateException("Unknown type element " + e1.tagName());
}

QString whyApply(const QString& fun,
                 const QVector<QString>& args)
{
    QString res;
    res = "(" + fun ;
    for(int i = 0;i < args.size(); ++i)
        res += (i > 0 ? " " : "") + args.at(i);
    res += ")";
    return res;
}
