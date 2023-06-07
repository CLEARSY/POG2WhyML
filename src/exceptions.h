#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include<exception>
#include<string>

class WhyTranslateException : public std::exception
{
public:
    WhyTranslateException(const char *desc);
    WhyTranslateException(const std::string& str);
    ~WhyTranslateException() throw();

    virtual const char *what() const throw();

private:
    std::string description;
};

struct NotTypedEmptyException : public std::exception
{
    virtual const char *what() const throw() {
        return "Cannot type empty set";
    }

};

#endif // EXCEPTIONS_H