#ifndef WHY_H
#define WHY_H

#include<map>
#include<vector>

#include<QDomElement>
#include<QFile>
#include<QList>
#include<QString>
#include<QVector>
#include<QMap>

typedef QHash< uint, QDomElement > TypeInfos;

void saveWhy3(QDomDocument pog, QFile& why, const std::map<int, std::vector<int>>& goals);
void saveWhy3(QDomDocument pog, QFile& why, bool xml, bool obfs);


//static QString obfusc(QString name);

void saveWhy3(TypeInfos* typeInfos, QList<QDomElement> hypotheses, QDomElement goal, QFile& file);


class whyLocalHyp;

class whyPredicate {
public:
    whyPredicate(QDomElement e);
    whyPredicate(QDomElement e, QList<whyLocalHyp*> localHyps);

    void collectVarAndTypes(QMap<QString,QString> enums);
    void translate(const QMap<QString, QString> &enums);

    virtual void declare(QTextStream& out);
    QString declareCall();

    QVector<QString> getVariables() { return variables; }
    QVector<QString> getTypes() { return types; }

protected:
    QString predName;
    QString predicate;
    QVector<QString> variables;
    QVector<QString> types;
    QVector<QString> functions;
    QMap<QString,QString> structTypes;
    QDomElement e;
    QStringList translate(const QDomElement formula, const QMap<QString, QString> &enums);

private:
    QList<whyLocalHyp*> localHyps;

    void collectVarAndTypes(QVector<QString>* variables,
                                   QVector<QString>* types,
                                   QDomElement formula,QMap<QString,QString> enums,
                                   QList<whyLocalHyp *> localHyps);
    QString translateTypeStruct
        (const QDomElement&,
         const QMap<QString,QString>&);
    QString translateTypeInfo(QDomElement ti, const QMap<QString, QString> &enums);
    QString translatePred(QDomElement formula, const QMap<QString, QString> &enums);
    QStringList translateSigmaPi(QDomElement formula, const QMap<QString,QString> &enums, QVector<QString> qVariablesG, QVector<QString> qTypesG, QVector<QString> qVariablesQ, QVector<QString> qTypesQ, int nbOp, QString op, QString a1, QString a2);
    QString translateExtensionSet(QDomElement formula, const QMap<QString,QString> &enums);
    QString translateExtensionSeq(QDomElement formula, const QMap<QString,QString> &enums, int index);
    QString translateVariables(QDomElement formula, const QMap<QString, QString> &enums);
    QString translateId
        (const QDomElement& e,
         const QMap<QString,QString>& enums);
   QString translateStruct
        (const QDomElement& e,
         const QMap<QString, QString> &enums);
    QString translateRecord
        (const QDomElement& e,
         const QMap<QString, QString> &enums);
    QString translateRecordFieldAccess
        (const QDomElement& e,
         const QMap<QString, QString> &enums);
    QString VariablesDecl(QVector<QString> variables, QVector<QString> types,
                          QDomElement formula,
                          QVector<QString>* variablesQ,
                          QVector<QString>* typesQ,
                          QVector<QString>* variablesG, QVector<QString> *typesG, QMap<QString, QString> enums);

    QDomElement merge(QDomElement type1, QDomElement type2, const QMap<QString, QString> &enums, const QDomElement &src);

    bool isSet(QDomElement e) ;
    QDomElement subset(QDomElement e);
private:
    static QDomElement left(const QDomElement e);
    static QDomElement right(const QDomElement e);
};

class whyDefine : public whyPredicate {
public:
    whyDefine(QDomElement d, int count);

private:
    QString name;
};

class whyLocalHyp : public whyPredicate {
public:
    whyLocalHyp(QDomElement d, int goal, QString num);

    void declareWithoutDuplicate(QList<whyLocalHyp*>, QTextStream& out);
    QString declareCallDuplicate();

    bool is(QString _num) {
        return num == _num;
    }

private:
    QString num;
    QString goal;
    whyLocalHyp* duplicate;
};

class whySimpleGoal : public whyPredicate {
public:
    whySimpleGoal(QDomElement d, int goal, QList<whyLocalHyp*>, QVector<whyDefine*>, whyLocalHyp* hypothesis);
    void declare(QTextStream& out);

private:
    QVector<whyDefine*> defines;
    whyLocalHyp* hypothesis;
};

class whyLemmaDefine;

class whyLemmaGoal : public whyPredicate {
public:
    whyLemmaGoal(QDomElement d, QVector<whyLemmaDefine*> wds);
    void declare(QTextStream& out);
    void translate(QMap<QString,QString> enums);

private:
    QVector<whyLemmaDefine*> defines;
    whyLocalHyp* hypothesis;
};

class whyLemmaDefine : public whyDefine {
public:
    whyLemmaDefine(QDomElement d, int count);
    void translate(QMap<QString, QString> enums);
};

class WhyTranslateException : public std::exception
{
public:
    WhyTranslateException(const char *desc);
    WhyTranslateException(const QString desc);
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
#endif // WHY_H
