#include "WhyMLInternal.h"

whyDefine::whyDefine(const XMLElement *dom, int count)
    : whyPredicate{dom, string("define_") + dom->Attribute("name")/* std::to_string(count)*/}
    , m_label{string(dom->Attribute("name"))}
{
    
}

void whyDefine::declare(ofstream& os)
{
    os << "(* " << m_label << " *)" << std::endl;
    this->whyPredicate::declare(os);
}

