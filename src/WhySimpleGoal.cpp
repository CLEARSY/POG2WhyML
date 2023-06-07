#include "WhyMLInternal.h"

#include "utils.h"

whySimpleGoal::whySimpleGoal
    (const XMLElement *dom, 
    const string &groupName,
    unsigned goal, 
    const vector<whyLocalHyp *> &wlh, 
    const vector<whyDefine *> &wds, 
    whyLocalHyp * hyp)
    : whyPredicate{dom, string("g_") + groupName + std::to_string(goal)}
    , m_defines{wds}
    , m_hypothesis{hyp}
    , m_localHypotheses{wlh}
{
    for (const auto& define: m_defines) {
        const auto& vars {define->variables()};
        const auto& types {define->types()};
        for (const auto& var: vars) {
            if (!m_variables.contains(var)) {
                m_variables.insert(var);
                const auto &it {types.find(var)};
                if (it == types.end())
                    continue;
                m_types.insert({var, it->second});
            }
        }
    }
    const auto& htypes {hyp->types()};
    for (const auto& var: hyp->variables()) {
        if (!m_variables.contains(var)) {
            m_variables.insert(var);
            const auto &it {htypes.find(var)};
            if (it == htypes.end())
                continue;
            m_types.insert({var, it->second});
        }
    }
}

void whySimpleGoal::declare(ofstream &os) {
    /* TODO
    vector<string> structs;
    mapIterator<string, string> i(structTypes);
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
    out << "\n"; */
    bool first{true};
    for(const auto &f: m_functions) {
        os << f << std::endl;
    }
    os << std::endl
        << "goal " << m_name << " :" << std::endl;
    if (!m_variables.empty()) {
        os << "\t" << "forall ";
        first = true;
        for(const auto &v: m_variables) {
            if (first) {
                first = false;
            }
            else {
                os << ", ";
            }
            os << v << ":" << m_types.find(v)->second;
        }
        os << "." << std::endl;
    }
    first = true;
    for(const auto &d: m_defines) {
        os << "\t";
        if (first) {
            first = false;
        }
        else {
            os << "/\\ ";
        }
        os << d->declareCall();
    }
    os << "\t/\\ ";
    os << m_hypothesis->declareCall();
    os << std::endl
       << "\t->" << std::endl
       << "\t" << m_translation << std::endl << std::endl;
}

void whySimpleGoal::collectVarAndTypes(const dict_t &enums,
                                      const TypingContext &tc) {
    for (auto var: m_hypothesis->variables()) {
        m_variables.insert(var);
        m_types.insert({var, m_hypothesis->types().find(var)->second});
    }

    const XMLElement *child = m_dom->FirstChildElement();
    while(child != nullptr) {
        const XMLElement *next = child->NextSiblingElement();
        const char *tag {child->Name()};
        if (cstrEq(tag, "Ref_Hyp")) {
            unsigned num {child->UnsignedAttribute("num")};
            whyLocalHyp *hyp {m_localHypotheses.at(num-1)};
            const dict_t &dict {hyp->types()};
            for (const auto& var: hyp->variables()) {
                if (!m_variables.contains(var)) {
                    const string type {dict.find(var)->second};
                    m_variables.insert(var);
                    m_types.insert({var , type});
                }
            }
        }
        else if (cstrEq(tag, "Goal")) {
            whyPredicate::collectVarAndTypes(m_variables, m_types, child, enums, tc);
        }
        child = next;
    }
}


void whySimpleGoal::translate(const dict_t &enums, const TypingContext &typeinfo) {

    bool first {true};
    stringstream ss;
    for (const auto& h: m_localHypotheses) {
        if (first) {
            first = false;
        }
        else {
            ss << " /\\\n";
        }
        ss << h->declareCall();
    }
    stringstream ssg;
    Translator translator{ssg, enums, typeinfo};
    translator.translate(m_dom->FirstChildElement("Goal"));
    const auto goal {translator.translation()};
    if (first) {
        ss << goal;
    }
    else {
        ss << " -> " << goal;
    }
    m_translation = ss.str();

}

ostream &operator<<(ostream &os, const whySimpleGoal &wp) {
    os << "--- whySimpleGoal " << std::endl
       << static_cast<const whyPredicate&>(wp);
    os << "--- end whySimpleGoal " << std::endl << std::endl;
    return os;
}