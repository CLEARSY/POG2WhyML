#include<string>

#include "exceptions.h"

using std::string;

WhyTranslateException::WhyTranslateException(const char *desc)
    : description(desc)
{

}

WhyTranslateException::WhyTranslateException(const std::string& desc)
    : description(desc)
{

}

WhyTranslateException::~WhyTranslateException() throw ()
{

}

const char *WhyTranslateException::what() const throw()
{
    return description.c_str();
}